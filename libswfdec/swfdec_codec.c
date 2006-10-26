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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_codec.h"
#include "swfdec_debug.h"

/*** DECODER LIST ***/

#ifdef HAVE_MAD
extern const SwfdecCodec swfdec_codec_mad;
#endif
#ifdef HAVE_FFMPEG
extern const SwfdecCodec swfdec_codec_ffmpeg_adpcm;
extern const SwfdecCodec swfdec_codec_ffmpeg_mp3;
#endif

/*** UNCOMPRESSED SOUND ***/

static gpointer
swfdec_codec_uncompressed_init (gboolean width, guint channels, guint rate_multiplier)
{
  guint ret = rate_multiplier;
  if (channels == 1)
    ret *= 2;
  if (!width)
    ret |= 0x100;
  return GUINT_TO_POINTER (ret);
}

static SwfdecBuffer *
swfdec_codec_uncompressed_decode_8bit (SwfdecBuffer *buffer, guint multiply)
{
  SwfdecBuffer *ret = swfdec_buffer_new_and_alloc (buffer->length * multiply * 2);
  gint16 *out = (gint16 *) ret->data;
  guint8 *in = buffer->data;
  guint count = buffer->length;
  guint i, j;

  for (i = 0; i < count; i++) {
    gint16 tmp = ((gint16) *in << 8) ^ (-1);
    for (j = 0; j < multiply; j++) {
      *out++ = tmp;
    }
    in++;
  }
  return ret;
}

static SwfdecBuffer *
swfdec_codec_uncompressed_decode_16bit (SwfdecBuffer *buffer, guint multiply)
{
  SwfdecBuffer *ret = swfdec_buffer_new_and_alloc (buffer->length * multiply);
  gint16 *out = (gint16 *) ret->data;
  gint16 *in = (gint16 *) buffer->data;
  guint count = buffer->length / 2;
  guint i, j;

  for (i = 0; i < count; i++) {
    for (j = 0; j < multiply; j++) {
      *out++ = GINT16_FROM_LE (*in);
    }
    in++;
  }
  return ret;
}

static SwfdecBuffer *
swfdec_codec_uncompressed_decode (gpointer codec_data, SwfdecBuffer *buffer)
{
  guint data = GPOINTER_TO_UINT (codec_data);
  if (data == 1) {
    swfdec_buffer_ref (buffer);
    return buffer;
  } else if (data & 0x100) {
    return swfdec_codec_uncompressed_decode_8bit (buffer, data & 0xFF);
  } else {
    return swfdec_codec_uncompressed_decode_16bit (buffer, data);
  }
}

static void
swfdec_codec_uncompressed_finish (gpointer codec_data)
{
  return;
}

static const SwfdecCodec swfdec_codec_uncompressed = {
  swfdec_codec_uncompressed_init,
  swfdec_codec_uncompressed_decode,
  swfdec_codec_uncompressed_finish,
};

/*** PUBLIC API ***/

const SwfdecCodec *
swfdec_codec_get_audio (SwfdecAudioFormat format)
{
  switch (format) {
    case SWFDEC_AUDIO_FORMAT_UNDEFINED:
    case SWFDEC_AUDIO_FORMAT_UNCOMPRESSED:
      return &swfdec_codec_uncompressed;
    case SWFDEC_AUDIO_FORMAT_ADPCM:
#ifdef HAVE_FFMPEG
      return &swfdec_codec_ffmpeg_adpcm;
#else
      SWFDEC_ERROR ("adpcm sound requires ffmpeg");
      return NULL;
#endif
    case SWFDEC_AUDIO_FORMAT_MP3:
#ifdef HAVE_FFMPEG
      return &swfdec_codec_ffmpeg_mp3;
#else
#ifdef HAVE_MAD
      //return &swfdec_codec_mad;
#else
      SWFDEC_ERROR ("mp3 sound requires ffmpeg or mad");
      return NULL;
#endif
#endif
    case SWFDEC_AUDIO_FORMAT_NELLYMOSER:
      SWFDEC_ERROR ("Nellymoser sound is not implemented yet");
      return NULL;
    default:
      SWFDEC_ERROR ("undefined sound format %u", format);
      return NULL;
  }
}

const SwfdecCodec *
swfdec_codec_get_video (unsigned int format)
{
  SWFDEC_ERROR ("video not implemented yet");
  return NULL;
}

