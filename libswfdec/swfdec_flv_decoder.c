/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_flv_decoder.h"
#include "swfdec_audio_internal.h"
#include "swfdec_bits.h"
#include "swfdec_codec.h"
#include "swfdec_debug.h"

enum {
  SWFDEC_STATE_HEADER,			/* need to parse header */
  SWFDEC_STATE_LAST_TAG,		/* read size of last tag */
  SWFDEC_STATE_TAG,			/* read next tag */
  SWFDEC_STATE_EOF			/* file is complete */
};

typedef struct _SwfdecFlvVideoTag SwfdecFlvVideoTag;
typedef struct _SwfdecFlvAudioTag SwfdecFlvAudioTag;

struct _SwfdecFlvVideoTag {
  guint			timestamp;		/* milliseconds */
  SwfdecVideoFormat	format;			/* format in use */
  int			frame_type;		/* 0: undefined, 1: keyframe, 2: iframe, 3: H263 disposable iframe */
  SwfdecBuffer *	buffer;			/* buffer for this data */
};

struct _SwfdecFlvAudioTag {
  guint			timestamp;		/* milliseconds */
  SwfdecAudioFormat	format;			/* format in use */
  gboolean		width;			/* TRUE for 16bit, FALSE for 8bit */
  SwfdecAudioOut	original_format;      	/* channel/rate information */
  SwfdecBuffer *	buffer;			/* buffer for this data */
};

G_DEFINE_TYPE (SwfdecFlvDecoder, swfdec_flv_decoder, SWFDEC_TYPE_DECODER)

static void
swfdec_flv_decoder_dispose (GObject *object)
{
  SwfdecFlvDecoder *flv = SWFDEC_FLV_DECODER (object);
  guint i;

  if (flv->audio) {
    for (i = 0; i < flv->audio->len; i++) {
      SwfdecFlvAudioTag *tag = &g_array_index (flv->audio, SwfdecFlvAudioTag, i);
      swfdec_buffer_unref (tag->buffer);
    }
    g_array_free (flv->audio, TRUE);
    flv->audio = NULL;
  }
  if (flv->video) {
    for (i = 0; i < flv->video->len; i++) {
      SwfdecFlvVideoTag *tag = &g_array_index (flv->video, SwfdecFlvVideoTag, i);
      swfdec_buffer_unref (tag->buffer);
    }
    g_array_free (flv->video, TRUE);
    flv->video = NULL;
  }

  G_OBJECT_CLASS (swfdec_flv_decoder_parent_class)->dispose (object);
}

static SwfdecStatus
swfdec_flv_decoder_parse_header (SwfdecFlvDecoder *flv)
{
  SwfdecDecoder *dec = SWFDEC_DECODER (flv);
  SwfdecBuffer *buffer;
  SwfdecBits bits;
  guint version, header_length;
  gboolean has_audio, has_video;

  /* NB: we're still reading from the original queue, since deflating is not initialized yet */
  buffer = swfdec_buffer_queue_peek (dec->queue, 9);
  if (buffer == NULL)
    return SWFDEC_STATUS_NEEDBITS;

  swfdec_bits_init (&bits, buffer);
  /* Check if we're really an FLV file */
  if (swfdec_bits_get_u8 (&bits) != 'F' ||
      swfdec_bits_get_u8 (&bits) != 'L' ||
      swfdec_bits_get_u8 (&bits) != 'V') {
    swfdec_buffer_unref (buffer);
    return SWFDEC_STATUS_ERROR;
  }

  version = swfdec_bits_get_u8 (&bits);
  swfdec_bits_getbits (&bits, 5);
  has_audio = swfdec_bits_getbit (&bits);
  swfdec_bits_getbit (&bits);
  has_video = swfdec_bits_getbit (&bits);
  header_length = swfdec_bits_get_bu32 (&bits);
  swfdec_buffer_unref (buffer);
  if (header_length < 9) {
    SWFDEC_ERROR ("invalid header length %u, must be 9 or greater", header_length);
    /* FIXME: treat as error or ignore? */
    return SWFDEC_STATUS_ERROR;
  }
  buffer = swfdec_buffer_queue_pull (dec->queue, header_length);
  if (buffer == NULL)
    return SWFDEC_STATUS_NEEDBITS;
  swfdec_buffer_unref (buffer);
  SWFDEC_LOG ("parsing flv stream");
  SWFDEC_LOG (" version %u", version);
  SWFDEC_LOG (" with%s audio", has_audio ? "" : "out");
  SWFDEC_LOG (" with%s video", has_video ? "" : "out");
  flv->version = version;
  if (has_audio) {
    flv->audio = g_array_new (FALSE, FALSE, sizeof (SwfdecFlvAudioTag));
  }
  if (has_video) {
    flv->video = g_array_new (FALSE, FALSE, sizeof (SwfdecFlvVideoTag));
  }
  flv->state = SWFDEC_STATE_LAST_TAG;

  return SWFDEC_STATUS_OK;
}

