
#include <swf.h>

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

swf_state_t *s;

GtkWidget *drawing_area;
GtkWidget *gtk_wind;
int plugged;
GdkWindow *gdk_parent;

GIOChannel *input_chan;

guint input_idle_id;
guint render_idle_id;

unsigned long xid;
int width = 100;
int height = 100;

static void do_help(void);
static void read_swf_file(char *fn);
static void read_swf_stdin(void);
static void new_gtk_window(void);
static void steal_window(void);

/* GTK callbacks */
static void destroy_cb (GtkWidget *widget, gpointer data);
static int expose_cb (GtkWidget *widget, GdkEventExpose *evt, gpointer data);
static int key_press (GtkWidget *widget, GdkEventKey *evt, gpointer data);
static void embedded (GtkPlug *plug, gpointer data);

/* GIOChan callbacks */
static gboolean input(GIOChannel *chan, GIOCondition cond, gpointer ignored);
static gboolean render_idle(gpointer data);


int main(int argc, char *argv[])
{
	int c;
	int index;
	static struct option options[] = {
		{ "xid", 1, NULL, 'x' },
		{ "width", 1, NULL, 'w' },
		{ "height", 1, NULL, 'h' },
		{ 0 },
	};

	gtk_init(&argc,&argv);
	gdk_rgb_init ();
	gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
	gtk_widget_set_default_visual(gdk_rgb_get_visual());

	s = swf_init();

	while(1){
		c = getopt_long(argc, argv, "x:", options, &index);
		if(c==-1)break;

		switch(c){
		case 'x':
			xid = strtoul(optarg, NULL, 0);
			break;
		case 'w':
			width = strtoul(optarg, NULL, 0);
			break;
		case 'h':
			height = strtoul(optarg, NULL, 0);
			break;
		default:
			do_help();
			break;
		}
	}

	if(optind != argc-1)do_help();

	if(strcmp(argv[optind],"-")==0){
		read_swf_stdin();
	}else{
		read_swf_file(argv[optind]);
	}

	if(xid){
		plugged = 1;
	}
	new_gtk_window();

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
	//GdkWindow *gdk_wind;

	if(plugged){
		gtk_wind = gtk_plug_new(0);
		gtk_signal_connect(GTK_OBJECT(gtk_wind), "embedded",
			GTK_SIGNAL_FUNC(embedded), NULL);
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

	gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		GTK_SIGNAL_FUNC(expose_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (drawing_area), "configure_event",
		GTK_SIGNAL_FUNC(expose_cb), NULL);

	gtk_signal_connect(GTK_OBJECT(gtk_wind), "key_press_event",
		GTK_SIGNAL_FUNC(key_press), NULL);

	gdk_parent = gdk_window_foreign_new(xid);

	gtk_widget_show_all(gtk_wind);

	if(plugged){
		XReparentWindow(GDK_WINDOW_XDISPLAY(gtk_wind->window),
			GDK_WINDOW_XID(gtk_wind->window),
			xid, 0, 0);
#if 0
		XSync(GDK_WINDOW_XDISPLAY(gtk_wind->window),0);
#endif
		XMapWindow(GDK_WINDOW_XDISPLAY(gtk_wind->window),
			GDK_WINDOW_XID(gtk_wind->window));
		gtk_widget_add_events(gtk_wind, GDK_ALL_EVENTS_MASK);
	}
}

#if 0
static void steal_window(void)
{
	gtk_wind = gtk_plug_new(xid);

	if(!gtk_wind){
		fprintf(stderr,"Can't find window with XID %ld\n",xid);
		exit(1);
	}

	gtk_signal_connect(GTK_OBJECT(gtk_wind), "delete_event",
		GTK_SIGNAL_FUNC (destroy_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(gtk_wind), "destroy",
		GTK_SIGNAL_FUNC(destroy_cb), NULL);

	drawing_area = gtk_drawing_area_new();
	gtk_container_add(GTK_CONTAINER(gtk_wind),
		GTK_WIDGET(drawing_area));

	gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		GTK_SIGNAL_FUNC(expose_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (drawing_area), "configure_event",
		GTK_SIGNAL_FUNC(expose_cb), NULL);

	gtk_signal_connect(GTK_OBJECT(drawing_area), "key_press_event",
		GTK_SIGNAL_FUNC(key_press), NULL);

	gtk_widget_show_all(gtk_wind);
}
#endif

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

	ret = swf_addbits(s,data,len);
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

/* GTK callbacks */

static int key_press (GtkWidget *widget, GdkEventKey *evt, gpointer data)
{
	int ret;

	//fprintf(stderr,"key press\n");
	
	return FALSE;
}

static int expose_cb (GtkWidget *widget, GdkEventExpose *evt, gpointer data)
{
	if(s->buffer){
		gdk_draw_rgb_image (widget->window, widget->style->black_gc, 
			0, 0, s->width, s->height, 
			GDK_RGB_DITHER_NONE,
			s->buffer,
			s->width*3);
	}

	return FALSE;
}

static void destroy_cb (GtkWidget *widget, gpointer data)
{
	gtk_main_quit ();
}

static void embedded (GtkPlug *plug, gpointer data)
{
	//printf("EMBEDDED!\n");
	//gtk_widget_show_all(gtk_wind);
}

/* idle stuff */

static gboolean input(GIOChannel *chan, GIOCondition cond, gpointer ignored)
{
	char *data;
	int bytes_read;
	GError *error = NULL;
	int ret;

	data = malloc(4096);
	ret = g_io_channel_read_chars(chan, data, 4096, &bytes_read, &error);
	if(ret==G_IO_STATUS_NORMAL){
		ret = swf_addbits(s,data,bytes_read);
		//fprintf(stderr,"addbits %d\n",bytes_read);
		if(!render_idle_id)render_idle_id = g_idle_add(render_idle,NULL);
	}else if(ret==G_IO_STATUS_ERROR){
		fprintf(stderr,"g_io_channel_read_chars: %s\n",error->message);
		exit(1);
	}else if(ret==G_IO_STATUS_EOF){
		fprintf(stderr,"got eof\n");
		gtk_idle_remove(input_idle_id);
		if(!render_idle_id)render_idle_id = g_idle_add(render_idle,NULL);
	}

	free(data);

	return TRUE;
}

static gboolean render_idle(gpointer data)
{
	int ret;

	ret = swf_parse(s);
	if(ret==SWF_NEEDBITS || ret==SWF_EOF){
		gtk_idle_remove(render_idle_id);
		render_idle_id = 0;
	}
	if(ret==SWF_IMAGE){
		gdk_draw_rgb_image (drawing_area->window,
			drawing_area->style->black_gc, 
			0, 0, s->width, s->height, 
			GDK_RGB_DITHER_NONE,
			s->buffer,
			s->width*3);
	}
	if(ret==SWF_CHANGE){
		gtk_window_resize(GTK_WINDOW(gtk_wind),
			s->width, s->height);
	}

	return TRUE;
}

