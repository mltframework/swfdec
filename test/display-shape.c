#include <gtk/gtk.h>
#include <libart_lgpl/libart.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "swf.h"


swf_state_t *s;

static void build_widget (unsigned char *buffer);

static void destroy_cb (GtkWidget *widget, gpointer data);
static int expose_cb (GtkWidget *widget, GdkEventExpose *evt, gpointer data);


static int key_press (GtkWidget *widget, GdkEventKey *evt, gpointer data);

static int 
expose_cb (GtkWidget *widget, GdkEventExpose *evt, gpointer data)
{
  //art_u8 *buf = (art_u8 *)data;

	if(s->buffer)
  gdk_draw_rgb_image (widget->window, widget->style->black_gc, 
		      0, 0, s->width, s->height, 
		      GDK_RGB_DITHER_NONE,
		      s->buffer,
		      s->width*3);
  return FALSE;
}

static void 
destroy_cb (GtkWidget *widget, gpointer data)
{
  gtk_main_quit ();
}

GtkWidget *window = NULL, *drawing_area = NULL;

static void 
build_widget (unsigned char *buffer)
{

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_default_size (GTK_WINDOW(window), s->width, s->height);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (destroy_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (destroy_cb), NULL);
  drawing_area = gtk_drawing_area_new ();
  gtk_container_add (GTK_CONTAINER (window),
		     GTK_WIDGET (drawing_area));

  gtk_signal_connect (GTK_OBJECT (drawing_area), "expose_event",
		      GTK_SIGNAL_FUNC (expose_cb), buffer);
  gtk_signal_connect (GTK_OBJECT (drawing_area), "configure_event",
		      GTK_SIGNAL_FUNC (expose_cb), buffer);

  gtk_signal_connect (GTK_OBJECT (window), "key_press_event",
		      GTK_SIGNAL_FUNC (key_press), buffer);

  gtk_widget_show_all (window);
}


void read_swf_file(char *fn);

int main (int argc, char *argv[])
{
  char *fn = "it.swf";

  /* gtk/gdkrgb initialization */
  gtk_init (&argc, &argv);
  gdk_rgb_init ();
  gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
  gtk_widget_set_default_visual(gdk_rgb_get_visual());

  if(argc>=2){
	fn = argv[1];
  }

  read_swf_file(fn);

  build_widget (s->buffer);


  /* gtk main loop */
  gtk_main ();

  return 0;
}



unsigned char *data;
int len;

void read_swf_file(char *fn)
{
	struct stat sb;
	int fd;
	int ret;

	s = swf_init();

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
	fprintf(stderr,"swf_addbits returned %d\n",ret);
}


static int key_press (GtkWidget *widget, GdkEventKey *evt, gpointer data)
{
	int ret;

	//fprintf(stderr,"key press\n");

	ret = swf_parse(s);
	//fprintf(stderr,"swf_addbits returned %d\n",ret);

	if(ret==SWF_IMAGE){
		gdk_draw_rgb_image (drawing_area->window,
			drawing_area->style->black_gc, 
			0, 0, s->width, s->height, 
			GDK_RGB_DITHER_NONE,
			s->buffer,
			s->width*3);
	}

	if(ret==SWF_EOF){
		gtk_main_quit();
	}

	return FALSE;
}

