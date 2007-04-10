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
#include <libswfdec/swfdec_buffer.h>

typedef enum {
  SWFDEC_VIDEO_FORMAT_UNDEFINED = 0,
  SWFDEC_VIDEO_FORMAT_H263 = 2,
  SWFDEC_VIDEO_FORMAT_SCREEN = 3,
  SWFDEC_VIDEO_FORMAT_VP6 = 4,
  SWFDEC_VIDEO_FORMAT_VP6_ALPHA = 5,
  SWFDEC_VIDEO_FORMAT_SCREEN2 = 6
} SwfdecVideoFormat;

typedef struct _SwfdecVideoDecoder SwfdecVideoDecoder;
typedef SwfdecVideoDecoder * (SwfdecVideoDecoderNewFunc) (SwfdecVideoFormat format);

struct _SwfdecVideoDecoder {
  SwfdecVideoFormat	format;
  SwfdecBuffer *	(* decode)	(SwfdecVideoDecoder *	decoder,
					 SwfdecBuffer *		buffer,
					 guint *		width,
					 guint *		height,
					 guint *		rowstride);
  void			(* free)	(SwfdecVideoDecoder *	decoder);
};

SwfdecVideoDecoder *	swfdec_video_decoder_new      	(SwfdecVideoFormat	format);
void			swfdec_video_decoder_free	(SwfdecVideoDecoder *   decoder);

cairo_surface_t *     	swfdec_video_decoder_decode	(SwfdecVideoDecoder *	decoder,
							 SwfdecBuffer *		buffer);


G_END_DECLS
#endif
