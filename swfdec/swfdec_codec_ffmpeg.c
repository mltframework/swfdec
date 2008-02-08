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
#include "swfdec_internal.h"

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
    ret = swfdec_buffer_new (buffer->length);
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
  ret = swfdec_buffer_new (buffer->length * rate);
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
  outbuf = swfdec_buffer_new (AVCODEC_MAX_AUDIO_FRAME_SIZE);
  for (amount = 0; amount < buffer->length; amount += len) {
    
    out_size = AVCODEC_MAX_AUDIO_FRAME_SIZE;
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

gboolean
swfdec_audio_decoder_ffmpeg_prepare (guint type, SwfdecAudioFormat format, char **details)
{
  return type == SWFDEC_AUDIO_CODEC_MP3 || type == SWFDEC_AUDIO_CODEC_ADPCM;
}

SwfdecAudioDecoder *
swfdec_audio_decoder_ffmpeg_new (guint type, SwfdecAudioFormat format)
{
  SwfdecAudioDecoderFFMpeg *ffmpeg;
  AVCodecContext *ctx;
  enum CodecID id;

  switch (type) {
    case SWFDEC_AUDIO_CODEC_ADPCM:
      id = CODEC_ID_ADPCM_SWF;
      break;
    case SWFDEC_AUDIO_CODEC_MP3:
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
  ffmpeg->decoder.format = swfdec_audio_format_new (44100, 2, TRUE);
  ffmpeg->decoder.pull = swfdec_audio_decoder_ffmpeg_pull;
  ffmpeg->decoder.push = swfdec_audio_decoder_ffmpeg_push;
  ffmpeg->decoder.free = swfdec_audio_decoder_ffmpeg_free;
  ctx->sample_rate = swfdec_audio_format_get_rate (format);
  ctx->channels = swfdec_audio_format_get_channels (format);

  return &ffmpeg->decoder;
}

/*** VIDEO ***/

typedef struct {
  SwfdecVideoDecoder	decoder;
  AVCodecContext *	ctx;		/* out context (d'oh) */
  AVFrame *		frame;		/* the frame we use for decoding */
  enum PixelFormat	format;		/* format we must output */
} SwfdecVideoDecoderFFMpeg;

static enum PixelFormat
swfdec_video_decoder_ffmpeg_get_format (guint codec)
{
  switch (swfdec_video_codec_get_format (codec)) {
    case SWFDEC_VIDEO_FORMAT_RGBA:
      return PIX_FMT_RGB32;
    case SWFDEC_VIDEO_FORMAT_I420:
      return PIX_FMT_YUV420P;
    default:
      g_return_val_if_reached (PIX_FMT_RGB32);
  }
}

#define ALIGNMENT 31
static gboolean
swfdec_video_decoder_ffmpeg_decode (SwfdecVideoDecoder *dec, SwfdecBuffer *buffer,
    SwfdecVideoImage *image)
{
  SwfdecVideoDecoderFFMpeg *codec = (SwfdecVideoDecoderFFMpeg *) dec;
  int got_image = 0;
  guchar *tmp, *aligned;

  /* fullfill alignment and padding requirements */
  tmp = g_try_malloc (buffer->length + ALIGNMENT + FF_INPUT_BUFFER_PADDING_SIZE);
  if (tmp == NULL) {
    SWFDEC_WARNING ("Could not allocate temporary memory");
    return FALSE;
  }
  aligned = (guchar *) (((uintptr_t) tmp + ALIGNMENT) & ~ALIGNMENT);
  memcpy (aligned, buffer->data, buffer->length);
  memset (aligned + buffer->length, 0, FF_INPUT_BUFFER_PADDING_SIZE);
  if (avcodec_decode_video (codec->ctx, codec->frame, &got_image, 
	aligned, buffer->length) < 0) {
    g_free (tmp);
    SWFDEC_WARNING ("error decoding frame");
    return FALSE;
  }
  g_free (tmp);
  if (got_image == 0) {
    SWFDEC_WARNING ("did not get an image from decoding");
    return FALSE;
  }
  if (codec->ctx->pix_fmt != codec->format) {
    SWFDEC_WARNING ("decoded to wrong format, expected %u, but got %u",
	codec->format, codec->ctx->pix_fmt);
    return FALSE;
  }
  image->width = codec->ctx->width;
  image->height = codec->ctx->height;
  image->mask = NULL;
  image->plane[0] = codec->frame->data[0];
  image->plane[1] = codec->frame->data[1];
  image->plane[2] = codec->frame->data[2];
  image->rowstride[0] = codec->frame->linesize[0];
  image->rowstride[1] = codec->frame->linesize[1];
  image->rowstride[2] = codec->frame->linesize[2];
  return TRUE;
}

static void
swfdec_video_decoder_ffmpeg_free (SwfdecVideoDecoder *dec)
{
  SwfdecVideoDecoderFFMpeg *codec = (SwfdecVideoDecoderFFMpeg *) dec;

  avcodec_close (codec->ctx);
  av_free (codec->ctx);
  av_free (codec->frame);
  g_free (codec);
}

gboolean
swfdec_video_decoder_ffmpeg_prepare (guint codec, char **detail)
{
  return codec == SWFDEC_VIDEO_CODEC_H263 ||
    codec == SWFDEC_VIDEO_CODEC_SCREEN ||
    codec == SWFDEC_VIDEO_CODEC_VP6;
}

SwfdecVideoDecoder *
swfdec_video_decoder_ffmpeg_new (guint type)
{
  SwfdecVideoDecoderFFMpeg *codec;
  AVCodecContext *ctx;
  enum CodecID id;

  switch (type) {
    case SWFDEC_VIDEO_CODEC_H263:
      id = CODEC_ID_FLV1;
      break;
    case SWFDEC_VIDEO_CODEC_SCREEN:
      id = CODEC_ID_FLASHSV;
      break;
    case SWFDEC_VIDEO_CODEC_VP6:
      id = CODEC_ID_VP6F;
      break;
    default:
      return NULL;
  }
  ctx = swfdec_codec_ffmpeg_init (id);

  if (ctx == NULL)
    return NULL;
  codec = g_new0 (SwfdecVideoDecoderFFMpeg, 1);
  codec->decoder.decode = swfdec_video_decoder_ffmpeg_decode;
  codec->decoder.free = swfdec_video_decoder_ffmpeg_free;
  codec->ctx = ctx;
  codec->frame = avcodec_alloc_frame ();
  codec->format = swfdec_video_decoder_ffmpeg_get_format (type);

  return &codec->decoder;
}

