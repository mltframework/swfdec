/* vim: set sw=4: -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* GdkPixbuf library - SWF image loader
 *
 * Copyright (C) 2003 Dom Lachowicz
 *
 * Authors: Dom Lachowicz <cinamod@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more  * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "swfdec.h"
#include "io-swf-anim.h"

#include <stdlib.h>
#include <glib.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk-pixbuf/gdk-pixbuf-io.h>

#define SWF_BUFFER_SIZE (1024 * 8)

typedef struct {
	SwfdecDecoder             * decoder;
	GdkPixbufSwfAnim          * animation;
	gboolean                    single_frame;
	GdkPixbufModuleUpdatedFunc  updated_func;
	GdkPixbufModulePreparedFunc prepared_func;
	GdkPixbufModuleSizeFunc     size_func;
	gpointer                    user_data;
} SwfContext;

G_MODULE_EXPORT void fill_vtable (GdkPixbufModule *module);
G_MODULE_EXPORT void fill_info (GdkPixbufFormat *info);

/****************************************************************************/
/****************************************************************************/

static SwfContext *
swf_context_new (void)
{
	SwfContext *context    = g_new0 (SwfContext, 1);
	context->decoder       = swfdec_decoder_new ();
	context->animation     = g_object_new (GDK_TYPE_PIXBUF_SWF_ANIM, NULL);

	return context;
}

static SwfContext *
swf_context_new_full (GdkPixbufModuleSizeFunc size_func,
					  GdkPixbufModulePreparedFunc prepared_func, 
					  GdkPixbufModuleUpdatedFunc  updated_func,
					  gpointer user_data)
{
	SwfContext * context   = swf_context_new ();

	context->size_func     = size_func;
	context->prepared_func = prepared_func;
	context->updated_func  = updated_func;
	context->user_data     = user_data;

	return context;
}

static void
swf_context_free (SwfContext * context)
{
	swfdec_decoder_free (context->decoder);
	g_object_unref (G_OBJECT (context->animation));
	g_free (context);
}

static void
swf_gerror_build (int swf_err, GError ** error)
{
	const char * message = NULL;

	if (swf_err == SWF_OK)
		return;
	
	switch (swf_err) {
	case SWF_ERROR: 
	default: 
		message = "Unknown Error";     
		break;
	}

	if (error)
		*error = g_error_new (GDK_PIXBUF_ERROR, swf_err, message);
}

static void
swf_pixels_destroy (guchar * pixels, gpointer data)
{
	g_free (pixels);
}

static void
swf_animation_change (SwfContext * context)
{
	int nframes = 0;
	int width   = 0, height = 0;
	double rate = 0.;

	if (SWF_OK != swfdec_decoder_get_n_frames (context->decoder, &nframes))
		return;

	if (SWF_OK != swfdec_decoder_get_image_size (context->decoder, &width, 
												 &height))
		return;

	if (SWF_OK != swfdec_decoder_get_rate (context->decoder, &rate))
		return;

	context->animation->n_frames   = nframes;
	context->animation->width      = width;
	context->animation->height     = height;
	context->animation->rate       = (int)(rate);

	if (context->size_func) {
		(*context->size_func) (&width, &height, context->user_data);
		if (width > 0 && height > 0)
			swfdec_decoder_set_image_size (context->decoder, width, height);
	}
}

static void
swf_animation_image (SwfContext * context)
{
	int nframe = 0;
	guchar * pixels = NULL;
	GdkPixbufFrame * frame;

	if (swfdec_decoder_get_image (context->decoder, &pixels) != SWF_OK && pixels != NULL)
		return;

	nframe = g_list_length (context->animation->frames) + 1;	

	frame = g_new0 (GdkPixbufFrame, 1);
	frame->delay_time = (int)(1000 / context->animation->rate);
	frame->elapsed = (int)(frame->delay_time * nframe);
	context->animation->total_time += frame->elapsed;
	frame->pixbuf = gdk_pixbuf_new_from_data (pixels, GDK_COLORSPACE_RGB,
											  FALSE, 8, 
											  context->animation->width, context->animation->height,
											  context->animation->width * 3, swf_pixels_destroy,
											  NULL);

	context->animation->frames = g_list_append (context->animation->frames, frame);

	if (context->prepared_func != NULL)
		(* context->prepared_func) (frame->pixbuf, NULL, context->user_data);
	
	if (context->updated_func != NULL)
		(* context->updated_func) (frame->pixbuf, 
								   0, 0, 
								   gdk_pixbuf_get_width (frame->pixbuf), 
								   gdk_pixbuf_get_height (frame->pixbuf), 
								   context->user_data);
}

static int
swf_flush (SwfContext * context)
{
	int result = SWF_OK;

	/* only load and render a single frame if required, return SWF image to note that
	 * an image has been loaded */
	if (context->single_frame && g_list_length (context->animation->frames) == 1)
		return SWF_IMAGE;

	result = swfdec_decoder_parse (context->decoder);

	if (result == SWF_CHANGE)
		swf_animation_change (context);
	else if (result == SWF_IMAGE)
		swf_animation_image (context);
	else
		return SWF_IMAGE;
	
	return SWF_OK;
}

