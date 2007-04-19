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

#include <math.h>
#include "swfdec_net_stream.h"
#include "swfdec_amf.h"
#include "swfdec_audio_flv.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"

/* NB: code and level must be rooted gc-strings */
static void
swfdec_net_stream_onstatus (SwfdecNetStream *stream, const char *code, const char *level)
{
  SwfdecAsValue val;
  SwfdecAsObject *object;

  object = swfdec_as_object_new (SWFDEC_AS_OBJECT (stream)->context);
  if (!object)
    return;
  SWFDEC_INFO ("emitting onStatus for %s %s", level, code);
  SWFDEC_AS_VALUE_SET_STRING (&val, code);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_CODE, &val);
  SWFDEC_AS_VALUE_SET_STRING (&val, level);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_LEVEL, &val);

  SWFDEC_AS_VALUE_SET_OBJECT (&val, object);
  swfdec_as_object_call (SWFDEC_AS_OBJECT (stream), SWFDEC_AS_STR_ON_STATUS, 1, &val, NULL);
}

static void swfdec_net_stream_update_playing (SwfdecNetStream *stream);
static void
swfdec_net_stream_video_goto (SwfdecNetStream *stream, guint timestamp)
{
  SwfdecBuffer *buffer;
  SwfdecVideoFormat format;
  cairo_surface_t *old;
  gboolean process_events;
  guint process_events_from;

  SWFDEC_LOG ("goto %ums", timestamp);
  process_events = timestamp == stream->next_time;
  process_events_from = MIN (stream->next_time, stream->current_time + 1);
  buffer = swfdec_flv_decoder_get_video (stream->flvdecoder, timestamp,
      FALSE, &format, &stream->current_time, &stream->next_time);
  old = stream->surface;
  if (stream->surface) {
    cairo_surface_destroy (stream->surface);
    stream->surface = NULL;
  }
  if (buffer == NULL) {
    SWFDEC_ERROR ("got no buffer?!");
  } else {
    if (format != stream->format) {
      if (stream->decoder)
	swfdec_video_decoder_free (stream->decoder);
      stream->format = format;
      stream->decoder = swfdec_video_decoder_new (format);
    }
    if (stream->decoder) {
      stream->surface = swfdec_video_decoder_decode (stream->decoder, buffer);
    }
    if (stream->surface) {
      GList *walk;
      for (walk = stream->movies; walk; walk = walk->next) {
	swfdec_video_movie_new_image (walk->data, stream->surface);
      }
    }
  }
  if (stream->next_time <= stream->current_time) {
    if (swfdec_flv_decoder_is_eof (stream->flvdecoder)) {
      swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NETSTREAM_PLAY_STOP, SWFDEC_AS_STR_STATUS);
    } else {
      stream->buffering = TRUE;
      swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NETSTREAM_BUFFER_EMPTY, 
	  SWFDEC_AS_STR_STATUS);
    }
    swfdec_net_stream_update_playing (stream);
  }
  if (process_events) {
    while (process_events_from <= stream->current_time) {
      SwfdecAsValue name, value;
      SwfdecBits bits;
      SwfdecBuffer *event = swfdec_flv_decoder_get_data (stream->flvdecoder, process_events_from, &process_events_from);
      if (!event)
	break;
      SWFDEC_LOG ("processing event from timestamp %u", process_events_from);
      process_events_from++; /* increase so we get the next event next time */
      swfdec_bits_init (&bits, event);
      if (swfdec_amf_parse (SWFDEC_AS_OBJECT (stream)->context, &bits, 2, 
	    SWFDEC_AMF_STRING, &name, SWFDEC_AMF_MIXED_ARRAY, &value) != 2) {
	SWFDEC_ERROR ("could not parse data tag");
      } else {
	swfdec_as_object_call (SWFDEC_AS_OBJECT (stream), 
	    SWFDEC_AS_VALUE_GET_STRING (&name), 1, &value, NULL);
      }
    }
  }
}

