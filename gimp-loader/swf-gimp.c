/* -*- Mode: C; tab-width: 8; c-basic-offset: 8 -*- */

/* The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* SWF loading file filter for the GIMP
 * -Dom Lachowicz <cinamod@hotmail.com> 2003
 *
 * TODO: 
 * *) image preview
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "swfdec.h"

#include <gtk/gtk.h>
#include <libgimp/gimp.h>
#include <libgimp/gimpui.h>

#define SWF_BUFFER_SIZE (1024 * 4)

typedef struct
{
	int frame;
	int width;
	int height;
} SwfLoadVals;

static SwfLoadVals load_vals =
{
	1,
	-1,
	-1
};

typedef struct
{
	GtkWidget *dialog;
	GtkObject *frame;
	GtkObject *width_adj;
	GtkObject *height_adj;
} LoadDialogVals;

/* TODO: remove me, initialize gimp i18n services */
#ifndef _
#define _(String) (String)
#endif

static void query (void);
static void run   (const gchar      *name,
		   gint              nparams,
		   const GimpParam  *param,
		   gint             *nreturn_vals,
		   GimpParam       **return_vals);

GimpPlugInInfo PLUG_IN_INFO = {
	NULL,                         /* init_proc  */
	NULL,                         /* quit_proc  */
	query,                        /* query_proc */
	run,                          /* run_proc   */
};

MAIN ()

static void
check_load_vals (void)
{
	/* which frame to load */
	if (load_vals.frame < 1)
		load_vals.frame = 1;

	if (load_vals.width < 0)
		load_vals.width = -1;

	if (load_vals.height < 0)
		load_vals.height = -1;
}

typedef struct
{
  gint run;
} SwfLoadInterface;

static SwfLoadInterface load_interface =
{
  FALSE
};

static void
load_ok_callback (GtkWidget *widget,
                  gpointer   data)

{
	LoadDialogVals *vals = (LoadDialogVals *)data;
	
	if (vals->frame)
		load_vals.frame = (int)gtk_adjustment_get_value (GTK_ADJUSTMENT (vals->frame));

	load_vals.width = (int)gtk_adjustment_get_value (GTK_ADJUSTMENT (vals->width_adj));
	load_vals.height = (int)gtk_adjustment_get_value (GTK_ADJUSTMENT (vals->height_adj));
	
	load_interface.run = TRUE;
	gtk_widget_destroy (GTK_WIDGET (vals->dialog));
}

