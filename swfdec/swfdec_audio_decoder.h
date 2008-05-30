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

#ifndef _SWFDEC_AUDIO_DECODER_H_
#define _SWFDEC_AUDIO_DECODER_H_

#include <swfdec/swfdec_buffer.h>
#include <swfdec/swfdec_audio_internal.h>

G_BEGIN_DECLS


#define SWFDEC_AUDIO_CODEC_UNDEFINED 0
#define SWFDEC_AUDIO_CODEC_ADPCM 1
#define SWFDEC_AUDIO_CODEC_MP3 2
#define SWFDEC_AUDIO_CODEC_UNCOMPRESSED 3
#define SWFDEC_AUDIO_CODEC_NELLYMOSER_8KHZ 5
#define SWFDEC_AUDIO_CODEC_NELLYMOSER 6


typedef struct _SwfdecAudioDecoder SwfdecAudioDecoder;
typedef struct _SwfdecAudioDecoderClass SwfdecAudioDecoderClass;

#define SWFDEC_TYPE_AUDIO_DECODER                    (swfdec_audio_decoder_get_type())
#define SWFDEC_IS_AUDIO_DECODER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AUDIO_DECODER))
#define SWFDEC_IS_AUDIO_DECODER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AUDIO_DECODER))
#define SWFDEC_AUDIO_DECODER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AUDIO_DECODER, SwfdecAudioDecoder))
#define SWFDEC_AUDIO_DECODER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AUDIO_DECODER, SwfdecAudioDecoderClass))
#define SWFDEC_AUDIO_DECODER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AUDIO_DECODER, SwfdecAudioDecoderClass))

struct _SwfdecAudioDecoder
{
  GObject		object;

  /*< private >*/
  guint			codec;		/* codec this decoder uses */
  SwfdecAudioFormat	format;		/* format the codec was initialized with */
  gboolean		error;		/* if this codec is in an error state */
};

struct _SwfdecAudioDecoderClass
{
  GObjectClass		object_class;

  void			(* push)	(SwfdecAudioDecoder *	decoder,
					 SwfdecBuffer *		buffer);
  SwfdecBuffer *	(* pull)	(SwfdecAudioDecoder *	decoder);
};

GType			swfdec_audio_decoder_get_type	(void);

gboolean		swfdec_audio_decoder_prepare	(guint			codec,
							 SwfdecAudioFormat	format,
							 char **		missing);
SwfdecAudioDecoder *   	swfdec_audio_decoder_new      	(guint			codec,
							 SwfdecAudioFormat	format);

void			swfdec_audio_decoder_push	(SwfdecAudioDecoder *	decoder,
							 SwfdecBuffer *		buffer);
SwfdecBuffer *		swfdec_audio_decoder_pull	(SwfdecAudioDecoder *	decoder);
gboolean		swfdec_audio_decoder_uses_format(SwfdecAudioDecoder *	decoder,
							 guint			codec,
							 SwfdecAudioFormat	format);

/* for subclasses */
void			swfdec_audio_decoder_error	(SwfdecAudioDecoder *	decoder,
							 const char *		error,
							 ...) G_GNUC_PRINTF (2, 3);
void			swfdec_audio_decoder_errorv	(SwfdecAudioDecoder *	decoder,
							 const char *		error,
							 va_list		args);



G_END_DECLS
#endif