static void
swfdec_net_stream_timeout (SwfdecTimeout *timeout)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM ((guchar *) timeout - G_STRUCT_OFFSET (SwfdecNetStream, timeout));

  SWFDEC_LOG ("timeout fired");
  stream->timeout.callback = NULL;
  swfdec_net_stream_video_goto (stream, stream->next_time);
  if (stream->next_time > stream->current_time) {
    SWFDEC_LOG ("readding timeout");
    stream->timeout.timestamp += SWFDEC_MSECS_TO_TICKS (stream->next_time - stream->current_time);
    stream->timeout.callback = swfdec_net_stream_timeout;
    swfdec_player_add_timeout (stream->player, &stream->timeout);
  } else {
    if (stream->audio) {
      /* FIXME: just unref and let it take care of removing itself? */
      SWFDEC_LOG ("stopping audio due to EOS");
      swfdec_audio_remove (stream->audio);
      g_object_unref (stream->audio);
      stream->audio = NULL;
    }
  }
}

static void
swfdec_net_stream_update_playing (SwfdecNetStream *stream)
{
  gboolean should_play;
    
  should_play = stream->playing; /* checks user-set play/pause */
  should_play &= !stream->buffering; /* checks enough data is available */
  should_play &= stream->flvdecoder != NULL; /* checks there even is something to play */
  should_play &= stream->next_time > stream->current_time; /* checks if EOF */
  if (should_play && stream->timeout.callback == NULL) {
    SWFDEC_DEBUG ("starting playback");
    stream->timeout.callback = swfdec_net_stream_timeout;
    stream->timeout.timestamp = stream->player->time + SWFDEC_MSECS_TO_TICKS (stream->next_time - stream->current_time);
    swfdec_player_add_timeout (stream->player, &stream->timeout);
    if (stream->flvdecoder->audio) {
      g_assert (stream->audio == NULL);
      SWFDEC_LOG ("starting audio");
      stream->audio = swfdec_audio_flv_new (stream->player, 
	  stream->flvdecoder, stream->current_time);
    } else {
      SWFDEC_LOG ("no audio");
    }
  } else if (!should_play && stream->timeout.callback != NULL) {
    if (stream->audio) {
      SWFDEC_LOG ("stopping audio");
      swfdec_audio_remove (stream->audio);
      g_object_unref (stream->audio);
      stream->audio = NULL;
    }
    swfdec_player_remove_timeout (stream->player, &stream->timeout);
    stream->timeout.callback = NULL;
    SWFDEC_DEBUG ("stopping playback");
  }
}

/*** SWFDEC_LOADER_TARGET interface ***/

static SwfdecPlayer *
swfdec_net_stream_loader_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_NET_STREAM (target)->player;
}

static void
swfdec_net_stream_loader_target_parse (SwfdecLoaderTarget *target, 
    SwfdecLoader *loader)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM (target);
  SwfdecDecoderClass *klass;
  gboolean recheck = FALSE;
  
  if (loader->error) {
    if (stream->flvdecoder == NULL)
      swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NETSTREAM_PLAY_STREAMNOTFOUND,
	  SWFDEC_AS_STR_ERROR);
    return;
  }
  if (!loader->eof && swfdec_buffer_queue_get_depth (loader->queue) == 0) {
    SWFDEC_INFO ("nothing to do");
    return;
  }
  if (stream->flvdecoder == NULL) {
    /* FIXME: add mp3 support */
    stream->flvdecoder = g_object_new (SWFDEC_TYPE_FLV_DECODER, NULL);
    SWFDEC_DECODER (stream->flvdecoder)->player = stream->player;
    SWFDEC_DECODER (stream->flvdecoder)->queue = loader->queue;
    swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NETSTREAM_PLAY_START,
	SWFDEC_AS_STR_STATUS);
    swfdec_loader_set_data_type (loader, SWFDEC_LOADER_DATA_FLV);
  }
  klass = SWFDEC_DECODER_GET_CLASS (stream->flvdecoder);
  g_return_if_fail (klass->parse);

  while (TRUE) {
    SwfdecStatus status = klass->parse (SWFDEC_DECODER (stream->flvdecoder));
    switch (status) {
      case SWFDEC_STATUS_OK:
	break;
      case SWFDEC_STATUS_INIT:
	/* HACK for native flv playback */
	swfdec_player_initialize (stream->player, 7,
	    SWFDEC_DECODER (stream->flvdecoder)->rate, 
	    SWFDEC_DECODER (stream->flvdecoder)->width, 
	    SWFDEC_DECODER (stream->flvdecoder)->height);
      case SWFDEC_STATUS_IMAGE:
	recheck = TRUE;
	break;
      case SWFDEC_STATUS_ERROR:
      case SWFDEC_STATUS_NEEDBITS:
      case SWFDEC_STATUS_EOF:
	goto out;
      default:
	g_assert_not_reached ();
	return;
    }
  }
