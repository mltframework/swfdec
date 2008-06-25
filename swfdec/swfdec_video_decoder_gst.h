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

#ifndef _SWFDEC_VIDEO_DECODER_GST_H_
#define _SWFDEC_VIDEO_DECODER_GST_H_

#include <swfdec/swfdec_codec_gst.h>
#include <swfdec/swfdec_video_decoder.h>

G_BEGIN_DECLS


typedef struct _SwfdecVideoDecoderGst SwfdecVideoDecoderGst;
typedef struct _SwfdecVideoDecoderGstClass SwfdecVideoDecoderGstClass;

#define SWFDEC_TYPE_VIDEO_DECODER_GST                    (swfdec_video_decoder_gst_get_type())
#define SWFDEC_IS_VIDEO_DECODER_GST(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_VIDEO_DECODER_GST))
#define SWFDEC_IS_VIDEO_DECODER_GST_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_VIDEO_DECODER_GST))
#define SWFDEC_VIDEO_DECODER_GST(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_VIDEO_DECODER_GST, SwfdecVideoDecoderGst))
#define SWFDEC_VIDEO_DECODER_GST_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_VIDEO_DECODER_GST, SwfdecVideoDecoderGstClass))
#define SWFDEC_VIDEO_DECODER_GST_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_VIDEO_DECODER_GST, SwfdecVideoDecoderGstClass))

struct _SwfdecVideoDecoderGst
{
  SwfdecVideoDecoder		decoder;

  SwfdecGstDecoder		dec;		/* the decoder element */
  GstBuffer *			last;		/* last decoded buffer */
};

struct _SwfdecVideoDecoderGstClass
{
  SwfdecVideoDecoderClass	decoder_class;
};

GType			swfdec_video_decoder_gst_get_type	(void);


G_END_DECLS
#endif
