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
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

enum {
  SWFDEC_STATE_HEADER,			/* need to parse header */
  SWFDEC_STATE_LAST_TAG,		/* read size of last tag */
  SWFDEC_STATE_TAG,			/* read next tag */
  SWFDEC_STATE_EOF			/* file is complete */
};

typedef struct _SwfdecFlvVideoTag SwfdecFlvVideoTag;
typedef struct _SwfdecFlvAudioTag SwfdecFlvAudioTag;
typedef struct _SwfdecFlvDataTag SwfdecFlvDataTag;

struct _SwfdecFlvVideoTag {
  guint			timestamp;		/* milliseconds */
  guint			format;			/* format in use */
  int			frame_type;		/* 0: undefined, 1: keyframe, 2: iframe, 3: H263 disposable iframe */
  SwfdecBuffer *	buffer;			/* buffer for this data */
};

struct _SwfdecFlvAudioTag {
  guint			timestamp;		/* milliseconds */
  guint			format;			/* format in use */
  SwfdecAudioFormat	original_format;      	/* channel/rate information */
  SwfdecBuffer *	buffer;			/* buffer for this data */
};

struct _SwfdecFlvDataTag {
  guint			timestamp;		/* milliseconds */
  SwfdecBuffer *	buffer;			/* buffer containing raw AMF data */
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
  if (flv->data) {
    for (i = 0; i < flv->data->len; i++) {
      SwfdecFlvDataTag *tag = &g_array_index (flv->data, SwfdecFlvDataTag, i);
      swfdec_buffer_unref (tag->buffer);
    }
    g_array_free (flv->data, TRUE);
    flv->data = NULL;
  }
  swfdec_buffer_queue_unref (flv->queue);
  flv->queue = NULL;

  G_OBJECT_CLASS (swfdec_flv_decoder_parent_class)->dispose (object);
}