out:
  if (loader->eof) {
    guint first, last;
    swfdec_flv_decoder_eof (stream->flvdecoder);
    recheck = TRUE;
    swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NETSTREAM_BUFFER_FLUSH,
	SWFDEC_AS_STR_STATUS);
    swfdec_net_stream_video_goto (stream, stream->current_time);
    stream->buffering = FALSE;
    if (swfdec_flv_decoder_get_video_info (stream->flvdecoder, &first, &last) &&
	stream->current_time + stream->buffer_time <= last) {
      swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NETSTREAM_BUFFER_FULL,
	  SWFDEC_AS_STR_STATUS);
    }
  }
  if (recheck) {
    if (stream->buffering) {
      guint first, last;
      if (swfdec_flv_decoder_get_video_info (stream->flvdecoder, &first, &last)) {
	guint current = MAX (first, stream->current_time);
	if (current + stream->buffer_time <= last) {
	  swfdec_net_stream_video_goto (stream, current);
	  stream->buffering = FALSE;
	  swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NETSTREAM_BUFFER_FULL,
	      SWFDEC_AS_STR_STATUS);
	}
      } else {
	SWFDEC_ERROR ("no video stream, how do we update buffering?");
      }
    }
    swfdec_net_stream_update_playing (stream);
  }
}


static void
swfdec_net_stream_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_net_stream_loader_target_get_player;
  iface->parse = swfdec_net_stream_loader_target_parse;
}

/*** SWFDEC VIDEO MOVIE INPUT ***/

static void
swfdec_net_stream_input_connect (SwfdecVideoMovieInput *input, SwfdecVideoMovie *movie)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM ((guchar *) input - G_STRUCT_OFFSET (SwfdecNetStream, input));

  stream->movies = g_list_prepend (stream->movies, movie);
  g_object_ref (stream);
}

static void
swfdec_net_stream_input_disconnect (SwfdecVideoMovieInput *input, SwfdecVideoMovie *movie)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM ((guchar *) input - G_STRUCT_OFFSET (SwfdecNetStream, input));

  stream->movies = g_list_remove (stream->movies, movie);
  g_object_unref (stream);
}

/*** SWFDEC_NET_STREAM ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecNetStream, swfdec_net_stream, SWFDEC_TYPE_AS_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_net_stream_loader_target_init))

static void
swfdec_net_stream_dispose (GObject *object)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM (object);

  swfdec_net_stream_set_playing (stream, FALSE);
  if (stream->surface) {
    cairo_surface_destroy (stream->surface);
    stream->surface = NULL;
  }
  if (stream->decoder) {
    swfdec_video_decoder_free (stream->decoder);
    stream->decoder = NULL;
  }
  swfdec_net_stream_set_loader (stream, NULL);
  g_object_unref (stream->conn);
  stream->conn = NULL;
  g_assert (stream->movies == NULL);

  G_OBJECT_CLASS (swfdec_net_stream_parent_class)->dispose (object);
}

static void
swfdec_net_stream_mark (SwfdecAsObject *object)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM (object);

  swfdec_as_object_mark (SWFDEC_AS_OBJECT (stream->conn));

  SWFDEC_AS_OBJECT_CLASS (swfdec_net_stream_parent_class)->mark (object);
}

static void
swfdec_net_stream_class_init (SwfdecNetStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_net_stream_dispose;

  asobject_class->mark = swfdec_net_stream_mark;
}

static void
swfdec_net_stream_init (SwfdecNetStream *stream)
{
  stream->input.connect = swfdec_net_stream_input_connect;
  stream->input.disconnect = swfdec_net_stream_input_disconnect;

  stream->buffer_time = 100; /* msecs */
}

