
#include <swfdec.h>
#include <swfdec_render.h>
#include <swfdec_buffer.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <glib.h>

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

#include <SDL.h>

gboolean debug = FALSE;

SwfdecDecoder *s;
unsigned char *image;
unsigned char *image4;

GtkWidget *drawing_area;
GtkWidget *gtk_wind;
GdkWindow *gdk_parent;

GIOChannel *input_chan;

guint input_idle_id;
guint render_idle_id;

int width;
int height;
int fast = FALSE;
int enable_sound = TRUE;
int quit_at_eof = FALSE;

double rate;
int interval;
struct timeval image_time;

static void do_help(void);
static void new_gtk_window(void);

static void sound_setup(void);

void convert_image (unsigned char *dest, unsigned char *src, int width,
    int height);

/* GTK callbacks */
static void destroy_cb (GtkWidget *widget, gpointer data);
static int expose_cb (GtkWidget *widget, GdkEventExpose *evt, gpointer data);
static int key_press (GtkWidget *widget, GdkEventKey *evt, gpointer data);
static int motion_notify (GtkWidget *widget, GdkEventMotion *evt, gpointer data);
static int configure_cb (GtkWidget *widget, GdkEventConfigure *evt, gpointer data);

static gboolean render_idle_audio (gpointer data);
static gboolean render_idle_noaudio (gpointer data);

int main(int argc, char *argv[])
{
	int c;
	int index;
	static struct option options[] = {
		{ "width", 1, NULL, 'w' },
		{ "height", 1, NULL, 'h' },
		{ "fast", 0, NULL, 'f' },
		{ "no-sound", 0, NULL, 's' },
		{ "quit", 0, NULL, 'q' },
		{ 0 },
	};
        char *contents;
        int length;
        int ret;

	gtk_init(&argc,&argv);
	gdk_rgb_init ();
	gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
	gtk_widget_set_default_visual(gdk_rgb_get_visual());

	s = swfdec_decoder_new();

	while(1){
		c = getopt_long(argc, argv, "qsfx:w:h:", options, &index);
		if(c==-1)break;

		switch(c){
		case 'w':
			width = strtoul(optarg, NULL, 0);
			printf("width set to %d\n",width);
			break;
		case 'h':
			height = strtoul(optarg, NULL, 0);
			printf("height set to %d\n",height);
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
		default:
			do_help();
			break;
		}
	}

	if(optind != argc-1)do_help();

	if(width){
		swfdec_decoder_set_image_size(s,width,height);
	}

        ret = g_file_get_contents (argv[optind], &contents, &length, NULL);
        if (!ret) {
          g_print("error reading file\n");
          exit(1);
        }
        swfdec_decoder_add_data (s, contents, length);

        while (1) {
          ret = swfdec_decoder_parse (s);
          if (ret == SWF_NEEDBITS ) {
            swfdec_decoder_eof (s);
          }
          if (ret == SWF_ERROR) {
            g_print("error while parsing\n");
            exit(1);
          }
          if (ret == SWF_EOF) {
            break;
          }
        }

        swfdec_decoder_get_rate (s, &rate);
        interval = 1000.0/rate;

        swfdec_decoder_get_image_size (s, &width, &height);

	new_gtk_window();

	if(enable_sound)sound_setup();

        if (enable_sound) {
          g_timeout_add (0, render_idle_audio, NULL);
        } else {
          g_timeout_add (0, render_idle_noaudio, NULL);
        }

	gtk_main();

	exit(0);
}

static void do_help(void)
{
	fprintf(stderr,"swf_play file.swf\n");
	exit(1);
}

static void new_gtk_window(void)
{
	gtk_wind = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_window_set_default_size(GTK_WINDOW(gtk_wind), width, height);
	gtk_signal_connect(GTK_OBJECT(gtk_wind), "delete_event",
		GTK_SIGNAL_FUNC (destroy_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(gtk_wind), "destroy",
		GTK_SIGNAL_FUNC(destroy_cb), NULL);

	drawing_area = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(gtk_wind),
		GTK_WIDGET(drawing_area));

	g_signal_connect(G_OBJECT(drawing_area), "expose_event",
		GTK_SIGNAL_FUNC(expose_cb), NULL);
	g_signal_connect (G_OBJECT (drawing_area), "configure_event",
		GTK_SIGNAL_FUNC(configure_cb), NULL);

	g_signal_connect (G_OBJECT(gtk_wind), "key_press_event",
		GTK_SIGNAL_FUNC(key_press), NULL);
	g_signal_connect (G_OBJECT(gtk_wind), "motion_notify_event",
		GTK_SIGNAL_FUNC(motion_notify), NULL);

	gtk_widget_add_events(gtk_wind, GDK_POINTER_MOTION_MASK);

	gtk_widget_show_all(gtk_wind);

}

GList *sound_buffers;
int sound_bytes;
unsigned char *sound_buf;

