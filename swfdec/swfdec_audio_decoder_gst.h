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

#ifndef _SWFDEC_AUDIO_DECODER_GST_H_
#define _SWFDEC_AUDIO_DECODER_GST_H_

#include <swfdec/swfdec_audio_decoder.h>
#include <swfdec/swfdec_codec_gst.h>

G_BEGIN_DECLS


typedef struct _SwfdecAudioDecoderGst SwfdecAudioDecoderGst;
typedef struct _SwfdecAudioDecoderGstClass SwfdecAudioDecoderGstClass;

#define SWFDEC_TYPE_AUDIO_DECODER_GST                    (swfdec_audio_decoder_gst_get_type())
#define SWFDEC_IS_AUDIO_DECODER_GST(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AUDIO_DECODER_GST))
#define SWFDEC_IS_AUDIO_DECODER_GST_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AUDIO_DECODER_GST))
#define SWFDEC_AUDIO_DECODER_GST(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AUDIO_DECODER_GST, SwfdecAudioDecoderGst))
#define SWFDEC_AUDIO_DECODER_GST_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AUDIO_DECODER_GST, SwfdecAudioDecoderGstClass))
#define SWFDEC_AUDIO_DECODER_GST_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AUDIO_DECODER_GST, SwfdecAudioDecoderGstClass))

struct _SwfdecAudioDecoderGst
{
  SwfdecAudioDecoder		decoder;

  SwfdecGstDecoder		dec;		/* the actual decoder */
};

struct _SwfdecAudioDecoderGstClass
{
  SwfdecAudioDecoderClass	decoder_class;
};

GType			swfdec_audio_decoder_gst_get_type	(void);

SwfdecAudioDecoder *	swfdec_audio_decoder_gst_new		(guint			codec, 
								 SwfdecAudioFormat	format);
gboolean		swfdec_audio_decoder_gst_prepare	(guint			codec,
								 SwfdecAudioFormat	format,
								 char **		missing);


G_END_DECLS
#endif