static SwfdecStatus
swfdec_flv_decoder_parse_header (SwfdecFlvDecoder *flv)
{
  SwfdecBuffer *buffer;
  SwfdecBits bits;
  guint version, header_length;
  gboolean has_audio, has_video;

  buffer = swfdec_buffer_queue_peek (flv->queue, 9);
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
  buffer = swfdec_buffer_queue_pull (flv->queue, header_length);
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
  SwfdecBuffer *buffer;
  SwfdecBits bits;
  guint last_tag;

  buffer = swfdec_buffer_queue_pull (flv->queue, 4);
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

static guint
swfdec_flv_decoder_find_data (SwfdecFlvDecoder *flv, guint timestamp)
{
  guint min, max;

  g_assert (flv->data);
  
  min = 0;
  max = flv->data->len;
  while (max - min > 1) {
    guint cur = (max + min) / 2;
    SwfdecFlvDataTag *tag = &g_array_index (flv->data, SwfdecFlvDataTag, cur);
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
  SwfdecFlvVideoTag tag;

  if (flv->video == NULL) {
    SWFDEC_INFO ("video tags even though header didn't decalre them. Initializing...");
    flv->video = g_array_new (FALSE, FALSE, sizeof (SwfdecFlvVideoTag));
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
    swfdec_player_use_video_codec (SWFDEC_DECODER (flv)->player, tag.format);
    return SWFDEC_STATUS_INIT;
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
  return SWFDEC_STATUS_IMAGE;
}

static SwfdecStatus
swfdec_flv_decoder_parse_audio_tag (SwfdecFlvDecoder *flv, SwfdecBits *bits, guint timestamp)
{
  SwfdecFlvAudioTag tag;

  if (flv->audio == NULL) {
    SWFDEC_INFO ("audio tags even though header didn't decalre them. Initializing...");
    flv->audio = g_array_new (FALSE, FALSE, sizeof (SwfdecFlvAudioTag));
    return SWFDEC_STATUS_OK;
  }

  tag.timestamp = timestamp;
  tag.format = swfdec_bits_getbits (bits, 4);
  tag.original_format = swfdec_audio_format_parse (bits);
  tag.buffer = swfdec_bits_get_buffer (bits, -1);
  SWFDEC_LOG ("  codec: %u", (guint) tag.format);
  SWFDEC_LOG ("  format: %s", swfdec_audio_format_to_string (tag.original_format));
  if (tag.buffer == NULL) {
    SWFDEC_WARNING ("no buffer, ignoring");
    return SWFDEC_STATUS_OK;
  }
  if (flv->audio->len == 0) {
    g_array_append_val (flv->audio, tag);
    swfdec_player_use_audio_codec (SWFDEC_DECODER (flv)->player, tag.format, tag.original_format);
    return SWFDEC_STATUS_INIT;
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
  return SWFDEC_STATUS_OK;
}

static void
swfdec_flv_decoder_parse_data_tag (SwfdecFlvDecoder *flv, SwfdecBits *bits, guint timestamp)
{
  SwfdecFlvDataTag tag;

  if (flv->data == NULL) {
    flv->data = g_array_new (FALSE, FALSE, sizeof (SwfdecFlvDataTag));
  }

  tag.timestamp = timestamp;
  tag.buffer = swfdec_bits_get_buffer (bits, -1);
  if (tag.buffer == NULL) {
    SWFDEC_WARNING ("no buffer, ignoring");
    return;
  }
  if (flv->data->len == 0) {
    g_array_append_val (flv->data, tag);
  } else if (g_array_index (flv->data, SwfdecFlvDataTag, 
	flv->data->len - 1).timestamp < tag.timestamp) {
    g_array_append_val (flv->data, tag);
  } else {
    guint idx;
    SWFDEC_WARNING ("timestamps of data buffers not increasing (last was %u, now %u)",
	g_array_index (flv->data, SwfdecFlvDataTag, flv->data->len - 1).timestamp, 
	tag.timestamp);
    idx = swfdec_flv_decoder_find_data (flv, tag.timestamp);
    g_array_insert_val (flv->data, idx, tag);
  }
}

static SwfdecStatus
swfdec_flv_decoder_parse_tag (SwfdecFlvDecoder *flv)
{
  SwfdecBuffer *buffer;
  SwfdecBits bits;
  guint size, type, timestamp;
  SwfdecStatus ret = SWFDEC_STATUS_OK;

  buffer = swfdec_buffer_queue_peek (flv->queue, 4);
  if (buffer == NULL)
    return SWFDEC_STATUS_NEEDBITS;
  swfdec_bits_init (&bits, buffer);
  swfdec_bits_get_u8 (&bits);
  size = swfdec_bits_get_bu24 (&bits);
  swfdec_buffer_unref (buffer);
  buffer = swfdec_buffer_queue_pull (flv->queue, 11 + size);
  if (buffer == NULL)
    return SWFDEC_STATUS_NEEDBITS;
  swfdec_bits_init (&bits, buffer);
  type = swfdec_bits_get_u8 (&bits);
  /* I think I'm paranoid and complicated. I think I'm paranoid, manipulated */
  if (size != swfdec_bits_get_bu24 (&bits)) {
    g_assert_not_reached ();
  }
  timestamp = swfdec_bits_get_bu24 (&bits);
  swfdec_bits_get_bu32 (&bits);
  SWFDEC_LOG ("new tag");
  SWFDEC_LOG ("  type %u", type);
  SWFDEC_LOG ("  size %u", size);
  SWFDEC_LOG ("  timestamp %u", timestamp);
  switch (type) {
    case 8:
      ret = swfdec_flv_decoder_parse_audio_tag (flv, &bits, timestamp);
      break;
    case 9:
      ret = swfdec_flv_decoder_parse_video_tag (flv, &bits, timestamp);
      break;
    case 18:
      swfdec_flv_decoder_parse_data_tag (flv, &bits, timestamp);
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
swfdec_flv_decoder_parse (SwfdecDecoder *dec, SwfdecBuffer *buffer)
{
  SwfdecFlvDecoder *flv = SWFDEC_FLV_DECODER (dec);
  SwfdecStatus status = 0;

  swfdec_buffer_queue_push (flv->queue, buffer);

  do {
    switch (flv->state) {
      case SWFDEC_STATE_HEADER:
	status |= swfdec_flv_decoder_parse_header (flv);
	break;
      case SWFDEC_STATE_LAST_TAG:
	status |= swfdec_flv_decoder_parse_last_tag (flv);
	break;
      case SWFDEC_STATE_TAG:
	status |= swfdec_flv_decoder_parse_tag (flv);
	break;
      case SWFDEC_STATE_EOF:
	status |= SWFDEC_STATUS_EOF;
	break;
      default:
	g_assert_not_reached ();
	status |= SWFDEC_STATUS_ERROR;
	break;
    }
  } while ((status & (SWFDEC_STATUS_EOF | SWFDEC_STATUS_NEEDBITS | SWFDEC_STATUS_ERROR)) == 0);

  return status;
}

static SwfdecStatus
swfdec_flv_decoder_eof (SwfdecDecoder *dec)
{
  SwfdecFlvDecoder *flv = SWFDEC_FLV_DECODER (dec);

  flv->state = SWFDEC_STATE_EOF;

  return 0;
}

static void
swfdec_flv_decoder_class_init (SwfdecFlvDecoderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  SwfdecDecoderClass *decoder_class = SWFDEC_DECODER_CLASS (class);

  object_class->dispose = swfdec_flv_decoder_dispose;

  decoder_class->parse = swfdec_flv_decoder_parse;
  decoder_class->eof = swfdec_flv_decoder_eof;
}

static void
swfdec_flv_decoder_init (SwfdecFlvDecoder *flv)
{
  flv->state = SWFDEC_STATE_HEADER;
  flv->queue = swfdec_buffer_queue_new ();
}

SwfdecBuffer *
swfdec_flv_decoder_get_video (SwfdecFlvDecoder *flv, guint timestamp,
    gboolean keyframe, guint *format, guint *real_timestamp, guint *next_timestamp)
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
      *format = SWFDEC_VIDEO_CODEC_UNDEFINED;
    return NULL;
  }
  offset = g_array_index (flv->video, SwfdecFlvVideoTag, 0).timestamp;
  timestamp += offset;
  id = swfdec_flv_decoder_find_video (flv, timestamp);
  tag = &g_array_index (flv->video, SwfdecFlvVideoTag, id);
  if (keyframe) {
    while (id > 0 && tag->frame_type != 1) {
      id--;
      tag--;
    }
  }
  if (next_timestamp) {
    if (id + 1 >= flv->video->len)
      *next_timestamp = 0;
    else
      *next_timestamp = g_array_index (flv->video, SwfdecFlvVideoTag, id + 1).timestamp - offset;
  }
  if (real_timestamp)
    *real_timestamp = tag->timestamp - offset;
  if (format)
    *format = tag->format;
  return tag->buffer;
}

gboolean
swfdec_flv_decoder_get_video_info (SwfdecFlvDecoder *flv,
    guint *first_timestamp, guint *last_timestamp)
{
  g_return_val_if_fail (SWFDEC_IS_FLV_DECODER (flv), FALSE);

  if (flv->video == NULL)
    return FALSE;

  if (flv->video->len == 0) {
    if (first_timestamp)
      *first_timestamp = 0;
    if (last_timestamp)
      *last_timestamp = 0;
    return TRUE;
  }
  if (first_timestamp)
    *first_timestamp = g_array_index (flv->video, SwfdecFlvVideoTag, 0).timestamp;
  if (last_timestamp)
    *last_timestamp = g_array_index (flv->video, SwfdecFlvVideoTag, flv->video->len - 1).timestamp;
  return TRUE;
}

SwfdecBuffer *
swfdec_flv_decoder_get_audio (SwfdecFlvDecoder *flv, guint timestamp,
    guint *codec, SwfdecAudioFormat *format,
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
    if (codec)
      *codec = SWFDEC_AUDIO_CODEC_UNDEFINED;
    if (format)
      *format = swfdec_audio_format_new (44100, 2, TRUE);
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
  if (codec)
    *codec = tag->format;
  if (format)
    *format = tag->original_format;
  return tag->buffer;
}

/**
 * swfdec_flv_decoder_get_data:
 * @flv: a #SwfdecFlvDecoder
 * @timestamp: timestamp to look for
 * @real_timestamp: the timestamp of the returned buffer, if any
 *
 * Finds the next data event with a timestamp of at least @timestamp. If one 
 * exists, it is returned, and its real timestamp put into @real_timestamp. 
 * Otherwise, %NULL is returned.
 *
 * Returns: a #SwfdecBuffer containing the next data or NULL if none
 **/
SwfdecBuffer *
swfdec_flv_decoder_get_data (SwfdecFlvDecoder *flv, guint timestamp, guint *real_timestamp)
{
  guint id;
  SwfdecFlvDataTag *tag;

  g_return_val_if_fail (SWFDEC_IS_FLV_DECODER (flv), NULL);
  
  if (flv->data == NULL ||
      flv->data->len == 0)
    return NULL;

  id = swfdec_flv_decoder_find_data (flv, timestamp);
  tag = &g_array_index (flv->data, SwfdecFlvDataTag, id);
  while (tag->timestamp < timestamp) {
    id++;
    if (id >= flv->data->len)
      return NULL;
    tag++;
  }
  if (real_timestamp)
    *real_timestamp = tag->timestamp;
  return tag->buffer;
}

gboolean
swfdec_flv_decoder_is_eof (SwfdecFlvDecoder *flv)
{
  g_return_val_if_fail (SWFDEC_IS_FLV_DECODER (flv), TRUE);

  return flv->state == SWFDEC_STATE_EOF;
}

