/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_VIDEO_DECODER_VP6_ALPHA_H_
#define _SWFDEC_VIDEO_DECODER_VP6_ALPHA_H_

#include <swfdec/swfdec_video_decoder.h>

G_BEGIN_DECLS


typedef struct _SwfdecVideoDecoderVp6Alpha SwfdecVideoDecoderVp6Alpha;
typedef struct _SwfdecVideoDecoderVp6AlphaClass SwfdecVideoDecoderVp6AlphaClass;

#define SWFDEC_TYPE_VIDEO_DECODER_VP6_ALPHA                    (swfdec_video_decoder_vp6_alpha_get_type())
#define SWFDEC_IS_VIDEO_DECODER_VP6_ALPHA(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_VIDEO_DECODER_VP6_ALPHA))
#define SWFDEC_IS_VIDEO_DECODER_VP6_ALPHA_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_VIDEO_DECODER_VP6_ALPHA))
#define SWFDEC_VIDEO_DECODER_VP6_ALPHA(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_VIDEO_DECODER_VP6_ALPHA, SwfdecVideoDecoderVp6Alpha))
#define SWFDEC_VIDEO_DECODER_VP6_ALPHA_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_VIDEO_DECODER_VP6_ALPHA, SwfdecVideoDecoderVp6AlphaClass))
#define SWFDEC_VIDEO_DECODER_VP6_ALPHA_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_VIDEO_DECODER_VP6_ALPHA, SwfdecVideoDecoderVp6AlphaClass))

struct _SwfdecVideoDecoderVp6Alpha
{
  SwfdecVideoDecoder		decoder;

  SwfdecVideoDecoder *		image;
  SwfdecVideoDecoder *		mask;
};

struct _SwfdecVideoDecoderVp6AlphaClass
{
  SwfdecVideoDecoderClass	decoder_class;
};

GType			swfdec_video_decoder_vp6_alpha_get_type	(void);


G_END_DECLS
#endif