SwfdecNetStream *
swfdec_net_stream_new (SwfdecPlayer *player, SwfdecConnection *conn)
{
  SwfdecAsContext *context;
  SwfdecNetStream *stream;
  
  g_return_val_if_fail (SWFDEC_IS_CONNECTION (conn), NULL);

  context = SWFDEC_AS_OBJECT (conn)->context;
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecNetStream)))
    return NULL;
  stream = g_object_new (SWFDEC_TYPE_NET_STREAM, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (stream), context, sizeof (SwfdecNetStream));
  stream->conn = conn;

  return stream;
}

void
swfdec_net_stream_set_url (SwfdecNetStream *stream, const char *url)
{
  SwfdecLoader *loader;

  g_return_if_fail (SWFDEC_IS_NET_STREAM (stream));
  g_return_if_fail (url != NULL);

  /* FIXME: use the connection once connections are implemented */
  loader = swfdec_player_load (stream->player, url);
  swfdec_net_stream_set_loader (stream, loader);
  g_object_unref (loader);
}

void
swfdec_net_stream_set_loader (SwfdecNetStream *stream, SwfdecLoader *loader)
{
  g_return_if_fail (SWFDEC_IS_NET_STREAM (stream));
  g_return_if_fail (loader == NULL || SWFDEC_IS_LOADER (loader));

  if (stream->loader)
    g_object_unref (stream->loader);
  if (stream->flvdecoder) {
    g_object_unref (stream->flvdecoder);
    stream->flvdecoder = NULL;
  }
  stream->loader = loader;
  stream->buffering = TRUE;
  if (loader) {
    g_object_ref (loader);
    swfdec_loader_set_target (loader, SWFDEC_LOADER_TARGET (stream));
    swfdec_loader_queue_parse (loader);
  }
  swfdec_net_stream_set_playing (stream, TRUE);
}

void
swfdec_net_stream_set_playing (SwfdecNetStream *stream, gboolean playing)
{
  g_return_if_fail (SWFDEC_IS_NET_STREAM (stream));

  stream->playing = playing;

  swfdec_net_stream_update_playing (stream);
}

gboolean
swfdec_net_stream_get_playing (SwfdecNetStream *stream)
{
  g_return_val_if_fail (SWFDEC_IS_NET_STREAM (stream), FALSE);

  return stream->playing;
}

void
swfdec_net_stream_set_buffer_time (SwfdecNetStream *stream, double secs)
{
  g_return_if_fail (SWFDEC_IS_NET_STREAM (stream));

  /* FIXME: is this correct? */
  if (secs <= 0)
    return;

  stream->buffer_time = secs * 1000;
}

double
swfdec_net_stream_get_buffer_time (SwfdecNetStream *stream)
{
  g_return_val_if_fail (SWFDEC_IS_NET_STREAM (stream), 0.1);

  return (double) stream->buffer_time / 1000.0;
}

void
swfdec_net_stream_seek (SwfdecNetStream *stream, double secs)
{
  guint first, last, msecs;

  g_return_if_fail (SWFDEC_IS_NET_STREAM (stream));

  if (stream->flvdecoder == NULL)
    return;
  if (!isfinite (secs) || secs < 0) {
    SWFDEC_ERROR ("seeking to %g doesn't work", secs);
    return;
  }
  if (!swfdec_flv_decoder_get_video_info (stream->flvdecoder, &first, &last)) {
    SWFDEC_ERROR ("FIXME: implement seeking in audio only NetStream");
    return;
  }
  msecs = secs * 1000;
  msecs += first;
  if (msecs > last)
    msecs = last;
  swfdec_flv_decoder_get_video (stream->flvdecoder, msecs, TRUE, NULL, &msecs, NULL);
  swfdec_net_stream_video_goto (stream, msecs);
  /* FIXME: this needs to be implemented correctly, but requires changes to audio handling:
   * - creating a new audio stream will cause attachAudio scripts to lose information 
   * - implementing seek on audio stream requires a SwfdecAudio::changed signal so audio
   *   backends can react correctly.
   */
  if (stream->audio) {
    SWFDEC_WARNING ("FIXME: restarting audio after seek");
    swfdec_audio_remove (stream->audio);
    g_object_unref (stream->audio);
    stream->audio = swfdec_audio_flv_new (stream->player, 
	stream->flvdecoder, stream->current_time);
  }
}

