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

typedef struct _SwfdecAudioCodec SwfdecAudioCodec;
typedef struct _SwfdecVideoCodec SwfdecVideoCodec;

typedef enum {
  SWFDEC_AUDIO_FORMAT_UNDEFINED = 0,
  SWFDEC_AUDIO_FORMAT_ADPCM = 1,
  SWFDEC_AUDIO_FORMAT_MP3 = 2,
  SWFDEC_AUDIO_FORMAT_UNCOMPRESSED = 3,
  SWFDEC_AUDIO_FORMAT_NELLYMOSER_8KHZ = 5,
  SWFDEC_AUDIO_FORMAT_NELLYMOSER = 6
} SwfdecAudioFormat;

typedef enum {
  SWFDEC_VIDEO_FORMAT_UNDEFINED = 0,
  SWFDEC_VIDEO_FORMAT_H263 = 2,
  SWFDEC_VIDEO_FORMAT_SCREEN = 3,
  SWFDEC_VIDEO_FORMAT_VP6 = 4,
  SWFDEC_VIDEO_FORMAT_VP6_ALPHA = 5,
  SWFDEC_VIDEO_FORMAT_SCREEN2 = 6
} SwfdecVideoFormat;

struct _SwfdecAudioCodec {
  gpointer		(* init)	(gboolean	width,
					 SwfdecAudioOut	format);
  SwfdecAudioOut	(* get_format)	(gpointer       codec_data);
  /* FIXME: add SwfdecRect *invalid for invalidated region - might make sense for screen? */
  SwfdecBuffer *	(* decode)	(gpointer	codec_data,
					 SwfdecBuffer *	buffer);
  SwfdecBuffer *	(* finish)	(gpointer	codec_data);
};

struct _SwfdecVideoCodec {
  gpointer		(* init)	(void);
  gboolean	    	(* get_size)	(gpointer	codec_data,
					 unsigned int *	width,
					 unsigned int *	height);
  SwfdecBuffer *	(* decode)	(gpointer	codec_data,
					 SwfdecBuffer *	buffer);
  void			(* finish)	(gpointer	codec_data);
};

const SwfdecAudioCodec *   	swfdec_codec_get_audio		(SwfdecAudioFormat	format);
const SwfdecVideoCodec *  	swfdec_codec_get_video		(SwfdecVideoFormat	format);

#define swfdec_audio_codec_init(codec,width,format) (codec)->init (width, format)
#define swfdec_audio_codec_get_format(codec, codec_data) (codec)->get_format (codec_data)
#define swfdec_audio_codec_decode(codec, codec_data, buffer) (codec)->decode (codec_data, buffer)
#define swfdec_audio_codec_finish(codec, codec_data) (codec)->finish (codec_data)

#define swfdec_video_codec_init(codec) (codec)->init ()
#define swfdec_video_codec_get_size(codec, codec_data, width, height) (codec)->get_size (codec_data, width, height)
#define swfdec_video_codec_decode(codec, codec_data, buffer) (codec)->decode (codec_data, buffer)
#define swfdec_video_codec_finish(codec, codec_data) (codec)->finish (codec_data)


G_END_DECLS
#endif
