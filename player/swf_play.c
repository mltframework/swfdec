
#include <swfdec.h>

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

struct sound_buffer_struct{
	int len;
	int offset;
	void *data;
};
typedef struct sound_buffer_struct SoundBuffer;

gboolean debug = FALSE;

SwfdecDecoder *s;
unsigned char *image;
unsigned char *image4;

GtkWidget *drawing_area;
GtkWidget *gtk_wind;
int plugged;
GdkWindow *gdk_parent;

GIOChannel *input_chan;

guint input_idle_id;
guint render_idle_id;

unsigned long xid;
int width;
int height;
int fast = FALSE;
int enable_sound = TRUE;
int quit_at_eof = FALSE;

int interval;
struct timeval image_time;

static void do_help(void);
static void read_swf_file(char *fn);
static void read_swf_stdin(void);
static void new_gtk_window(void);

static void sound_setup(void);

void convert_image (unsigned char *dest, unsigned char *src, int width,
    int height);

/* GTK callbacks */
static void destroy_cb (GtkWidget *widget, gpointer data);
static int expose_cb (GtkWidget *widget, GdkEventExpose *evt, gpointer data);
static int key_press (GtkWidget *widget, GdkEventKey *evt, gpointer data);
static int motion_notify (GtkWidget *widget, GdkEventMotion *evt, gpointer data);
static void embedded (GtkPlug *plug, gpointer data);
static int configure_cb (GtkWidget *widget, GdkEventConfigure *evt, gpointer data);

/* GIOChan callbacks */
static gboolean input(GIOChannel *chan, GIOCondition cond, gpointer ignored);
static gboolean render_idle(gpointer data);

/* fault handling stuff */
void fault_handler(int signum, siginfo_t *si, void *misc);
void fault_restore(void);
void fault_setup(void);

