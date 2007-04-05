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

extern const SwfdecAudioCodec swfdec_codec_adpcm;
extern const SwfdecVideoCodec swfdec_codec_screen;

#ifdef HAVE_MAD
extern const SwfdecAudioCodec swfdec_codec_mad;
#endif

#ifdef HAVE_FFMPEG
extern const SwfdecAudioCodec swfdec_codec_ffmpeg_audio;
extern const SwfdecVideoCodec swfdec_codec_ffmpeg_video;
#endif

extern const SwfdecVideoCodec swfdec_codec_gst_video;

/*** UNCOMPRESSED SOUND ***/

#define U8_FLAG (0x10000)
static gpointer
swfdec_codec_uncompressed_init (SwfdecAudioFormat type, gboolean width, SwfdecAudioOut format)
{
  guint ret = format;
  if (!width)
    ret |= U8_FLAG;
  return GUINT_TO_POINTER (ret);
}

static SwfdecBuffer *
swfdec_codec_uncompressed_decode_8bit (SwfdecBuffer *buffer)
{
  SwfdecBuffer *ret = swfdec_buffer_new_and_alloc (buffer->length * 2);
  gint16 *out = (gint16 *) ret->data;
  guint8 *in = buffer->data;
  guint count = buffer->length;
  guint i;

  for (i = 0; i < count; i++) {
    *out = ((gint16) *in << 8) ^ (-1);
    out++;
    in++;
  }
  return ret;
}

static SwfdecBuffer *
swfdec_codec_uncompressed_decode_16bit (SwfdecBuffer *buffer)
{
  swfdec_buffer_ref (buffer);
  return buffer;
}

static SwfdecAudioOut
swfdec_codec_uncompressed_get_format (gpointer codec_data)
{
  guint format = GPOINTER_TO_UINT (codec_data);
  return format & ~U8_FLAG;
}

static SwfdecBuffer *
swfdec_codec_uncompressed_decode (gpointer codec_data, SwfdecBuffer *buffer)
{
  guint data = GPOINTER_TO_UINT (codec_data);
  if (data & U8_FLAG) {
    return swfdec_codec_uncompressed_decode_8bit (buffer);
  } else {
    return swfdec_codec_uncompressed_decode_16bit (buffer);
  }
}

static SwfdecBuffer *
swfdec_codec_uncompressed_finish (gpointer codec_data)
{
  return NULL;
}

static const SwfdecAudioCodec swfdec_codec_uncompressed = {
  swfdec_codec_uncompressed_init,
  swfdec_codec_uncompressed_get_format,
  swfdec_codec_uncompressed_decode,
  swfdec_codec_uncompressed_finish,
};

/*** PUBLIC API ***/

const SwfdecAudioCodec *
swfdec_codec_get_audio (SwfdecAudioFormat format)
{
  switch (format) {
    case SWFDEC_AUDIO_FORMAT_UNDEFINED:
    case SWFDEC_AUDIO_FORMAT_UNCOMPRESSED:
      return &swfdec_codec_uncompressed;
    case SWFDEC_AUDIO_FORMAT_ADPCM:
      return &swfdec_codec_adpcm;
#ifdef HAVE_FFMPEG
      return &swfdec_codec_ffmpeg_audio;
#else
      SWFDEC_ERROR ("adpcm sound requires ffmpeg");
      return NULL;
#endif
    case SWFDEC_AUDIO_FORMAT_MP3:
#ifdef HAVE_MAD
      return &swfdec_codec_mad;
#else
#ifdef HAVE_FFMPEG
      return &swfdec_codec_ffmpeg_audio;
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

const SwfdecVideoCodec *
swfdec_codec_get_video (SwfdecVideoFormat format)
{
  switch (format) {
    case SWFDEC_VIDEO_FORMAT_SCREEN:
      return &swfdec_codec_screen;
#ifdef HAVE_FFMPEG
      return &swfdec_codec_ffmpeg_video;
#endif
      SWFDEC_ERROR ("Screen video requires ffmpeg");
      return NULL;
    case SWFDEC_VIDEO_FORMAT_H263:
#ifdef HAVE_GST
      return &swfdec_codec_gst_video;
#else
#ifdef HAVE_FFMPEG
      return &swfdec_codec_ffmpeg_video;
#else
      SWFDEC_ERROR ("H263 video requires ffmpeg or GStreamer");
      return NULL;
#endif
#endif
    case SWFDEC_VIDEO_FORMAT_VP6:
#ifdef HAVE_GST
      return &swfdec_codec_gst_video;
#else
      SWFDEC_ERROR ("VP6 video requires ffmpeg or GStreamer");
      return NULL;
#endif
    default:
      SWFDEC_ERROR ("video codec %u not implemented yet", (guint) format);
      return NULL;
  }
}