static int
swf_add_bits (SwfContext * context, const guint8 * buf, gsize nread)
{
	int result;

	/* only load and render a single frame if required, return SWF image to note that
	 * an image has been loaded */
	if (context->single_frame && g_list_length (context->animation->frames) == 1)
		return SWF_OK;	

	result = swfdec_decoder_addbits (context->decoder, (guint8*)buf, (int)nread);
	if (result == SWF_OK)
		swf_flush (context);

	return result;
}

/****************************************************************************/
/****************************************************************************/

static GdkPixbuf *
gdk_pixbuf__swf_image_load (FILE *file, GError **error)
{
	SwfContext * context = NULL;
	GdkPixbuf * pixbuf = NULL;
	gsize nread = 0;
	guint8 buf [SWF_BUFFER_SIZE];
	int result = SWF_OK;

	if (error)
		*error = NULL;

	g_return_val_if_fail (file != NULL, NULL);

	context = swf_context_new ();
	context->single_frame = TRUE;

	while ((nread = fread (buf, sizeof (guint8), sizeof (buf), file)) != 0 && result != SWF_ERROR)
		result = swf_add_bits (context, buf, nread);

	if (result != SWF_ERROR) {
		while (SWF_OK == swf_flush (context))
			;

		pixbuf = gdk_pixbuf_animation_get_static_image (GDK_PIXBUF_ANIMATION (context->animation));
		if (pixbuf)
			g_object_ref (G_OBJECT (pixbuf));
	} 
	else
		swf_gerror_build (result, error);

	swf_context_free (context);

	return pixbuf;
}

static GdkPixbufAnimation *
gdk_pixbuf__swf_image_load_animation (FILE *file,
                                      GError **error)
{
	SwfContext * context = NULL;
	GdkPixbufAnimation *animation = NULL;
	gsize nread = 0;
	guint8 buf [SWF_BUFFER_SIZE];
	int result = SWF_OK;

	if (error)
		*error = NULL;

	g_return_val_if_fail (file != NULL, NULL);

	context = swf_context_new ();

	while ((nread = fread (buf, sizeof (guint8), sizeof (buf), file)) != 0 && result != SWF_ERROR) 
		result = swf_add_bits (context, buf, nread);

	if (result == SWF_OK) {
		while (SWF_OK == swf_flush (context))
			;

		animation = GDK_PIXBUF_ANIMATION (context->animation);
		if (animation)
			g_object_ref (G_OBJECT (animation));
	}
	else
		swf_gerror_build (result, error);

	swf_context_free (context);

	return animation;
}

static gpointer
gdk_pixbuf__swf_image_begin_load (GdkPixbufModuleSizeFunc size_func,
                                  GdkPixbufModulePreparedFunc prepared_func, 
                                  GdkPixbufModuleUpdatedFunc  updated_func,
                                  gpointer user_data,
                                  GError **error)
{
	SwfContext * context = swf_context_new_full (size_func, prepared_func, 
												 updated_func, user_data);

	if (error)
		*error = NULL;

	return context;
}

static gboolean
gdk_pixbuf__swf_image_load_increment (gpointer data,
									  const guchar *buf, guint size,
									  GError **error)
{
	int result = SWF_OK;
	SwfContext *context = (SwfContext *)data;

	if (error)
		*error = NULL;
	result = swf_add_bits (context, buf, size);
	if (result == SWF_ERROR)
		swf_gerror_build (result, error);

	return (result != SWF_ERROR);
}

static gboolean
gdk_pixbuf__swf_image_stop_load (gpointer data, GError **error)
{
	SwfContext *context = (SwfContext *)data;  

	if (error)
		*error = NULL;

	while (SWF_OK == swf_flush (context))
		;

	swf_context_free (context);

	return TRUE;
}

/****************************************************************************/
/****************************************************************************/

void
fill_vtable (GdkPixbufModule *module)
{
	module->load           = gdk_pixbuf__swf_image_load;
	module->begin_load     = gdk_pixbuf__swf_image_begin_load;
	module->stop_load      = gdk_pixbuf__swf_image_stop_load;
	module->load_increment = gdk_pixbuf__swf_image_load_increment;
	module->load_animation = gdk_pixbuf__swf_image_load_animation;
}

void
fill_info (GdkPixbufFormat *info)
{
	static GdkPixbufModulePattern signature[] = {
		{ "FWS", NULL, 100 },
		{ "CWS", NULL, 100 },
		{ NULL, NULL, 0 }
	};
	static gchar *mime_types[] = { 
		"application/x-shockwave-flash", 
		"image/vnd.rn-realflash",
		NULL 
	};
	static gchar *extensions[] = { 
		"swf", 
		NULL 
	};   

	info->name        = "swf";
	info->signature   = signature;
	info->description = "Shockwave Flash";
	info->mime_types  = mime_types;
	info->extensions  = extensions;
	info->flags       = 0;
}