int main(int argc, char *argv[])
{
	int c;
	int index;
	static struct option options[] = {
		{ "xid", 1, NULL, 'x' },
		{ "width", 1, NULL, 'w' },
		{ "height", 1, NULL, 'h' },
		{ "fast", 0, NULL, 'f' },
		{ "no-sound", 0, NULL, 's' },
		{ "quit", 0, NULL, 'q' },
		{ 0 },
	};

	//fault_setup();

	gtk_init(&argc,&argv);
	gdk_rgb_init ();
	gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
	gtk_widget_set_default_visual(gdk_rgb_get_visual());

	s = swfdec_decoder_new();

	while(1){
		c = getopt_long(argc, argv, "qsfx:w:h:", options, &index);
		if(c==-1)break;

		switch(c){
		case 'x':
			xid = strtoul(optarg, NULL, 0);
			break;
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

	if(strcmp(argv[optind],"-")==0){
		read_swf_stdin();
	}else{
		read_swf_file(argv[optind]);
	}

	if(xid){
		plugged = 1;
	}
	new_gtk_window();

	if(enable_sound)sound_setup();

	gtk_main();

	exit(0);
}

static void do_help(void)
{
	fprintf(stderr,"swf_play [--xid NNN] file.swf\n");
	exit(1);
}

static void new_gtk_window(void)
{
	if(plugged){
		gtk_wind = gtk_plug_new(0);
		gtk_signal_connect(GTK_OBJECT(gtk_wind), "embedded",
			GTK_SIGNAL_FUNC(embedded), NULL);

		gdk_parent = gdk_window_foreign_new(xid);
		gdk_window_get_geometry(gdk_parent, NULL, NULL, &width, &height, NULL);
		printf("width=%d height=%d\n",width,height);
	}else{
		gtk_wind = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	}
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

	if(plugged){
		XReparentWindow(GDK_WINDOW_XDISPLAY(gtk_wind->window),
			GDK_WINDOW_XID(gtk_wind->window),
			xid, 0, 0);
		XMapWindow(GDK_WINDOW_XDISPLAY(gtk_wind->window),
			GDK_WINDOW_XID(gtk_wind->window));
	}
}

static void read_swf_file(char *fn)
{
	struct stat sb;
	int fd;
	int ret;
	unsigned char *data;
	int len;

	fd = open(fn,O_RDONLY);
	if(fd<0){
		perror(fn);
		exit(1);
	}

	ret = fstat(fd, &sb);
	if(ret<0){
		perror("stat");
		exit(1);
	}

	len = sb.st_size;
	data = malloc(len);
	ret = read(fd, data, len);
	if(ret<0){
		perror("read");
		exit(1);
	}

	ret = swfdec_decoder_addbits(s,data,len);
	free(data);

	if(!render_idle_id)render_idle_id = g_idle_add(render_idle,NULL);
}

static void read_swf_stdin(void)
{
	GError *error = NULL;

	input_chan = g_io_channel_unix_new(0);

	g_io_channel_set_encoding(input_chan, NULL, &error);
	g_io_channel_set_flags(input_chan, G_IO_FLAG_NONBLOCK, &error);

	input_idle_id = g_io_add_watch(input_chan, G_IO_IN, input, NULL);
}

GList *sound_buffers;
int sound_bytes;
unsigned char *sound_buf;

static void fill_audio(void *udata, Uint8 *stream, int len)
{
	GList *g;
	SoundBuffer *buffer;
	int n;
	int offset = 0;

	while(1){
		g = g_list_first(sound_buffers);
		if(!g)break;

		buffer = (SoundBuffer *)g->data;
		n = MIN(buffer->len - buffer->offset,len - offset);

		memcpy(sound_buf + offset, buffer->data + buffer->offset, n);
		sound_bytes -= n;
		buffer->offset += n;
		offset += n;
		if(buffer->offset >= buffer->len){
			sound_buffers = g_list_delete_link(sound_buffers,g);
			free(buffer->data);
			free(buffer);
		}

		if(offset >= len)break;
	}

	if(offset<len){
		memset(sound_buf+offset,0,len-offset);
	}

	SDL_MixAudio(stream, sound_buf, len, SDL_MIX_MAXVOLUME);
}

static void pull_sound(SwfdecDecoder *s)
{
	SoundBuffer *sb;
	void *data;
	int n;

	while(1){
		data = swfdec_decoder_get_sound_chunk(s, &n);
		if(!data)return;

		if(enable_sound){
			sb = g_new(SoundBuffer,1);

			sb->len = n;
			sb->offset = 0;
			sb->data = data;

			sound_buffers = g_list_append(sound_buffers, sb);

			sound_bytes += n;
		}else{
			g_free(data);
		}
	}
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
	wanted.channels = 2;    /* 1 = mono, 2 = stereo */
	wanted.samples = 1024;  /* Good low-latency value for callback */
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

static void embedded (GtkPlug *plug, gpointer data)
{
}

/* idle stuff */

static gboolean input(GIOChannel *chan, GIOCondition cond, gpointer ignored)
{
	char *data;
	gsize bytes_read;
	GError *error = NULL;
	int ret;

	data = malloc(4096);
	ret = g_io_channel_read_chars(chan, data, 4096, &bytes_read, &error);
	if(ret==G_IO_STATUS_NORMAL){
		ret = swfdec_decoder_addbits(s,data,bytes_read);
		if(!render_idle_id)render_idle_id = g_idle_add(render_idle,NULL);
	}else if(ret==G_IO_STATUS_ERROR){
		exit(1);
	}else if(ret==G_IO_STATUS_EOF){
		gtk_idle_remove(input_idle_id);
		if(!render_idle_id)render_idle_id = g_idle_add(render_idle,NULL);
	}

	free(data);

	return TRUE;
}

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

static gboolean render_idle(gpointer data)
{
	int ret;
	static int ack = 0;

	ack++;
	if(ack>=1)ack=0;
	/* disabled temporarily */
#if 0
	if(ack==0){
		swfdec_decoder_enable_render(s);
	}else{
		swfdec_decoder_disable_render(s);
	}
#endif
	ret = swfdec_decoder_parse(s);
	if(ret==SWF_NEEDBITS){
		gtk_idle_remove(render_idle_id);
		render_idle_id = 0;
	}
	if(ret==SWF_EOF){
		swfdec_decoder_get_image(s,&image4);
		swfdec_decoder_free(s);
		s = NULL;
		if(quit_at_eof){
			if(image) g_free(image);
			if(image4) g_free(image4);
			gtk_exit(0);
		}
		gtk_idle_remove(render_idle_id);
		render_idle_id = 0;
	}
	if(ret==SWF_IMAGE){
		struct timeval now;

		swfdec_decoder_peek_image(s,&image4);
		pull_sound(s);

		if(!enable_sound){
			gettimeofday(&now, NULL);
			tv_add_usec(&image_time, interval);
			if(tv_compare(&image_time, &now) > 0){
				int x = tv_diff(&image_time, &now);
				//printf("sleeping for %d us\n",x);
				if(!fast)usleep(x);
			}else{
				gettimeofday(&image_time, NULL);
			}
		}else{
			while(sound_bytes>=10000){
				usleep(10000);
			}
		}

                if (image == NULL) {
                  image = malloc (width * height *3);
                }
                convert_image (image, image4, width, height);
		gdk_draw_rgb_image (drawing_area->window,
			drawing_area->style->black_gc, 
			0, 0, width, height, 
			GDK_RGB_DITHER_NONE,
			image,
			width*3);
	}
	if(ret==SWF_CHANGE && !plugged){
		double rate;

		swfdec_decoder_get_rate(s, &rate);
		interval = 1000000/rate;
		swfdec_decoder_get_image_size(s, &width, &height);
		gtk_window_resize(GTK_WINDOW(gtk_wind),
			width, height);
	}

	return TRUE;
}

void convert_image (unsigned char *dest, unsigned char *src, int width,
    int height)
{
  int i,j;

  for(j=0;j<height;j++){
    for(i=0;i<width;i++){
      dest[0] = src[0];
      dest[1] = src[1];
      dest[2] = src[2];
      dest += 3;
      src += 4;
    }
  }
}

extern volatile gboolean glib_on_error_halt;

void fault_handler(int signum, siginfo_t *si, void *misc)
{
	//int spinning = TRUE;

	fault_restore();

	if(si->si_signo == SIGSEGV){
		g_print ("Caught SIGSEGV accessing address %p\n", si->si_addr);
	}else if(si->si_signo == SIGQUIT){
		g_print ("Caught SIGQUIT\n");
	}else{
		g_print ("signo:  %d\n",si->si_signo);
		g_print ("errno:  %d\n",si->si_errno);
		g_print ("code:   %d\n",si->si_code);
	}

	glib_on_error_halt = FALSE;
	g_on_error_stack_trace("swf_play");

	wait(NULL);

	_exit(0);
}

void fault_restore(void)
{
	struct sigaction action;

	memset(&action,0,sizeof(action));
	action.sa_handler = SIG_DFL;

	sigaction(SIGSEGV, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);
}

void fault_setup(void)
{
	struct sigaction action;

	memset(&action,0,sizeof(action));
	action.sa_sigaction = fault_handler;
	action.sa_flags = SA_SIGINFO;

	sigaction(SIGSEGV, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);
}

