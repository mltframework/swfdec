
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib.h>
#include <gst/xoverlay/xoverlay.h>
#include <gst/navigation/navigation.h>

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include "gstappsrc.h"
#include "spp.h"

#define MAX_VERSION 100

#define static

struct sound_buffer_struct
{
  int len;
  int offset;
  void *data;
};
typedef struct sound_buffer_struct SoundBuffer;

gboolean debug = TRUE;

unsigned char *image;

GtkWidget *gtk_wind;

//GtkWidget *video_widget;
GdkWindow *gdk_parent;

GIOChannel *input_chan;

guint input_idle_id;
guint render_idle_id;

unsigned long xid;
unsigned long eb_xid;
int fast = FALSE;
int enable_sound = TRUE;
int quit_at_eof = FALSE;
int safe = FALSE;

int interval;
struct timeval image_time;

//int video_window_x;
//int video_window_y;
//int video_window_w;
//int video_window_h;

//GdkWindow *video_window;

char *filename;

gboolean visible;
gboolean iconified;

/* state variables */
gboolean have_pipeline;
gboolean is_plugged;
gboolean error_occurred;
gboolean streaming = FALSE;
gboolean playing;

/* functions */
static void do_help (void);
static void print_formats (void);
static gboolean create_ui (void);
gboolean ui_create_pipeline(gpointer ignored);
static gboolean create_pipeline (int streaming, const char *location);
static void handle_error (const char *format, ...);
static void map_xoverlay (void);

/* Gstreamer callbacks */
static void desired_size (GObject * obj, int w, int h, gpointer closure);
static void embed_url (GObject * obj, const char *url, const char *target, gpointer closure);

static void packet_write (int fd, int code, int len, const char *s);
static void packet_go_to_url (const char *url, const char *target);

/* GTK callbacks */
static void destroy_cb (GtkWidget * widget, gpointer data);
static void embedded (GtkPlug * plug, gpointer data);
static int expose (GtkWidget * widget, GdkEventExpose * evt, gpointer data);
static int button_press_event (GtkWidget * widget, GdkEventButton * evt,
    gpointer data);
static int button_release_event (GtkWidget * widget, GdkEventButton * evt,
    gpointer data);
static int key_press (GtkWidget * widget, GdkEventKey * evt, gpointer data);
static int motion_notify (GtkWidget * widget, GdkEventMotion * evt,
    gpointer data);
static int configure_cb (GtkWidget * widget, GdkEventConfigure * evt,
    gpointer data);
static int window_state_event (GtkWidget * widget, GdkEventWindowState * evt, gpointer data);
static int visibility_notify_event (GtkWidget * widget, GdkEventVisibility * evt, gpointer data);
static int unmap (GtkWidget *widget, gpointer closure);
static gboolean timeout (gpointer closure);

static void video_widget_realize (GtkWidget * widget, gpointer ignored);
static void video_widget_allocate (GtkWidget * widget,
    GtkAllocation * allocation, gpointer ignored);

/* fault handling stuff */
void fault_handler (int signum, siginfo_t * si, void *misc);
void fault_restore (void);
void fault_setup (void);

static GstElement *pipeline;
static GstElement *thread;
static GstElement *src;
static GstElement *swfdec;
static GstElement *thread2;
static GstElement *thread3;
static GstElement *queue1;
static GstElement *queue2;
static GstElement *audioconvert;
static GstElement *audioscale;
static GstElement *audio_sink;
static GstElement *colorspace;
static GstElement *video_sink;
static GstElement *xoverlay;