static gint
load_dialog (const gchar *file_name)
{
	LoadDialogVals *vals;
	GtkWidget *frame;
	GtkWidget *vbox;
	GtkWidget *table;
	GtkWidget *label;
	GtkWidget *adj;

	int nframes, width, height;

	{
		/* quickly get the # of frames without doing any real processing */
		SwfdecDecoder * decoder = NULL;
		FILE * file;
		int got_nframes = 0, nread, result = SWF_OK;
		char buf[SWF_BUFFER_SIZE];
		
		file = fopen (file_name, "rb");
		if (!file)
			return 0;
		
		decoder = swfdec_decoder_new ();
		
		while ((nread = fread (buf, sizeof (guint8), sizeof (buf), file)) != 0 && result != SWF_ERROR && !got_nframes)
			if (SWF_OK == swfdec_decoder_addbits (decoder, (guint8*)buf, (int)nread))
				if (SWF_CHANGE == swfdec_decoder_parse (decoder)) {
					got_nframes = (swfdec_decoder_get_n_frames (decoder, &nframes) == SWF_OK);
					swfdec_decoder_get_image_size (decoder, &width, &height);
				}
		fclose (file);
		swfdec_decoder_free (decoder);

		if (!got_nframes)
			return 0;
	}
	
	gimp_ui_init ("swf", FALSE);
	
	vals = g_new0 (LoadDialogVals, 1);
	
	vals->dialog = gimp_dialog_new (_("Load Shockwave Flash Image"), "swf",
					NULL, NULL, /* gimp_standard_help_func, "swf, */
					GTK_WIN_POS_MOUSE,
					FALSE, TRUE, FALSE,
					GTK_STOCK_CANCEL, gtk_widget_destroy,
					NULL, 1, NULL, FALSE, TRUE,
					GTK_STOCK_OK, load_ok_callback,
					vals, NULL, NULL, TRUE, FALSE,
					NULL);
	
	g_signal_connect (vals->dialog, "destroy",
			  G_CALLBACK (gtk_main_quit),
			  NULL);
	
	frame = gtk_frame_new (g_strdup_printf (_("Rendering %s"), file_name));
	gtk_container_set_border_width (GTK_CONTAINER (frame), 6);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (vals->dialog)->vbox), frame,
			    TRUE, TRUE, 0);
	
	vbox = gtk_vbox_new (FALSE, 4);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 4);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	
	table = gtk_table_new (nframes == 1 ? 2 : 3, 
			       nframes == 1 ? 2 : 3, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 2);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);
	gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);
	gtk_widget_show (table);
	
	if (nframes > 1) {
		vals->frame = gimp_scale_entry_new (GTK_TABLE (table), 0, 1,
						    _("Frame:"), -1, -1,
						    load_vals.frame, 1, nframes,
						    1., 0.5, 0,
						    TRUE, 0.0, 0.0,
						    NULL, NULL);
	}

	label = gtk_label_new (_("Width:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, nframes == 1 ? 1 : 2, 
			  nframes == 1 ? 2 : 3,
			  GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (label);	
	adj = gimp_spin_button_new (&vals->width_adj, width, 1, width * 4,
				    1.0, 0.5, 0.5, 1.0, 1);
	gtk_table_attach (GTK_TABLE (table), adj, 1, 2, nframes == 1 ? 1 : 2, 
			  nframes == 1 ? 2 : 3,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show (adj);

	label = gtk_label_new (_("Height:"));
	gtk_misc_set_alignment (GTK_MISC (label), 1.0, 1.0);
	gtk_table_attach (GTK_TABLE (table), label, 0, 1, nframes == 1 ? 2 : 3, 
			  nframes == 1 ? 3 : 4,
			  GTK_FILL, GTK_FILL, 0, 0);
	gtk_widget_show (label);	
	adj = gimp_spin_button_new (&vals->height_adj, height, 1, height * 4,
				    1.0, 0.5, 0.5, 1.0, 1);
	gtk_table_attach (GTK_TABLE (table), adj, 1, 2, nframes == 1 ? 2 : 3, 
			  nframes == 1 ? 3 : 4,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show (adj);
	
	gtk_widget_show (vbox);
	gtk_widget_show (frame);
	
	gtk_widget_show (vals->dialog);
	
	gtk_main ();
	gdk_flush ();
	
	g_free (vals);	

	return load_interface.run;
}

static void
swf_animation_change (SwfdecDecoder * context)
{
	int nframes = 0;

	if (SWF_OK != swfdec_decoder_get_n_frames (context, &nframes))
		return;

	if (load_vals.frame > nframes)
		load_vals.frame = nframes;
}

static int
swf_flush (SwfdecDecoder * context, guint32 * nb_frames, guchar ** out_pixels, gint * width, gint * height)
{
	int result = SWF_OK;

	result = swfdec_decoder_parse (context);

	if (result == SWF_CHANGE)
		swf_animation_change (context);
	else if (result == SWF_IMAGE) {
		guchar * swf_pixels = NULL;

		*nb_frames = *nb_frames + 1;

		if (*nb_frames == load_vals.frame) {
			if (swfdec_decoder_get_image (context, &swf_pixels) == SWF_OK && swf_pixels) {
				if (swfdec_decoder_get_image_size (context, width, height) == SWF_OK) {
					*out_pixels = g_new (guchar, *width * *height * 3);
					memcpy (*out_pixels, swf_pixels, *width * *height * 3);
				}
			}
		}
	}
	else
		return SWF_IMAGE;
	
	return SWF_OK;
}

static int
swf_add_bits (SwfdecDecoder * context, const guint8 * buf, gsize nread, guint32 * nb_frames, guchar ** out_pixels,
	      gint * width, gint * height)
{
	int result = SWF_OK;

	result = swfdec_decoder_addbits (context, (guint8*)buf, (int)nread);
	if (result == SWF_OK)
		swf_flush (context, nb_frames, out_pixels, width, height);

	return result;
}

static guchar *
swf_load_file (const char * filename, gint * width, gint * height)
{
	SwfdecDecoder * decoder = NULL;
	gsize nread = 0;
	guint8 buf [SWF_BUFFER_SIZE];
	int result = SWF_OK, nb_frames = 0;
	guchar *out_pixels = NULL;
	FILE * file;

	*width = *height = 0;

	g_return_val_if_fail (filename != NULL, NULL);

	file = fopen (filename, "rb");
	if (!file)
		return NULL;

	decoder = swfdec_decoder_new ();

	if (load_vals.width != -1 && load_vals.height != -1)
		swfdec_decoder_set_image_size (decoder, load_vals.width, load_vals.height);

	while ((nread = fread (buf, sizeof (guint8), sizeof (buf), file)) != 0 && result != SWF_ERROR)
		result = swf_add_bits (decoder, buf, nread, &nb_frames, &out_pixels, width, height);

	if (result != SWF_ERROR) {
		while (SWF_OK == swf_flush (decoder, &nb_frames, &out_pixels, width, height))
			;
	}	

	fclose (file);
	swfdec_decoder_free (decoder);

	return out_pixels;
}

/*
 * 'load_image()' - Load a SWF image into a new image window.
 */
static gint32
load_image (const gchar  *filename, 	/* I - File to load */ 
	    GimpRunMode  runmode, 
	    gboolean     preview)
{
	gint32        image;	        /* Image */
	gint32	      layer;		/* Layer */
	GimpDrawable *drawable;	        /* Drawable for layer */
	GimpPixelRgn  pixel_rgn;	/* Pixel region for layer */
	gchar        *status;
	gint          i, rowstride;

	guchar * pixels, * buf;
	gint width, height;
	
	pixels = buf = swf_load_file (filename, &width, &height);
	if (!pixels)
		return -1;

	rowstride = width * 3;

	if (!pixels)
		{
			g_message (_("Can't open '%s'\n"), filename);
			gimp_quit ();
		}
	
	status = g_strdup_printf (_("Loading %s:"), filename);
	gimp_progress_init (status);
	g_free (status);
	
	image = gimp_image_new (width, height, GIMP_RGB);
	
	if (image == -1)
		{
			g_message ("Can't allocate new image: %s\n", filename);
			gimp_quit ();
		}
	
	gimp_image_set_filename (image, filename);
	
	layer = gimp_layer_new (image, _("Rendered SWF"),
				width, height, GIMP_RGB_IMAGE, 100, GIMP_NORMAL_MODE);
	
	drawable = gimp_drawable_get (layer);

	gimp_tile_cache_ntiles ((width / gimp_tile_width ()) + 1);

	gimp_pixel_rgn_init (&pixel_rgn, drawable, 0, 0,
			     drawable->width, drawable->height, TRUE, FALSE);
	
	for (i = 0; i < height; i++)
		{
			gimp_pixel_rgn_set_row (&pixel_rgn, buf, 0, i, width);
			buf += rowstride;
			
			if (i % 100 == 0)
				gimp_progress_update ((gdouble) i / (gdouble) height);
		}

	g_free (pixels);

	gimp_drawable_detach (drawable);
	
	gimp_progress_update (1.0);
	
	/* Tell the GIMP to display the image.
	 */
	gimp_image_add_layer (image, layer, 0);
	gimp_drawable_flush (drawable);
	
	return image;
}

/*
 * 'query()' - Respond to a plug-in query...
 */
static void
query (void)
{
	static GimpParamDef load_args[] =
		{
			{ GIMP_PDB_INT32,    "run_mode",     "Interactive, non-interactive" },
			{ GIMP_PDB_STRING,   "filename",     "The name of the file to load" },
			{ GIMP_PDB_STRING,   "raw_filename", "The name of the file to load" }
		};

	static GimpParamDef load_return_vals[] =
		{
			{ GIMP_PDB_IMAGE,   "image",         "Output image" }
		};

	static GimpParamDef load_setargs_args[] =
		{
			{ GIMP_PDB_INT32, "frame", "Which frame to load" },
			{ GIMP_PDB_INT32, "width", "Image's width" },
			{ GIMP_PDB_INT32, "heigt", "Image's height" }
		};
	
	gimp_install_procedure ("file_swf_load",
				"Loads files in the SWF file format",
				"Loads files in the SWF file format",
				"Dom Lachowicz <cinamod@hotmail.com>",
				"Dom Lachowicz <cinamod@hotmail.com>",
				"(c) 2003",
				"<Load>/SWF",
				NULL,
				GIMP_PLUGIN,
				G_N_ELEMENTS (load_args),
				G_N_ELEMENTS (load_return_vals),
				load_args, load_return_vals);

	gimp_install_procedure ("file_swf_load_setargs",
				"set additional parameters for the procedure file_swf_load",
				"set additional parameters for the procedure file_swf_load",
				"Dom Lachowicz <cinamod@hotmail.com>",
				"Dom Lachowicz",
				"2003",
				NULL,
				NULL,
				GIMP_PLUGIN,
				G_N_ELEMENTS (load_setargs_args), 0,
				load_setargs_args, NULL);
	
	gimp_register_magic_load_handler ("file_swf_load",
					  "swf", "",
					  "0,string,FWS,0,string,CWS");
}

/*
 * 'run()' - Run the plug-in...
 */
static void
run (const gchar      *name,
     gint        nparams,
     const GimpParam  *param,
     gint       *nreturn_vals,
     GimpParam **return_vals)
{
	static GimpParam     values[2];
	GimpRunMode          run_mode;
	GimpPDBStatusType    status = GIMP_PDB_SUCCESS;
	gint32               image_ID;
	
	run_mode = param[0].data.d_int32;
	
	*nreturn_vals = 1;
	*return_vals  = values;
	
	values[0].type          = GIMP_PDB_STATUS;
	values[0].data.d_status = GIMP_PDB_EXECUTION_ERROR;
	
	if (strcmp (name, "file_swf_load") == 0)
		{
			gimp_get_data ("file_swf_load", &load_vals);
			switch (run_mode)
				{
				case GIMP_RUN_INTERACTIVE:
					if (!load_dialog (param[1].data.d_string))
					   status = GIMP_PDB_CANCEL;
					break;
					
				case GIMP_RUN_NONINTERACTIVE:
					if (nparams > 3)
						load_vals.frame = param[3].data.d_int32;
					if (nparams > 4)
						load_vals.width = param[4].data.d_int32;
					if (nparams > 5)
						load_vals.height = param[5].data.d_int32;
					break;
					
				case GIMP_RUN_WITH_LAST_VALS:
					
				default:
					break;
				}
			
			if (status == GIMP_PDB_SUCCESS)
				{
					check_load_vals ();
					
					image_ID = load_image (param[1].data.d_string, run_mode, FALSE);
					gimp_set_data ("file_swf_load", &load_vals, sizeof (load_vals));
					
					if (image_ID != -1)
						{
							*nreturn_vals = 2;
							values[1].type         = GIMP_PDB_IMAGE;
							values[1].data.d_image = image_ID;
						}
					else
						status = GIMP_PDB_EXECUTION_ERROR;
				}
		}
	else
		status = GIMP_PDB_CALLING_ERROR;
	
	values[0].data.d_status = status;
}