static SwfdecStatus
swfdec_flv_decoder_parse_last_tag (SwfdecFlvDecoder *flv)
{
  SwfdecDecoder *dec = SWFDEC_DECODER (flv);
  SwfdecBuffer *buffer;
  SwfdecBits bits;
  guint last_tag;

  buffer = swfdec_buffer_queue_pull (dec->queue, 4);
  if (buffer == NULL)
    return SWFDEC_STATUS_NEEDBITS;

  swfdec_bits_init (&bits, buffer);
  last_tag = swfdec_bits_get_bu32 (&bits);
  SWFDEC_LOG ("last tag was %u bytes", last_tag);
  swfdec_buffer_unref (buffer);
  flv->state = SWFDEC_STATE_TAG;
  return SWFDEC_STATUS_OK;
}

static guint
swfdec_flv_decoder_find_video (SwfdecFlvDecoder *flv, guint timestamp)
{
  guint min, max;

  g_assert (flv->video);
  
  min = 0;
  max = flv->video->len;
  while (max - min > 1) {
    guint cur = (max + min) / 2;
    SwfdecFlvVideoTag *tag = &g_array_index (flv->video, SwfdecFlvVideoTag, cur);
    if (tag->timestamp > timestamp) {
      max = cur;
    } else {
      min = cur;
    }
  }
  return min;
}

static guint
swfdec_flv_decoder_find_audio (SwfdecFlvDecoder *flv, guint timestamp)
{
  guint min, max;

  g_assert (flv->audio);
  
  min = 0;
  max = flv->audio->len;
  while (max - min > 1) {
    guint cur = (max + min) / 2;
    SwfdecFlvAudioTag *tag = &g_array_index (flv->audio, SwfdecFlvAudioTag, cur);
    if (tag->timestamp > timestamp) {
      max = cur;
    } else {
      min = cur;
    }
  }
  return min;
}