int
main (int argc, char *argv[])
{
  int c;
  int index;

  //int log_fd;
  static struct option options[] = {
    {"xid", 1, NULL, 'x'},
    {"safe", 0, NULL, 0x1001},
    {"plugin", 0, NULL, 0x1002},
    {"fast", 0, NULL, 'f'},
    {"no-sound", 0, NULL, 's'},
    {"quit", 0, NULL, 'q'},
    {"print-formats", 0, NULL, 0x1000},
    {0},
  };

#if 0
  log_fd = g_file_open_tmp ("gst-mozilla-plugin.log.XXXXXX", NULL, NULL);
  dup2 (log_fd, 2);
#endif

  gst_init (NULL, NULL);

  gtk_init (&argc, &argv);
  gdk_rgb_init ();
  gtk_widget_set_default_colormap (gdk_rgb_get_cmap ());
  gtk_widget_set_default_visual (gdk_rgb_get_visual ());

  while (1) {
    c = getopt_long (argc, argv, "qsfx:w:h:", options, &index);
    if (c == -1)
      break;

    switch (c) {
      case 'x':
        xid = strtoul (optarg, NULL, 0);
        break;
      case 'f':
        fast = TRUE;
        break;
      case 's':
        enable_sound = FALSE;
        break;
      case 'q':
        quit_at_eof = TRUE;
        break;
      case 0x1000:
        print_formats ();
        break;
      case 0x1001:
        safe = TRUE;
        break;
      case 0x1002:
        streaming = TRUE;
        break;
      default:
        do_help ();
        break;
    }
  }

  if (xid) {
    is_plugged = 1;
  }

  create_ui ();

  filename = argv[optind];

  g_idle_add (ui_create_pipeline, NULL);

  g_timeout_add (1000, timeout, NULL);

  gtk_main ();

  exit (0);
}

gboolean
ui_create_pipeline(gpointer ignored)
{
  int ret;

  if (filename || streaming) {
    have_pipeline = create_pipeline(streaming, filename);
    if (!have_pipeline) {
      handle_error ("Creation of pipeline failed.");
    }
  } else {
    have_pipeline = FALSE;
  }

  if (safe) {
    handle_error ("Safe mode");
  } else {
    ret = gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
    if (ret != GST_STATE_SUCCESS) {
      handle_error ("Failed to set pipeline to PLAYING");
    }
    playing = TRUE;
  }
  
  return FALSE;
}

#define CREATE_ELEMENT(e,type) \
  do { \
    e = gst_element_factory_make (type, NULL); \
    if (e == NULL) { \
      char *s; \
      if (bad_elements) { \
        s = g_strconcat (bad_elements, " ", type, NULL); \
        g_free(bad_elements); \
        bad_elements = s; \
      } else { \
        bad_elements = g_strdup (type); \
      } \
    } \
  } while (0)


static gboolean
create_pipeline (int streaming, const char *location)
{
  int ret;
  char *bad_elements = NULL;
  
  CREATE_ELEMENT (pipeline, "pipeline");
  CREATE_ELEMENT (thread, "thread");
  if (streaming) {
    CREATE_ELEMENT (src, "appsrc");
  } else {
    if (location == NULL) {
      handle_error ("No file specified");
      return FALSE;
    }
    CREATE_ELEMENT (src, "filesrc");
  }
  CREATE_ELEMENT (swfdec, "swfdec2");
  CREATE_ELEMENT (thread2, "thread");
  CREATE_ELEMENT (queue1, "queue");
  CREATE_ELEMENT (thread3, "thread");
  CREATE_ELEMENT (queue2, "queue");
  CREATE_ELEMENT (audioconvert, "audioconvert");
  CREATE_ELEMENT (audioscale, "audioscale");
  CREATE_ELEMENT (colorspace, "ffmpegcolorspace");
  CREATE_ELEMENT (video_sink, "ximagesink");
  CREATE_ELEMENT (audio_sink, "gconfaudiosink");

  if (bad_elements) {
    handle_error ("Failed to create elements: %s", bad_elements);
    g_free(bad_elements);
    return FALSE;
  }

  xoverlay = video_sink;

  if (!streaming) {
    g_object_set (src, "location", location, NULL);
  }
  g_object_set (G_OBJECT(queue2), "max-size-buffers", 1, NULL);

  map_xoverlay();
  g_signal_connect (G_OBJECT (xoverlay), "desired-size-changed",
      G_CALLBACK (desired_size), NULL);

  g_signal_connect (G_OBJECT (swfdec), "embed-url",
      G_CALLBACK (embed_url), NULL);

  gst_bin_add (GST_BIN (pipeline), thread);
  gst_bin_add_many (GST_BIN (thread), src, swfdec, thread2, thread3, NULL);
  gst_bin_add_many (GST_BIN (thread2), queue1, audioconvert, audioscale,
      audio_sink, NULL);
  gst_bin_add_many (GST_BIN (thread3), queue2, colorspace, video_sink, NULL);

  ret = gst_element_link (src, swfdec);
  ret &= gst_element_link (swfdec, queue2);
  ret &= gst_element_link (queue2, colorspace);
  ret &= gst_element_link (colorspace, video_sink);
  ret &= gst_element_link (swfdec, queue1);
  ret &= gst_element_link (queue1, audioscale);
  ret &= gst_element_link (audioscale, audioconvert);
  ret &= gst_element_link (audioconvert, audio_sink);
  if (ret == FALSE) {
    handle_error ("Failed to link elements");
    return FALSE;
  }

  return TRUE;
}

