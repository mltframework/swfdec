#include <gtk/gtk.h>
#include <swfdec.h>
#include <swfdec_render.h>
#include <swfdec_buffer.h>
#include <swfdec_decoder.h>

typedef struct {
  int x;
  int y;
  int button;
  gboolean changed;
} MouseData;

static gboolean
motion_notify (GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
  MouseData *mouse = data;

  mouse->x = event->x;
  mouse->y = event->y;
  mouse->changed = TRUE;
  return FALSE;
}

static gboolean
leave_notify (GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
  MouseData *mouse = data;

  mouse->x = -1;
  mouse->y = -1;
  mouse->changed = TRUE;
  return FALSE;
}

static gboolean
press_notify (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  MouseData *mouse = data;

  mouse->button = 1;
  mouse->changed = TRUE;
  return FALSE;
}

static gboolean
release_notify (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  MouseData *mouse = data;

  mouse->button = 0;
  mouse->changed = TRUE;
  return FALSE;
}

static gboolean
next_image (gpointer data)
{
  GtkWidget *win = data;
  SwfdecDecoder *dec = g_object_get_data (G_OBJECT (win), "swfdec");
  MouseData *mouse = g_object_get_data (G_OBJECT (win), "swfmouse");

  if (mouse->changed) {
    swfdec_decoder_set_mouse (dec, mouse->x, mouse->y, mouse->button);
    mouse->changed = FALSE;
  }
  if (!swfdec_render_iterate (dec)) {
    gtk_main_quit ();
    return FALSE;
  }
  gtk_widget_queue_draw (win);
  
  return TRUE;
}

static gboolean
draw_current_image (GtkWidget *win, GdkEventExpose *event, SwfdecDecoder *dec)
{
  SwfdecBuffer *buf = swfdec_render_get_image (dec);
  cairo_surface_t *surface;
  cairo_t *cr;
  int width, height;

  if (!buf) {
    g_assert_not_reached ();
    return FALSE;
  }
  swfdec_decoder_get_image_size (dec, &width, &height);
  surface = cairo_image_surface_create_for_data (buf->data,
      CAIRO_FORMAT_ARGB32, width, height, width * 4);
  cr = gdk_cairo_create (win->window);
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_surface_destroy (surface);
  cairo_paint (cr);
  cairo_destroy (cr);

  return TRUE;
}

static void
view_swf (SwfdecDecoder *dec)
{
  GtkWidget *win;
  double rate;
  int width, height;
  MouseData mouse = { 0, 0, 0, FALSE };
  
  /* create toplevel window and set its title */
  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW(win), "Flash File");
  
  /* exit when 'X' is clicked */
  g_object_set_data (G_OBJECT (win), "swfdec", dec);
  g_object_set_data (G_OBJECT (win), "swfmouse", &mouse);
  g_signal_connect (win, "destroy", G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect_after (win, "expose-event", G_CALLBACK (draw_current_image), dec);
  g_signal_connect_after (win, "motion-notify-event", G_CALLBACK (motion_notify), &mouse);
  g_signal_connect_after (win, "leave-notify-event", G_CALLBACK (leave_notify), &mouse);
  g_signal_connect_after (win, "button-press-event", G_CALLBACK (press_notify), &mouse);
  g_signal_connect_after (win, "button-release-event", G_CALLBACK (release_notify), &mouse);
  gtk_widget_add_events (win, GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_MASK
      | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK);
  
  swfdec_decoder_get_image_size (dec, &width, &height);
  gtk_window_set_default_size (GTK_WINDOW (win), width, height);
  gtk_widget_show_all (win);
  swfdec_decoder_get_rate (dec, &rate);
  g_timeout_add (1000 / rate, next_image, win);
  
  gtk_main ();

  g_object_unref (win);
}

static void buffer_free (SwfdecBuffer *buffer, void *priv)
{
  g_free (buffer->data);
}

int main (int argc, char *argv[])
{
  gsize length;
  int ret;
  char *fn = "it.swf";
  SwfdecDecoder *s;
  char *contents;
  SwfdecBuffer *buffer;
  GError *error = NULL;

  swfdec_init ();
  gtk_init (&argc, &argv);

  if(argc>=2){
	fn = argv[1];
  }

  ret = g_file_get_contents (fn, &contents, &length, &error);
  if (!ret) {
    g_printerr ("Couldn't open file \"%s\": %s\n", fn, error->message);
    return 1;
  }

  s = swfdec_decoder_new();
  buffer = swfdec_buffer_new_with_data (contents, length);
  buffer->free = buffer_free;
  ret = swfdec_decoder_add_buffer(s, buffer);

  while (ret != SWF_EOF) {
    ret = swfdec_decoder_parse(s);
    if (ret == SWF_NEEDBITS) {
      swfdec_decoder_eof(s);
    }
    if (ret == SWF_ERROR) {
      g_print("error while parsing\n");
      return 1;
    }
    if (ret == SWF_IMAGE) {
      break;
    }
  }

  view_swf (s);

  swfdec_decoder_free(s);
  s=NULL;
  return 0;
}

