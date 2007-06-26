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
#include <avcodec.h>
#include <swscale.h>

#include "swfdec_codec_audio.h"
#include "swfdec_codec_video.h"
#include "swfdec_debug.h"

/*** GENERAL ***/

static AVCodecContext *
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

typedef struct {
  SwfdecAudioDecoder	decoder;
  AVCodecContext *	ctx;
  SwfdecBufferQueue *	queue;
} SwfdecAudioDecoderFFMpeg;

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

static void
swfdec_audio_decoder_ffmpeg_push (SwfdecAudioDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecAudioDecoderFFMpeg *ffmpeg = (SwfdecAudioDecoderFFMpeg *) dec;
  int out_size;
  int len;
  guint amount;
  SwfdecBuffer *outbuf = NULL;

  if (buffer == NULL)
    return;
  outbuf = swfdec_buffer_new_and_alloc (AVCODEC_MAX_AUDIO_FRAME_SIZE);
  for (amount = 0; amount < buffer->length; amount += len) {
    
    len = avcodec_decode_audio2 (ffmpeg->ctx, (short *) outbuf->data, &out_size, buffer->data + amount, buffer->length - amount);

    if (len < 0) {
      SWFDEC_ERROR ("Error %d while decoding", len);
      swfdec_buffer_unref (outbuf);
      return;
    }
    if (out_size > 0) {
      SwfdecBuffer *convert;
      outbuf->length = out_size;
      convert = swfdec_codec_ffmpeg_convert (ffmpeg->ctx, outbuf);
      if (convert == NULL) {
	swfdec_buffer_unref (outbuf);
	return;
      }
      swfdec_buffer_queue_push (ffmpeg->queue, convert);
      outbuf->length = AVCODEC_MAX_AUDIO_FRAME_SIZE;
    }
  }
  swfdec_buffer_unref (outbuf);
}

static SwfdecBuffer *
swfdec_audio_decoder_ffmpeg_pull (SwfdecAudioDecoder *dec)
{
  SwfdecAudioDecoderFFMpeg *ffmpeg = (SwfdecAudioDecoderFFMpeg *) dec;

  return swfdec_buffer_queue_pull_buffer (ffmpeg->queue);
}

static void
swfdec_audio_decoder_ffmpeg_free (SwfdecAudioDecoder *dec)
{
  SwfdecAudioDecoderFFMpeg *ffmpeg = (SwfdecAudioDecoderFFMpeg *) dec;

  avcodec_close (ffmpeg->ctx);
  av_free (ffmpeg->ctx);
  swfdec_buffer_queue_unref (ffmpeg->queue);

  g_slice_free (SwfdecAudioDecoderFFMpeg, ffmpeg);
}

SwfdecAudioDecoder *
swfdec_audio_decoder_ffmpeg_new (SwfdecAudioFormat type, gboolean width, SwfdecAudioOut format)
{
  SwfdecAudioDecoderFFMpeg *ffmpeg;
  AVCodecContext *ctx;
  enum CodecID id;

  switch (type) {
    case SWFDEC_AUDIO_FORMAT_ADPCM:
      id = CODEC_ID_ADPCM_SWF;
      break;
    case SWFDEC_AUDIO_FORMAT_MP3:
      id = CODEC_ID_MP3;
      break;
    default:
      return NULL;
  }
  ctx = swfdec_codec_ffmpeg_init (id);
  if (ctx == NULL)
    return NULL;
  ffmpeg = g_slice_new (SwfdecAudioDecoderFFMpeg);
  ffmpeg->ctx = ctx;
  ffmpeg->queue = swfdec_buffer_queue_new ();
  ffmpeg->decoder.out_format = SWFDEC_AUDIO_OUT_STEREO_44100;
  ffmpeg->decoder.pull = swfdec_audio_decoder_ffmpeg_pull;
  ffmpeg->decoder.push = swfdec_audio_decoder_ffmpeg_push;
  ffmpeg->decoder.free = swfdec_audio_decoder_ffmpeg_free;
  ctx->sample_rate = SWFDEC_AUDIO_OUT_RATE (format);
  ctx->channels = SWFDEC_AUDIO_OUT_N_CHANNELS (format);

  return &ffmpeg->decoder;
}

/*** VIDEO ***/

typedef struct {
  SwfdecVideoDecoder	decoder;
  AVCodecContext *	ctx;		/* out context (d'oh) */
  AVFrame *		frame;		/* the frame we use for decoding */
  struct SwsContext *	sws;		/* the format conversion */
} SwfdecVideoDecoderFFMpeg;

SwfdecBuffer *
swfdec_video_decoder_ffmpeg_decode (SwfdecVideoDecoder *dec, SwfdecBuffer *buffer,
    guint *width, guint *height, guint *rowstride)
{
  SwfdecVideoDecoderFFMpeg *codec = (SwfdecVideoDecoderFFMpeg *) dec;
  int got_image;
  SwfdecBuffer *ret;
  AVPicture picture;

  if (avcodec_decode_video (codec->ctx, codec->frame, &got_image, 
	buffer->data, buffer->length) < 0) {
    SWFDEC_WARNING ("error decoding frame");
    return NULL;
  }
  if (codec->sws == NULL) {
    codec->sws = sws_getContext (codec->ctx->width, codec->ctx->height, codec->ctx->pix_fmt,
	codec->ctx->width, codec->ctx->height, PIX_FMT_RGB32, 0, NULL, NULL, NULL);
    if (codec->sws == NULL) {
      SWFDEC_ERROR ("Could not get conversion context");
      return NULL;
    }
  }
  ret = swfdec_buffer_new_and_alloc (codec->ctx->width * codec->ctx->height * 4);
  avpicture_fill (&picture, ret->data, PIX_FMT_RGB32, codec->ctx->width,
      codec->ctx->height);
  sws_scale (codec->sws, codec->frame->data, codec->frame->linesize, 0, codec->ctx->height,
      picture.data, picture.linesize);
  *width = codec->ctx->width;
  *height = codec->ctx->height;
  *rowstride = codec->ctx->width * 4;
  return ret;
}

static void
swfdec_video_decoder_ffmpeg_free (SwfdecVideoDecoder *dec)
{
  SwfdecVideoDecoderFFMpeg *codec = (SwfdecVideoDecoderFFMpeg *) dec;

  if (codec->sws) {
    sws_freeContext (codec->sws);
  };
  avcodec_close (codec->ctx);
  av_free (codec->ctx);
  av_free (codec->frame);
  g_free (codec);
}

SwfdecVideoDecoder *
swfdec_video_decoder_ffmpeg_new (SwfdecVideoFormat type)
{
  SwfdecVideoDecoderFFMpeg *codec;
  AVCodecContext *ctx;
  enum CodecID id;

  switch (type) {
    case SWFDEC_VIDEO_FORMAT_H263:
      id = CODEC_ID_FLV1;
      break;
    case SWFDEC_VIDEO_FORMAT_SCREEN:
      id = CODEC_ID_FLASHSV;
      break;
    default:
      return NULL;
  }
  ctx = swfdec_codec_ffmpeg_init (id);

  if (ctx == NULL)
    return NULL;
  codec = g_new (SwfdecVideoDecoderFFMpeg, 1);
  codec->decoder.decode = swfdec_video_decoder_ffmpeg_decode;
  codec->decoder.free = swfdec_video_decoder_ffmpeg_free;
  codec->ctx = ctx;
  codec->frame = avcodec_alloc_frame ();

  return &codec->decoder;
}