static SwfdecStatus
swfdec_flv_decoder_parse_video_tag (SwfdecFlvDecoder *flv, SwfdecBits *bits, guint timestamp)
{
  SwfdecDecoder *dec = SWFDEC_DECODER (flv);
  SwfdecFlvVideoTag tag;

  if (flv->video == NULL) {
    SWFDEC_ERROR ("wow, video tags in an FLV without video!");
    return SWFDEC_STATUS_OK;
  }

  tag.timestamp = timestamp;
  tag.frame_type = swfdec_bits_getbits (bits, 4);
  tag.format = swfdec_bits_getbits (bits, 4);
  tag.buffer = swfdec_bits_get_buffer (bits, -1);
  SWFDEC_LOG ("  format: %u", tag.format);
  SWFDEC_LOG ("  frame type: %u", tag.frame_type);
  if (tag.buffer == NULL) {
    SWFDEC_WARNING ("no buffer, ignoring");
    return SWFDEC_STATUS_OK;
  }
  if (flv->video->len == 0) {
    g_array_append_val (flv->video, tag);
  } else if (g_array_index (flv->video, SwfdecFlvVideoTag, 
	flv->video->len - 1).timestamp < tag.timestamp) {
    g_array_append_val (flv->video, tag);
  } else {
    guint idx;
    SWFDEC_WARNING ("timestamps of video buffers not increasing (last was %u, now %u)",
	g_array_index (flv->video, SwfdecFlvVideoTag, flv->video->len - 1).timestamp, 
	tag.timestamp);
    idx = swfdec_flv_decoder_find_video (flv, tag.timestamp);
    g_array_insert_val (flv->video, idx, tag);
  }
  if (dec->width == 0 && dec->height == 0) {
    SwfdecFlvVideoTag *tag = &g_array_index (flv->video, SwfdecFlvVideoTag, 0);
    const SwfdecVideoCodec *codec = swfdec_codec_get_video (tag->format);
    gpointer decoder;
    SwfdecBuffer *ignore;

    if (codec == NULL)
      return SWFDEC_STATUS_OK;
    decoder = swfdec_video_codec_init (codec);
    if (decoder == NULL)
      return SWFDEC_STATUS_OK;
    ignore = swfdec_video_codec_decode (codec, decoder, tag->buffer);
    if (ignore)
      swfdec_buffer_unref (ignore);
    if (!swfdec_video_codec_get_size (codec, decoder, &dec->width, &dec->height)) {
      swfdec_video_codec_finish (codec, decoder);
      return SWFDEC_STATUS_OK;
    }
    swfdec_video_codec_finish (codec, decoder);
    return SWFDEC_STATUS_INIT;
  } else {
    return SWFDEC_STATUS_IMAGE;
  }
}

static void
swfdec_flv_decoder_parse_audio_tag (SwfdecFlvDecoder *flv, SwfdecBits *bits, guint timestamp)
{
  SwfdecFlvAudioTag tag;
  int rate;
  gboolean stereo;

  if (flv->audio == NULL) {
    SWFDEC_ERROR ("wow, audio tags in an FLV without audio!");
    return;
  }

  tag.timestamp = timestamp;
  tag.format = swfdec_bits_getbits (bits, 4);
  rate = 44100 / (1 << (3 - swfdec_bits_getbits (bits, 2)));
  tag.width = swfdec_bits_getbit (bits);
  stereo = swfdec_bits_getbit (bits);
  tag.original_format = SWFDEC_AUDIO_OUT_GET (stereo ? 2 : 1, rate);
  tag.buffer = swfdec_bits_get_buffer (bits, -1);
  SWFDEC_LOG ("  format: %u", (guint) tag.format);
  SWFDEC_LOG ("  rate: %d", rate);
  SWFDEC_LOG ("  channels: %d", stereo ? 2 : 1);
  if (tag.buffer == NULL) {
    SWFDEC_WARNING ("no buffer, ignoring");
    return;
  }
  if (flv->audio->len == 0) {
    g_array_append_val (flv->audio, tag);
  } else if (g_array_index (flv->audio, SwfdecFlvAudioTag, 
	flv->audio->len - 1).timestamp < tag.timestamp) {
    g_array_append_val (flv->audio, tag);
  } else {
    guint idx;
    SWFDEC_WARNING ("timestamps of audio buffers not increasing (last was %u, now %u)",
	g_array_index (flv->audio, SwfdecFlvAudioTag, flv->audio->len - 1).timestamp, 
	tag.timestamp);
    idx = swfdec_flv_decoder_find_audio (flv, tag.timestamp);
    g_array_insert_val (flv->audio, idx, tag);
  }
}

