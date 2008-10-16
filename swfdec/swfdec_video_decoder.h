/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef _SWFDEC_VIDEO_DECODER_H_
#define _SWFDEC_VIDEO_DECODER_H_

#include <swfdec/swfdec_buffer.h>
#include <swfdec/swfdec_renderer.h>

G_BEGIN_DECLS


#define SWFDEC_VIDEO_CODEC_UNDEFINED 0
#define SWFDEC_VIDEO_CODEC_H263 2
#define SWFDEC_VIDEO_CODEC_SCREEN 3
#define SWFDEC_VIDEO_CODEC_VP6 4
#define SWFDEC_VIDEO_CODEC_VP6_ALPHA 5
#define SWFDEC_VIDEO_CODEC_SCREEN2 6
#define SWFDEC_VIDEO_CODEC_H264 7

typedef enum {
  SWFDEC_VIDEO_FORMAT_RGBA,
  SWFDEC_VIDEO_FORMAT_I420
} SwfdecVideoFormat;


typedef struct _SwfdecVideoDecoder SwfdecVideoDecoder;
typedef struct _SwfdecVideoDecoderClass SwfdecVideoDecoderClass;

#define SWFDEC_TYPE_VIDEO_DECODER                    (swfdec_video_decoder_get_type())
#define SWFDEC_IS_VIDEO_DECODER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_VIDEO_DECODER))
#define SWFDEC_IS_VIDEO_DECODER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_VIDEO_DECODER))
#define SWFDEC_VIDEO_DECODER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_VIDEO_DECODER, SwfdecVideoDecoder))
#define SWFDEC_VIDEO_DECODER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_VIDEO_DECODER, SwfdecVideoDecoderClass))
#define SWFDEC_VIDEO_DECODER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_VIDEO_DECODER, SwfdecVideoDecoderClass))

struct _SwfdecVideoDecoder
{
  GObject		object;

  /*< protected >*/
  guint			width;		/* width of the current image */
  guint			height;		/* height of the current image */
  guint8 *		plane[3];	/* planes of the image, not all might be used */
  guint		  	rowstride[3];	/* rowstrides of the planes */
  guint8 *		mask;		/* A8 mask or NULL if none */
  guint			mask_rowstride;	/* rowstride of mask plane */
  /*< private >*/
  guint			codec;		/* codec this decoder uses */
  gboolean		error;		/* if this codec is in an error state */
};

struct _SwfdecVideoDecoderClass
{
  /*< private >*/
  GObjectClass		object_class;

  /*< public >*/
  gboolean		(* prepare)	(guint                  codec,
					 char **                missing);
  SwfdecVideoDecoder *	(* create)	(guint                  codec,
					 SwfdecBuffer *		data);

  void			(* decode)	(SwfdecVideoDecoder *	decoder,
					 SwfdecBuffer *		buffer);
};

SwfdecVideoFormat     	swfdec_video_codec_get_format	(guint			codec);

GType			swfdec_video_decoder_get_type	(void);

void			swfdec_video_decoder_register	(GType			type);

gboolean		swfdec_video_decoder_prepare	(guint			codec,
							 char **		missing);

SwfdecVideoDecoder *   	swfdec_video_decoder_new      	(guint			codec,
							 SwfdecBuffer *		buffer);

void			swfdec_video_decoder_decode	(SwfdecVideoDecoder *	decoder,
							 SwfdecBuffer *		buffer);
guint			swfdec_video_decoder_get_codec	(SwfdecVideoDecoder *	decoder);
guint			swfdec_video_decoder_get_width	(SwfdecVideoDecoder *	decoder);
guint			swfdec_video_decoder_get_height	(SwfdecVideoDecoder *	decoder);
cairo_surface_t *	swfdec_video_decoder_get_image	(SwfdecVideoDecoder *	decoder,
							 SwfdecRenderer *	renderer);
gboolean		swfdec_video_decoder_get_error	(SwfdecVideoDecoder *   decoder);

/* for subclasses */
void			swfdec_video_decoder_error	(SwfdecVideoDecoder *	decoder,
							 const char *		error,
							 ...) G_GNUC_PRINTF (2, 3);
void			swfdec_video_decoder_errorv	(SwfdecVideoDecoder *	decoder,
							 const char *		error,
							 va_list		args);



G_END_DECLS
#endif
