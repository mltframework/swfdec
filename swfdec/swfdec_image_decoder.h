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

#ifndef __SWFDEC_IMAGE_DECODER_H__
#define __SWFDEC_IMAGE_DECODER_H__

#include <glib.h>

#include <swfdec/swfdec_decoder.h>
#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_rect.h>

G_BEGIN_DECLS

typedef struct _SwfdecImageDecoder SwfdecImageDecoder;
typedef struct _SwfdecImageDecoderClass SwfdecImageDecoderClass;

#define SWFDEC_TYPE_IMAGE_DECODER                    (swfdec_image_decoder_get_type())
#define SWFDEC_IS_IMAGE_DECODER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_IMAGE_DECODER))
#define SWFDEC_IS_IMAGE_DECODER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_IMAGE_DECODER))
#define SWFDEC_IMAGE_DECODER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_IMAGE_DECODER, SwfdecImageDecoder))
#define SWFDEC_IMAGE_DECODER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_IMAGE_DECODER, SwfdecImageDecoderClass))
#define SWFDEC_IMAGE_DECODER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_IMAGE_DECODER, SwfdecImageDecoderClass))

struct _SwfdecImageDecoder
{
  SwfdecDecoder		decoder;

  SwfdecBufferQueue *	queue;		/* keeps the data while decoding */
  SwfdecImage *		image;		/* the image we display */
};

struct _SwfdecImageDecoderClass {
  SwfdecDecoderClass	decoder_class;
};

GType		swfdec_image_decoder_get_type		(void);


G_END_DECLS

#endif