static SwfdecStatus
swfdec_flv_decoder_parse_tag (SwfdecFlvDecoder *flv)
{
  SwfdecDecoder *dec = SWFDEC_DECODER (flv);
  SwfdecBuffer *buffer;
  SwfdecBits bits;
  guint size, type, timestamp;
  SwfdecStatus ret = SWFDEC_STATUS_OK;

  buffer = swfdec_buffer_queue_peek (dec->queue, 4);
  if (buffer == NULL)
    return SWFDEC_STATUS_NEEDBITS;
  swfdec_bits_init (&bits, buffer);
  swfdec_bits_get_u8 (&bits);
  size = swfdec_bits_get_bu24 (&bits);
  swfdec_buffer_unref (buffer);
  buffer = swfdec_buffer_queue_pull (dec->queue, 11 + size);
  if (buffer == NULL)
    return SWFDEC_STATUS_NEEDBITS;
  swfdec_bits_init (&bits, buffer);
  type = swfdec_bits_get_u8 (&bits);
  /* I think I'm paranoid and complicated. I think I'm paranoid, manipulated */
  if (size != swfdec_bits_get_bu24 (&bits))
    g_assert_not_reached ();
  timestamp = swfdec_bits_get_bu24 (&bits);
  swfdec_bits_get_bu32 (&bits);
  SWFDEC_LOG ("new tag");
  SWFDEC_LOG ("  type %u", type);
  SWFDEC_LOG ("  size %u", size);
  SWFDEC_LOG ("  timestamp %u", timestamp);
  switch (type) {
    case 8:
      swfdec_flv_decoder_parse_audio_tag (flv, &bits, timestamp);
      break;
    case 9:
      ret = swfdec_flv_decoder_parse_video_tag (flv, &bits, timestamp);
      break;
    default:
      SWFDEC_WARNING ("unknown tag (type %u)", type);
      break;
  }
  swfdec_buffer_unref (buffer);
  flv->state = SWFDEC_STATE_LAST_TAG;
  return ret;
}

static SwfdecStatus
swfdec_flv_decoder_parse (SwfdecDecoder *dec)
{
  SwfdecFlvDecoder *flv = SWFDEC_FLV_DECODER (dec);
  SwfdecStatus ret;

  switch (flv->state) {
    case SWFDEC_STATE_HEADER:
      ret = swfdec_flv_decoder_parse_header (flv);
      break;
    case SWFDEC_STATE_LAST_TAG:
      ret = swfdec_flv_decoder_parse_last_tag (flv);
      break;
    case SWFDEC_STATE_TAG:
      ret = swfdec_flv_decoder_parse_tag (flv);
      break;
    case SWFDEC_STATE_EOF:
      ret = SWFDEC_STATUS_EOF;
      break;
    default:
      g_assert_not_reached ();
      ret = SWFDEC_STATUS_ERROR;
      break;
  }

  return ret;
}

static void
swfdec_flv_decoder_class_init (SwfdecFlvDecoderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  SwfdecDecoderClass *decoder_class = SWFDEC_DECODER_CLASS (class);

  object_class->dispose = swfdec_flv_decoder_dispose;

  decoder_class->parse = swfdec_flv_decoder_parse;
}

static void
swfdec_flv_decoder_init (SwfdecFlvDecoder *flv)
{
  flv->state = SWFDEC_STATE_HEADER;
}

SwfdecBuffer *
swfdec_flv_decoder_get_video (SwfdecFlvDecoder *flv, guint timestamp,
    gboolean keyframe, SwfdecVideoFormat *format, guint *real_timestamp, guint *next_timestamp)
{
  guint id, offset;
  SwfdecFlvVideoTag *tag;

  g_return_val_if_fail (SWFDEC_IS_FLV_DECODER (flv), NULL);
  g_return_val_if_fail (flv->video != NULL, NULL);

  if (flv->video->len == 0) {
    if (next_timestamp)
      *next_timestamp = 0;
    if (real_timestamp)
      *real_timestamp = 0;
    if (format)
      *format = SWFDEC_VIDEO_FORMAT_UNDEFINED;
    return NULL;
  }
  offset = g_array_index (flv->video, SwfdecFlvVideoTag, 0).timestamp;
  timestamp += offset;
  id = swfdec_flv_decoder_find_video (flv, timestamp);
  if (next_timestamp) {
    if (id + 1 >= flv->video->len)
      *next_timestamp = 0;
    else
      *next_timestamp = g_array_index (flv->video, SwfdecFlvVideoTag, id + 1).timestamp - offset;
  }
  tag = &g_array_index (flv->video, SwfdecFlvVideoTag, id);
  if (real_timestamp)
    *real_timestamp = tag->timestamp - offset;
  if (format)
    *format = tag->format;
  return tag->buffer;
}

