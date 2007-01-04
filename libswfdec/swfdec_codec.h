/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

typedef struct _SwfdecCodec SwfdecCodec;

typedef enum {
  SWFDEC_AUDIO_FORMAT_UNDEFINED = 0,
  SWFDEC_AUDIO_FORMAT_ADPCM = 1,
  SWFDEC_AUDIO_FORMAT_MP3 = 2,
  SWFDEC_AUDIO_FORMAT_UNCOMPRESSED = 3,
  SWFDEC_AUDIO_FORMAT_NELLYMOSER_8KHZ = 5,
  SWFDEC_AUDIO_FORMAT_NELLYMOSER = 6
} SwfdecAudioFormat;

typedef enum {
  SWFDEC_VIDEO_FORMAT_UNDEFINED
} SwfdecVideoFormat;

struct _SwfdecCodec {
  gpointer		(* init)	(gboolean	width,
					 SwfdecAudioOut	format);
  SwfdecAudioOut	(* get_format)	(gpointer       codec_data);
  SwfdecBuffer *	(* decode)	(gpointer	codec_data,
					 SwfdecBuffer *	buffer);
  SwfdecBuffer *	(* finish)	(gpointer	codec_data);
};

const SwfdecCodec *   	swfdec_codec_get_audio		(SwfdecAudioFormat	format);
const SwfdecCodec *  	swfdec_codec_get_video		(SwfdecVideoFormat	format);

#define swfdec_codec_init(codec,width,format) (codec)->init (width, format)
#define swfdec_codec_get_format(codec, codec_data) (codec)->get_format (codec_data)
#define swfdec_codec_decode(codec, codec_data, buffer) (codec)->decode (codec_data, buffer)
#define swfdec_codec_finish(codec, codec_data) (codec)->finish (codec_data)


G_END_DECLS
#endif