static void
do_help (void)
{
  fprintf (stderr, "gst-mozilla-player [--xid NNN] file\n");
  exit (1);
}

static void
print_formats (void)
{
  g_print ("application/x-shockwave-flash:swf:Shockwave Flash");
  exit (0);
}

static gboolean
create_ui (void)
{
  if (is_plugged) {
    gtk_wind = gtk_plug_new (0);
    gtk_signal_connect (GTK_OBJECT (gtk_wind), "embedded",
        GTK_SIGNAL_FUNC (embedded), NULL);

    gdk_parent = gdk_window_foreign_new (xid);
    //gdk_window_get_geometry (gdk_parent, NULL, NULL, &width, &height, NULL);
  } else {
    gtk_wind = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  }
  //g_object_set (G_OBJECT(gtk_wind), "visible", FALSE, NULL);
  gtk_window_set_default_size (GTK_WINDOW (gtk_wind), 320, 200);
  gtk_signal_connect (GTK_OBJECT (gtk_wind), "delete_event",
      GTK_SIGNAL_FUNC (destroy_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (gtk_wind), "destroy",
      GTK_SIGNAL_FUNC (destroy_cb), NULL);

  g_signal_connect (G_OBJECT (gtk_wind), "realize",
      GTK_SIGNAL_FUNC (video_widget_realize), NULL);
  g_signal_connect (G_OBJECT (gtk_wind), "size_allocate",
      GTK_SIGNAL_FUNC (video_widget_allocate), NULL);

  g_signal_connect (G_OBJECT (gtk_wind), "visibility-notify-event",
      GTK_SIGNAL_FUNC (visibility_notify_event), NULL);
  g_signal_connect (G_OBJECT (gtk_wind), "window-state-event",
      GTK_SIGNAL_FUNC (window_state_event), NULL);
  g_signal_connect (G_OBJECT (gtk_wind), "unmap",
      GTK_SIGNAL_FUNC (unmap), NULL);

  g_signal_connect (G_OBJECT (gtk_wind), "key_press_event",
      GTK_SIGNAL_FUNC (key_press), NULL);
  g_signal_connect (G_OBJECT (gtk_wind), "motion_notify_event",
      GTK_SIGNAL_FUNC (motion_notify), NULL);
  g_signal_connect (G_OBJECT (gtk_wind), "button_press_event",
      GTK_SIGNAL_FUNC (button_press_event), NULL);
  g_signal_connect (G_OBJECT (gtk_wind), "button_release_event",
      GTK_SIGNAL_FUNC (button_release_event), NULL);
#if 0
  g_signal_connect (G_OBJECT (gtk_wind), "expose_event",
      GTK_SIGNAL_FUNC (expose), NULL);
#endif

  gtk_widget_add_events (gtk_wind,
      GDK_LEAVE_NOTIFY_MASK
      | GDK_BUTTON_PRESS_MASK
      | GDK_BUTTON_RELEASE_MASK
      | GDK_VISIBILITY_NOTIFY_MASK
      | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

  gtk_widget_show_all (gtk_wind);

  if (is_plugged) {
    XReparentWindow (GDK_WINDOW_XDISPLAY (gtk_wind->window),
        GDK_WINDOW_XID (gtk_wind->window), xid, 0, 0);
    XMapWindow (GDK_WINDOW_XDISPLAY (gtk_wind->window),
        GDK_WINDOW_XID (gtk_wind->window));
  }

  return TRUE;
}

static void
handle_error (const char *format, ...)
{
  GtkWidget *label;
  GtkWidget *align;
  char *error_message;
  char *s;
  va_list varargs;

  va_start (varargs, format);
  error_message = g_strdup_vprintf (format, varargs);
  va_end (varargs);

  if (playing) {
    gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PAUSED);
  }

  s = g_strdup_printf("An error occurred playing SWF file:\n%s\n",
      error_message);
  g_free(error_message);
  label = gtk_label_new(s);
  g_free(s);
  
  align = gtk_alignment_new (0.5, 0.5, 0, 0);
  gtk_container_add (GTK_CONTAINER(gtk_wind), align);
  gtk_container_add (GTK_CONTAINER(align), label);
  gtk_widget_show_all (align);

  g_signal_connect (G_OBJECT (align), "expose_event",
      GTK_SIGNAL_FUNC (expose), NULL);
}

/* GTK callbacks */

static gboolean
timeout (gpointer closure)
{
#if 0
  if (debug) {
    if (GTK_WIDGET_MAPPED(gtk_wind)){
      fprintf(stderr, "is mapped\n");
    }else{
      fprintf(stderr, "is unmapped\n");
    }
  }
#endif

  return TRUE;
}

static void
menu_open (GtkMenuItem *item, gpointer user_data)
{
  char *url;

  g_object_get (G_OBJECT(src), "source_url", &url, NULL);

  packet_go_to_url(url, "_self");

  g_free (url);
}

static void
menu_report_bug (GtkMenuItem *item, gpointer user_data)
{
  char *url;
  char *s;

  if (streaming) {
    g_object_get (G_OBJECT(src), "source_url", &url, NULL);

    s = g_strdup_printf("http://www.schleef.org/swfdec/?%s", url);
    packet_go_to_url(s, "_self");
    g_free(s);
  }
}

static void
menu_copy_url (GtkMenuItem *item, gpointer user_data)
{
  char *url;
  GtkClipboard *clipboard;

  if (streaming) {
    g_object_get (G_OBJECT(src), "source_url", &url, NULL);
    clipboard = gtk_clipboard_get_for_display (gdk_display_get_default(),
        GDK_SELECTION_PRIMARY);

    gtk_clipboard_set_text (clipboard, url, strlen(url));
  }
}


static int
button_press_event (GtkWidget * widget, GdkEventButton * evt, gpointer data)
{
  GtkWidget *menu;

  if (debug)
    fprintf (stderr, "button press\n");

  if (evt->button == 3) {
    GtkWidget *item;

    menu = gtk_menu_new ();
    item = gtk_menu_item_new_with_label ("Open in new page");
    g_signal_connect (G_OBJECT(item), "activate", G_CALLBACK(menu_open), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    item = gtk_menu_item_new_with_label ("Report bug...");
    g_signal_connect (G_OBJECT(item), "activate", G_CALLBACK(menu_report_bug), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    item = gtk_menu_item_new_with_label ("Copy URL");
    g_signal_connect (G_OBJECT(item), "activate", G_CALLBACK(menu_copy_url), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    gtk_widget_show_all (menu);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 3, evt->time);
  }
  if (evt->button == 1) {
    if (xoverlay) {
      gst_navigation_send_mouse_event (GST_NAVIGATION (xoverlay),
          "mouse-button-press", 0, evt->x, evt->y);
    }
  }

  return FALSE;
}

static int
button_release_event (GtkWidget * widget, GdkEventButton * evt, gpointer data)
{
  if (debug)
    fprintf (stderr, "button release\n");

  if (evt->button == 1) {
    if (xoverlay) {
      gst_navigation_send_mouse_event (GST_NAVIGATION (xoverlay),
          "mouse-button-release", 0, evt->x, evt->y);
    }
  }

  return FALSE;
}

static int
key_press (GtkWidget * widget, GdkEventKey * evt, gpointer data)
{
  if (debug)
    fprintf (stderr, "key press\n");

  //gtk_exit(0);

  return FALSE;
}

static int
motion_notify (GtkWidget * widget, GdkEventMotion * evt, gpointer data)
{
  if (debug)
    fprintf (stderr, "motion notify\n");

  /* ximagesink gets these automatically */
#if 0
  {
    int x, y;
    GdkModifierType state;

    gdk_window_get_pointer (evt->window, &x, &y, &state);

    gst_navigation_send_mouse_event (GST_NAVIGATION (xoverlay),
        "mouse-move", 0, x, y);
  }
#endif

  return TRUE;
}

static void
check_playing (void)
{
  gboolean should_play;
  //int ret;

  if (visible && !iconified) {
    should_play = TRUE;
  } else {
    should_play = FALSE;
  }

#if 0
should_play = TRUE;
  if (should_play != playing) {
    if (should_play) {
      if (debug) fprintf(stderr, "setting pipeline to PLAYING");
      ret = gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
    } else {
      if (debug) fprintf(stderr, "setting pipeline to PAUSED");
      ret = gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PAUSED);
    }
    if (ret != GST_STATE_SUCCESS) {
      fprintf(stderr, "Failed to set pipeline to PLAYING\n");
      exit(1);
    }
    playing = should_play;
  }
#endif

}

static int
window_state_event (GtkWidget * widget, GdkEventWindowState * evt, gpointer data)
{
  if (debug)
    fprintf(stderr, "window state 0x%08x\n", evt->new_window_state);

  if (evt->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
    iconified = TRUE;
  } else {
    iconified = FALSE;
  }

  check_playing();

  return FALSE;
}

static int
visibility_notify_event (GtkWidget * widget, GdkEventVisibility * evt, gpointer data)
{
  if (debug)
    fprintf (stderr, "visibility notify\n");

  if (evt->state == GDK_VISIBILITY_FULLY_OBSCURED) {
    visible = FALSE;
  } else {
    visible = TRUE;
  }

  check_playing();
    
  return FALSE;
}

static int
unmap (GtkWidget *widget, gpointer closure)
{
  if (debug)
    fprintf (stderr, "unmap\n");

  return FALSE;
}

static int
configure_cb (GtkWidget * widget, GdkEventConfigure * evt, gpointer data)
{
  map_xoverlay();

  return FALSE;
}

static int
expose (GtkWidget * widget, GdkEventExpose * evt, gpointer data)
{
  if (xoverlay) {
    gst_x_overlay_expose (GST_X_OVERLAY(xoverlay));
  }

  return FALSE;
}

static void
destroy_cb (GtkWidget * widget, gpointer data)
{
  gtk_main_quit ();
}

static void
embedded (GtkPlug * plug, gpointer data)
{
}


static void
video_widget_realize (GtkWidget * widget, gpointer ignored)
{

  //video_window = widget->window;

  //map_xoverlay();
}

static void
map_xoverlay (void)
{
  if (xoverlay) {
    if (GST_IS_X_OVERLAY (xoverlay) && GDK_IS_WINDOW (gtk_wind->window)) {
      /* This is necessary because we're sending an XID for a window that
       * the X server doesn't necessarily know about yet. */
      XSync (GDK_DISPLAY (), False);
      /* Now it knows. */
      gst_x_overlay_set_xwindow_id (GST_X_OVERLAY (xoverlay),
          GDK_WINDOW_XID (gtk_wind->window));
    } else {
      g_warning ("Could not find an XOVERLAY element for reparenting");
    }
  }
}

static void
video_widget_allocate (GtkWidget * widget,
    GtkAllocation * allocation, gpointer ignored)
{
  gint x, y, w, h;

  gdk_window_get_geometry (gtk_wind->window, &x, &y, &w, &h, NULL);

  //video_window_x = x;
  //video_window_y = y;
  //video_window_w = w;
  //video_window_h = h;
}

static void
desired_size (GObject * obj, int w, int h, gpointer closure)
{
  if (debug)
    fprintf (stderr, "got size %d %d\n", w, h);

  gtk_window_resize (GTK_WINDOW (gtk_wind), w, h);

  map_xoverlay();
}

static void
embed_url (GObject * obj, const char *url, const char *target, gpointer closure)
{
  packet_go_to_url (url, target);
}

static void
packet_go_to_url (const char *url, const char *target)
{
  char *buf;
  int len;

  len = strlen (url) + 1 + strlen (target) + 1;
  buf = g_malloc (len);

  memcpy (buf, url, strlen(url) + 1);
  memcpy (buf + strlen(url) + 1, target, strlen (target) + 1);

  if (streaming) {
    packet_write (1, SPP_GO_TO_URL2, len, buf);
  } else {
    fprintf(stderr, "go to URL %s\n", url);
  }

  g_free (buf);
}

static void
packet_write (int fd, int code, int len, const char *s)
{
  char *buf;

  buf = g_malloc (len + 8);
  memcpy (buf, &code, 4);
  memcpy (buf + 4, &len, 4);
  memcpy (buf + 8, s, len);
  write (fd, buf, len + 8);

  g_free (buf);
}