SwfdecBuffer *
swfdec_flv_decoder_get_audio (SwfdecFlvDecoder *flv, guint timestamp,
    SwfdecAudioFormat *codec_format, gboolean *width, SwfdecAudioOut *format,
    guint *real_timestamp, guint *next_timestamp)
{
  guint id, offset;
  SwfdecFlvAudioTag *tag;

  g_return_val_if_fail (SWFDEC_IS_FLV_DECODER (flv), NULL);
  g_return_val_if_fail (flv->audio != NULL, NULL);

  if (flv->audio->len == 0) {
    if (next_timestamp)
      *next_timestamp = 0;
    if (real_timestamp)
      *real_timestamp = 0;
    if (codec_format)
      *codec_format = SWFDEC_AUDIO_FORMAT_UNDEFINED;
    if (width)
      *width = TRUE;
    if (format)
      *format = SWFDEC_AUDIO_OUT_STEREO_44100;
    return NULL;
  }
  offset = g_array_index (flv->audio, SwfdecFlvAudioTag, 0).timestamp;
  timestamp += offset;
  id = swfdec_flv_decoder_find_audio (flv, timestamp);
  if (next_timestamp) {
    if (id + 1 >= flv->audio->len)
      *next_timestamp = 0;
    else
      *next_timestamp = g_array_index (flv->audio, SwfdecFlvAudioTag, id + 1).timestamp - offset;
  }
  tag = &g_array_index (flv->audio, SwfdecFlvAudioTag, id);
  if (real_timestamp)
    *real_timestamp = tag->timestamp - offset;
  if (codec_format)
    *codec_format = tag->format;
  if (width)
    *width = tag->width;
  if (format)
    *format = tag->original_format;
  return tag->buffer;
}

/*** HACK ***/

/* This is a hack to allow native FLV playback IN SwfdecPlayer */

#include "swfdec_loadertarget.h"
#include "swfdec_net_stream.h"
#include "swfdec_root_movie.h"
#include "swfdec_sprite.h"
#include "swfdec_video_movie.h"

static void
notify_initialized (SwfdecPlayer *player, GParamSpec *pspec, SwfdecVideoMovie *movie)
{
  movie->video->width = player->width;
  movie->video->height = player->height;

  swfdec_movie_queue_update (SWFDEC_MOVIE (movie), SWFDEC_MOVIE_INVALID_MATRIX);
  swfdec_movie_invalidate (SWFDEC_MOVIE (movie));
}

SwfdecMovie *
swfdec_flv_decoder_add_movie (SwfdecFlvDecoder *flv, SwfdecMovie *parent)
{
  SwfdecContent *content = swfdec_content_new (0);
  SwfdecMovie *movie;
  SwfdecVideo *video;
  SwfdecConnection *conn;
  SwfdecNetStream *stream;

  /* set up the video movie */
  video = g_object_new (SWFDEC_TYPE_VIDEO, NULL);
  video->width = G_MAXUINT;
  video->height = G_MAXUINT;
  content->graphic = SWFDEC_GRAPHIC (video);
  movie = swfdec_movie_new (parent, content);
  g_object_weak_ref (G_OBJECT (movie), (GWeakNotify) swfdec_content_free, content);
  g_object_weak_ref (G_OBJECT (movie), (GWeakNotify) g_object_unref, video);
  g_signal_connect (SWFDEC_ROOT_MOVIE (parent)->player, "notify::initialized",
      G_CALLBACK (notify_initialized), movie);
  /* set up the playback stream */
  conn = swfdec_connection_new (SWFDEC_ROOT_MOVIE (parent)->player->jscx);
  stream = swfdec_net_stream_new (SWFDEC_ROOT_MOVIE (parent)->player, conn);
  swfdec_net_stream_set_loader (stream, SWFDEC_ROOT_MOVIE (parent)->loader);
  if (!swfdec_loader_target_set_decoder (SWFDEC_LOADER_TARGET (stream), SWFDEC_DECODER (flv))) {
    g_assert_not_reached ();
  }
  swfdec_video_movie_set_input (SWFDEC_VIDEO_MOVIE (movie), &stream->input);
  swfdec_net_stream_set_playing (stream, TRUE);

  return movie;
}

