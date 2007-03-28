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

/*** GENERAL ***/

static gpointer
swfdec_codec_ffmpeg_init (enum CodecID id)
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

  return ctx;
fail:
  SWFDEC_ERROR ("failed to initialize playback via ffmpeg");
  avcodec_close (ctx);
  av_free (ctx);
  return NULL;
}

/*** AUDIO ***/

static gpointer
swfdec_codec_ffmpeg_mp3_init (gboolean width, SwfdecAudioOut format)
{
  AVCodecContext *ctx;
  
  ctx = swfdec_codec_ffmpeg_init (CODEC_ID_MP3);
  ctx->sample_rate = SWFDEC_AUDIO_OUT_RATE (format);
  ctx->channels = SWFDEC_AUDIO_OUT_N_CHANNELS (format);

  return ctx;
}

static gpointer
swfdec_codec_ffmpeg_adpcm_init (gboolean width, SwfdecAudioOut format)
{
  AVCodecContext *ctx;
  
  ctx = swfdec_codec_ffmpeg_init (CODEC_ID_ADPCM_SWF);
  ctx->sample_rate = SWFDEC_AUDIO_OUT_RATE (format);
  ctx->channels = SWFDEC_AUDIO_OUT_N_CHANNELS (format);

  return ctx;
}

static SwfdecAudioOut
swfdec_codec_ffmpeg_get_format (gpointer data)
{
  /* FIXME: improve this */
  return SWFDEC_AUDIO_OUT_STEREO_44100;
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
      swfdec_buffer_queue_unref (queue);
      swfdec_buffer_unref (outbuf);
      return NULL;
    }
    if (out_size > 0) {
      SwfdecBuffer *convert;
      outbuf->length = out_size;
      convert = swfdec_codec_ffmpeg_convert (ctx, outbuf);
      if (convert == NULL) {
	swfdec_buffer_queue_unref (queue);
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
  swfdec_buffer_queue_unref (queue);

  return outbuf;
}

static SwfdecBuffer *
swfdec_codec_ffmpeg_audio_finish (gpointer ctx)
{
  avcodec_close (ctx);
  av_free (ctx);

  return NULL;
}


const SwfdecAudioCodec swfdec_codec_ffmpeg_mp3 = {
  swfdec_codec_ffmpeg_mp3_init,
  swfdec_codec_ffmpeg_get_format,
  swfdec_codec_ffmpeg_decode,
  swfdec_codec_ffmpeg_audio_finish
};

const SwfdecAudioCodec swfdec_codec_ffmpeg_adpcm = {
  swfdec_codec_ffmpeg_adpcm_init,
  swfdec_codec_ffmpeg_get_format,
  swfdec_codec_ffmpeg_decode,
  swfdec_codec_ffmpeg_audio_finish
};

/*** VIDEO ***/

typedef struct {
  AVCodecContext *	ctx;		/* out context (d'oh) */
  AVFrame *		frame;		/* the frame we use for decoding */
} SwfdecCodecFFMpegVideo;

static gpointer
swfdec_codec_ffmpeg_h263_init (void)
{
  SwfdecCodecFFMpegVideo *codec;
  AVCodecContext *ctx = swfdec_codec_ffmpeg_init (CODEC_ID_FLV1);

  if (ctx == NULL)
    return NULL;
  codec = g_new (SwfdecCodecFFMpegVideo, 1);
  codec->ctx = ctx;
  codec->frame = avcodec_alloc_frame ();

  return codec;
}

static gboolean
swfdec_codec_ffmpeg_video_get_size (gpointer codec_data,
    unsigned int *width, unsigned int *height)
{
  SwfdecCodecFFMpegVideo *codec = codec_data;
  AVCodecContext *ctx = codec->ctx;

  if (ctx->width <= 0 || ctx->height <= 0)
    return FALSE;

  *width = ctx->width;
  *height = ctx->height;

  return TRUE;
}

SwfdecBuffer *
swfdec_codec_ffmpeg_video_decode (gpointer codec_data, SwfdecBuffer *buffer)
{
  SwfdecCodecFFMpegVideo *codec = codec_data;
  int got_image;
  SwfdecBuffer *ret;
  AVPicture picture;

  if (avcodec_decode_video (codec->ctx, codec->frame, &got_image, 
	buffer->data, buffer->length) < 0) {
    SWFDEC_WARNING ("error decoding frame");
    return NULL;
  }
  ret = swfdec_buffer_new_and_alloc (codec->ctx->width * codec->ctx->height * 4);
  avpicture_fill (&picture, ret->data, PIX_FMT_RGB32, codec->ctx->width,
      codec->ctx->height);
  img_convert (&picture, PIX_FMT_RGB32, 
      (AVPicture *) codec->frame, codec->ctx->pix_fmt,
      codec->ctx->width, codec->ctx->height);
  return ret;
}

static void
swfdec_codec_ffmpeg_video_finish (gpointer codec_data)
{
  SwfdecCodecFFMpegVideo *codec = codec_data;
  avcodec_close (codec->ctx);
  av_free (codec->ctx);
  av_free (codec->frame);
}

static gpointer
swfdec_codec_ffmpeg_screen_init (void)
{
  SwfdecCodecFFMpegVideo *codec;
  AVCodecContext *ctx = swfdec_codec_ffmpeg_init (CODEC_ID_FLASHSV);

  if (ctx == NULL)
    return NULL;
  codec = g_new (SwfdecCodecFFMpegVideo, 1);
  codec->ctx = ctx;
  codec->frame = avcodec_alloc_frame ();

  return codec;
}


const SwfdecVideoCodec swfdec_codec_ffmpeg_h263 = {
  swfdec_codec_ffmpeg_h263_init,
  swfdec_codec_ffmpeg_video_get_size,
  swfdec_codec_ffmpeg_video_decode,
  swfdec_codec_ffmpeg_video_finish
};

const SwfdecVideoCodec swfdec_codec_ffmpeg_screen = {
  swfdec_codec_ffmpeg_screen_init,
  swfdec_codec_ffmpeg_video_get_size,
  swfdec_codec_ffmpeg_video_decode,
  swfdec_codec_ffmpeg_video_finish
};

