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
#include <string.h>
#include <config.h>
#include <avcodec.h>

#include "swfdec_codec.h"
#include "swfdec_debug.h"

static gpointer
swfdec_codec_ffmpeg_init (enum CodecID id, gboolean width, guint channels, guint rate)
{
  AVCodec *codec;
  AVCodecContext *ctx;
  static gboolean initialized = FALSE;

  if (!initialized) {
    avcodec_init();
    avcodec_register_all ();
    initialized = TRUE;
  }

  codec = avcodec_find_decoder (id);
  if (!codec)
    return NULL;

  ctx = avcodec_alloc_context ();
  if (avcodec_open (ctx, codec) < 0)
    goto fail;
  ctx->sample_rate = 44100 / rate;
  ctx->channels = channels;

  return ctx;
fail:
  SWFDEC_ERROR ("failed to initialize playback via ffmpeg");
  avcodec_close (ctx);
  av_free (ctx);
  return NULL;
}

static gpointer
swfdec_codec_ffmpeg_mp3_init (gboolean width, guint channels, guint rate)
{
  return swfdec_codec_ffmpeg_init (CODEC_ID_MP3, width, channels, rate);
}

static gpointer
swfdec_codec_ffmpeg_adpcm_init (gboolean width, guint channels, guint rate)
{
  return swfdec_codec_ffmpeg_init (CODEC_ID_ADPCM_SWF, width, channels, rate);
}

static SwfdecBuffer *
swfdec_codec_ffmpeg_convert (AVCodecContext *ctx, SwfdecBuffer *buffer)
{
  SwfdecBuffer *ret;
  guint count, i, j, rate;
  gint16 *out, *in;

  /* do the common case fast */
  if (ctx->channels == 2 && ctx->sample_rate == 44100) {
    ret = swfdec_buffer_new_and_alloc (buffer->length);
    memcpy (ret->data, buffer->data, buffer->length);
    return ret;
  }

  switch (ctx->sample_rate) {
    case 44100:
      rate = 1;
      break;
    case 22050:
      rate = 2;
      break;
    case 11025:
      rate = 4;
      break;
    default:
      SWFDEC_ERROR ("unsupported sample rate %u", ctx->sample_rate);
      return NULL;
  }
  if (ctx->channels == 1)
    rate *= 2;
  ret = swfdec_buffer_new_and_alloc (buffer->length * rate);
  out = (gint16 *) ret->data;
  in = (gint16 *) buffer->data;
  count = buffer->length / 2;

  for (i = 0; i < count; i++) {
    for (j = 0; j < rate; j++) {
      *out++ = *in;
    }
    in++;
  }
  return ret;
}

static SwfdecBuffer *
swfdec_codec_ffmpeg_decode (gpointer ctx, SwfdecBuffer *buffer)
{
  int out_size;
  int len;
  guint amount;
  SwfdecBuffer *outbuf = NULL;
  SwfdecBufferQueue *queue = swfdec_buffer_queue_new ();

  outbuf = swfdec_buffer_new_and_alloc (AVCODEC_MAX_AUDIO_FRAME_SIZE);
  for (amount = 0; amount < buffer->length; amount += len) {
    
    len = avcodec_decode_audio (ctx, (short *) outbuf->data, &out_size, buffer->data + amount, buffer->length - amount);

    if (len < 0) {
      SWFDEC_ERROR ("Error %d while decoding", len);
      swfdec_buffer_queue_free (queue);
      swfdec_buffer_unref (outbuf);
      return NULL;
    }
    if (out_size > 0) {
      SwfdecBuffer *convert;
      outbuf->length = out_size;
      convert = swfdec_codec_ffmpeg_convert (ctx, outbuf);
      if (convert == NULL) {
	swfdec_buffer_queue_free (queue);
	swfdec_buffer_unref (outbuf);
	return NULL;
      }
      swfdec_buffer_queue_push (queue, convert);
      outbuf->length = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    }
  }
  swfdec_buffer_unref (outbuf);

  amount = swfdec_buffer_queue_get_depth (queue);
  //g_print ("got %u bytes\n", amount);
  if (amount > 0)
    outbuf = swfdec_buffer_queue_pull (queue, amount);
  else
    outbuf = NULL;
  swfdec_buffer_queue_free (queue);

  return outbuf;
}

static SwfdecBuffer *
swfdec_codec_ffmpeg_finish (gpointer ctx)
{
  avcodec_close (ctx);
  av_free (ctx);

  return NULL;
}


const SwfdecCodec swfdec_codec_ffmpeg_mp3 = {
  swfdec_codec_ffmpeg_mp3_init,
  swfdec_codec_ffmpeg_decode,
  swfdec_codec_ffmpeg_finish
};

const SwfdecCodec swfdec_codec_ffmpeg_adpcm = {
  swfdec_codec_ffmpeg_adpcm_init,
  swfdec_codec_ffmpeg_decode,
  swfdec_codec_ffmpeg_finish
};
