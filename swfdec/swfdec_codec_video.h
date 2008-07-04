/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_CODEC_VIDEO_H_
#define _SWFDEC_CODEC_VIDEO_H_

#include <glib.h>
#include <cairo.h>
#include <swfdec/swfdec_buffer.h>

G_BEGIN_DECLS

#define SWFDEC_VIDEO_CODEC_UNDEFINED 0
#define SWFDEC_VIDEO_CODEC_H263 2
#define SWFDEC_VIDEO_CODEC_SCREEN 3
#define SWFDEC_VIDEO_CODEC_VP6 4
#define SWFDEC_VIDEO_CODEC_VP6_ALPHA 5
#define SWFDEC_VIDEO_CODEC_SCREEN2 6

typedef enum {
  SWFDEC_VIDEO_FORMAT_RGBA,
  SWFDEC_VIDEO_FORMAT_I420
} SwfdecVideoFormat;

typedef struct {
  guint			width;	      	/* width of image in pixels */
  guint			height;	    	/* height of image in pixels */
  const guint8 *	plane[3];	/* planes of the image, not all might be used */
  const guint8 *	mask;		/* A8 mask or NULL if none */
  guint		  	rowstride[3];	/* rowstrides of the planes */
  guint			mask_rowstride;	/* rowstride of mask plane */
} SwfdecVideoImage;

typedef struct _SwfdecVideoDecoder SwfdecVideoDecoder;
typedef SwfdecVideoDecoder * (SwfdecVideoDecoderNewFunc) (guint format);

/* notes about the decode function:
 * - the data must be in the format specified by swfdec_video_codec_get_format()
 * - the data returned in the image belongs to the decoder and must be valid 
 *   until the next function is called on the decoder.
 * - you need to explicitly set mask to %NULL.
 */
struct _SwfdecVideoDecoder {
  guint			codec;
  gboolean		(* decode)	(SwfdecVideoDecoder *	decoder,
					 SwfdecBuffer *		buffer,
					 SwfdecVideoImage *	result);
  void			(* free)	(SwfdecVideoDecoder *	decoder);
};

SwfdecVideoFormat     	swfdec_video_codec_get_format	(guint			codec);

char *			swfdec_video_decoder_prepare	(guint			codec);
SwfdecVideoDecoder *	swfdec_video_decoder_new      	(guint			codec);
void			swfdec_video_decoder_free	(SwfdecVideoDecoder *   decoder);

cairo_surface_t *     	swfdec_video_decoder_decode	(SwfdecVideoDecoder *	decoder,
							 SwfdecBuffer *		buffer);


G_END_DECLS
#endif
