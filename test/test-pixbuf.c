/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*-
 *   gcc `pkg-config --cflags --libs gtk+-2.0` -o test-pixbuf test-pixbuf.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <gtk/gtk.h>

static void
quit_cb (GtkWidget *win, gpointer unused)
{
	/* exit the main loop */
	gtk_main_quit();
}

static void
view_pixbuf (const char * file)
{
	GtkWidget *win, *img;
	
	/* create toplevel window and set its title */
	win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title (GTK_WINDOW(win), "Pixbuf Viewer");
	
	/* exit when 'X' is clicked */
	g_signal_connect(G_OBJECT(win), "destroy", G_CALLBACK(quit_cb), NULL);
	
	img = gtk_image_new_from_file (file);

	/* pack the window with the image */
	gtk_container_add(GTK_CONTAINER(win), img);
	gtk_widget_show_all (win);
}

int 
main (int argc, char **argv)
{
	/* initialize gtk+ */
	gtk_init (&argc, &argv) ;
	
	view_pixbuf (argv[1]);
	
	/* run the gtk+ main loop */
	gtk_main ();
	
	return 0;
}