static void fill_audio(void *udata, Uint8 *stream, int len)
{
	GList *g;
	SwfdecBuffer *buffer;
	int n;
	int offset = 0;

	while(1){
		g = g_list_first(sound_buffers);
		if(!g)break;

		buffer = (SwfdecBuffer *)g->data;
                if (buffer->length < len - offset) {
                  n = buffer->length;
		  memcpy(sound_buf + offset, buffer->data, n);
                  swfdec_buffer_unref (buffer);
                  sound_buffers = g_list_delete_link (sound_buffers, g);
                } else {
                  SwfdecBuffer *subbuffer;

                  n = len - offset;
		  memcpy(sound_buf + offset, buffer->data, n);
                  subbuffer = swfdec_buffer_new_subbuffer (buffer, n,
                      buffer->length - n);
                  g->data = subbuffer;
                  swfdec_buffer_unref (buffer);
                }

		sound_bytes -= n;
		offset += n;

		if(offset >= len)break;
	}

	if(offset<len){
		memset(sound_buf+offset,0,len-offset);
	}

	SDL_MixAudio(stream, sound_buf, len, SDL_MIX_MAXVOLUME);
}

static void sound_setup(void)
{
	SDL_AudioSpec wanted;

	wanted.freq = 44100;
#if G_BYTE_ORDER == 4321
	wanted.format = AUDIO_S16MSB;
#else
	wanted.format = AUDIO_S16LSB;
#endif
	wanted.channels = 2;
	wanted.samples = 1024;
	wanted.callback = fill_audio;
	wanted.userdata = NULL;

	sound_buf = malloc(1024*2*2);

	if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
		fprintf(stderr, "Couldn't open audio: %s, disabling\n", SDL_GetError());
		enable_sound = FALSE;
	}

	SDL_PauseAudio(0);
}


/* GTK callbacks */

static int key_press (GtkWidget *widget, GdkEventKey *evt, gpointer data)
{
	if(debug)fprintf(stderr,"key press\n");
	
	return FALSE;
}

static int motion_notify (GtkWidget *widget, GdkEventMotion *evt, gpointer data)
{
	if(debug)fprintf(stderr,"motion notify\n");
	
	return FALSE;
}

static int configure_cb (GtkWidget *widget, GdkEventConfigure *evt, gpointer data)
{
	return FALSE;
}

static int expose_cb (GtkWidget *widget, GdkEventExpose *evt, gpointer data)
{
	if(image){
		gdk_draw_rgb_image (widget->window, widget->style->black_gc, 
			0, 0, width, height, 
			GDK_RGB_DITHER_NONE,
			image,
			width*3);
	}

	return FALSE;
}

static void destroy_cb (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}


#if 0
static void tv_add_usec(struct timeval *a, unsigned int x)
{
	a->tv_usec += x;
	while(a->tv_usec >= 1000000){
		a->tv_sec++;
		a->tv_usec-=1000000;
	}
}

static int tv_compare(struct timeval *a,struct timeval *b)
{
	if(a->tv_sec > b->tv_sec)return 1;
	if(a->tv_sec == b->tv_sec){
		if(a->tv_usec > b->tv_usec)return 1;
		if(a->tv_usec == b->tv_usec)return 0;
	}
	return -1;
}

static int tv_diff(struct timeval *a,struct timeval *b)
{
	int diff;
	diff = (a->tv_sec - b->tv_sec)*1000000;
	diff += (a->tv_usec - b->tv_usec);
	return diff;
}
#endif

static void
fixup_buffer (SwfdecBuffer *buffer)
{
  int i;
  unsigned char tmp;
  unsigned char *data = buffer->data;

  for(i=0;i<buffer->length;i+=4){
    tmp = data[2];
    data[2] = data[0];
    data[0] = tmp;
    data+=4;
  }

}

static gboolean render_idle_audio(gpointer data)
{
        SwfdecBuffer *video_buffer;
        SwfdecBuffer *audio_buffer;

        if (sound_bytes >= 10000) {
          g_timeout_add (10, render_idle_audio, NULL);
          return FALSE;
        }

        swfdec_render_iterate (s);

        video_buffer = swfdec_render_get_image (s);
        audio_buffer = swfdec_render_get_audio (s);

        sound_buffers = g_list_append (sound_buffers, audio_buffer);
	sound_bytes += audio_buffer->length;

        fixup_buffer (video_buffer);

	gdk_draw_rgb_32_image (drawing_area->window,
		drawing_area->style->black_gc, 
		0, 0, width, height, 
		GDK_RGB_DITHER_NONE,
		video_buffer->data,
		width*4);
          swfdec_buffer_unref (video_buffer);

  g_timeout_add (10, render_idle_audio, NULL);

	return FALSE;
}

static gboolean render_idle_noaudio(gpointer data)
{
        SwfdecBuffer *video_buffer;
        SwfdecBuffer *audio_buffer;

        swfdec_render_iterate (s);

        video_buffer = swfdec_render_get_image (s);
        audio_buffer = swfdec_render_get_audio (s);

        swfdec_buffer_unref (audio_buffer);

	gdk_draw_rgb_32_image (drawing_area->window,
		drawing_area->style->black_gc, 
		0, 0, width, height, 
		GDK_RGB_DITHER_NONE,
		video_buffer->data,
		width*4);
          swfdec_buffer_unref (video_buffer);

  g_timeout_add (interval, render_idle_noaudio, NULL);

	return FALSE;
}

