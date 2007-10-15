/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_CODEC_H_
#define _SWFDEC_CODEC_H_

#include <glib.h>
#include <libswfdec/swfdec_audio_internal.h>
#include <libswfdec/swfdec_buffer.h>

typedef struct _SwfdecAudioDecoder SwfdecAudioDecoder;

typedef enum {
  SWFDEC_AUDIO_CODEC_UNDEFINED = 0,
  SWFDEC_AUDIO_CODEC_ADPCM = 1,
  SWFDEC_AUDIO_CODEC_MP3 = 2,
  SWFDEC_AUDIO_CODEC_UNCOMPRESSED = 3,
  SWFDEC_AUDIO_CODEC_NELLYMOSER_8KHZ = 5,
  SWFDEC_AUDIO_CODEC_NELLYMOSER = 6
} SwfdecAudioCodec;

typedef SwfdecAudioDecoder * (SwfdecAudioDecoderNewFunc) (SwfdecAudioCodec type, gboolean width,
    SwfdecAudioFormat format);
struct _SwfdecAudioDecoder {
  SwfdecAudioCodec	codec;
  SwfdecAudioFormat	format;
  void			(* push)	(SwfdecAudioDecoder *	decoder,
					 SwfdecBuffer *		buffer);
  SwfdecBuffer *	(* pull)	(SwfdecAudioDecoder *	decoder);
  void		  	(* free)	(SwfdecAudioDecoder *	decoder);
};

SwfdecAudioDecoder *   	swfdec_audio_decoder_new      	(SwfdecAudioCodec	codec,
							 SwfdecAudioFormat	format);
void			swfdec_audio_decoder_free      	(SwfdecAudioDecoder *	decoder);
SwfdecAudioFormat	swfdec_audio_decoder_get_format	(SwfdecAudioDecoder *	decoder);
void			swfdec_audio_decoder_push	(SwfdecAudioDecoder *	decoder,
							 SwfdecBuffer *		buffer);
SwfdecBuffer *		swfdec_audio_decoder_pull	(SwfdecAudioDecoder *	decoder);


G_END_DECLS
#endif
