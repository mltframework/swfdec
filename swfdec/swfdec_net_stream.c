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
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_audio_flv.h"
#include "swfdec_cached_video.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_resource.h"
#include "swfdec_sandbox.h"
#include "swfdec_stream_target.h"
#include "swfdec_video_provider.h"

/* NB: code and level must be rooted gc-strings */
static void
swfdec_net_stream_onstatus (SwfdecNetStream *stream, const char *code, const char *level)
{
  SwfdecAsValue val;
  SwfdecAsObject *object;

  swfdec_sandbox_use (stream->sandbox);
  object = swfdec_as_object_new (SWFDEC_AS_OBJECT (stream)->context);
  if (!object) {
    swfdec_sandbox_unuse (stream->sandbox);
    return;
  }
  SWFDEC_INFO ("emitting onStatus for %s %s", level, code);
  SWFDEC_AS_VALUE_SET_STRING (&val, code);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_code, &val);
  SWFDEC_AS_VALUE_SET_STRING (&val, level);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_level, &val);

  SWFDEC_AS_VALUE_SET_OBJECT (&val, object);
  swfdec_as_object_call (SWFDEC_AS_OBJECT (stream), SWFDEC_AS_STR_onStatus, 1, &val, NULL);
  swfdec_sandbox_unuse (stream->sandbox);
}

static void swfdec_net_stream_update_playing (SwfdecNetStream *stream);
static void
swfdec_net_stream_video_goto (SwfdecNetStream *stream, guint timestamp)
{
  SwfdecBuffer *buffer;
  guint format;
  cairo_surface_t *old;
  gboolean process_events;
  guint process_events_from;

  SWFDEC_LOG ("goto %ums", timestamp);
  process_events = timestamp == stream->next_time;
  process_events_from = MIN (stream->next_time, stream->current_time + 1);
  old = stream->surface;
  if (stream->surface) {
    cairo_surface_destroy (stream->surface);
    stream->surface = NULL;
  }
  if (stream->flvdecoder == NULL)
    return;
  if (stream->flvdecoder->video) {
    buffer = swfdec_flv_decoder_get_video (stream->flvdecoder, timestamp,
	FALSE, &format, &stream->current_time, &stream->next_time);
  } else {
    buffer = NULL;
  }
  if (buffer == NULL) {
    SWFDEC_ERROR ("got no buffer - no video available?");
  } else {
    if (format != stream->format) {
      if (stream->decoder)
	swfdec_video_decoder_free (stream->decoder);
      stream->format = format;
      stream->decoder = NULL;
    }
    swfdec_video_provider_new_image (SWFDEC_VIDEO_PROVIDER (stream));
  }
  if (stream->next_time <= stream->current_time) {
    if (swfdec_flv_decoder_is_eof (stream->flvdecoder)) {
      swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NetStream_Play_Stop, SWFDEC_AS_STR_status);
    } else {
      stream->buffering = TRUE;
      swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NetStream_Buffer_Empty,
	  SWFDEC_AS_STR_status);
    }
    swfdec_net_stream_update_playing (stream);
  }
  if (process_events) {
    while (stream->flvdecoder && process_events_from <= stream->current_time) {
      SwfdecAsValue name, value;
      SwfdecBits bits;
      SwfdecBuffer *event = swfdec_flv_decoder_get_data (stream->flvdecoder, process_events_from, &process_events_from);
      if (!event)
	break;
      SWFDEC_LOG ("processing event from timestamp %u", process_events_from);
      process_events_from++; /* increase so we get the next event next time */
      swfdec_bits_init (&bits, event);
      swfdec_sandbox_use (stream->sandbox);
      if (swfdec_amf_parse (SWFDEC_AS_OBJECT (stream)->context, &bits, 2, 
	    SWFDEC_AMF_STRING, &name, SWFDEC_AMF_MIXED_ARRAY, &value) != 2) {
	SWFDEC_ERROR ("could not parse data tag");
      } else {
	swfdec_as_object_call (SWFDEC_AS_OBJECT (stream), 
	    SWFDEC_AS_VALUE_GET_STRING (&name), 1, &value, NULL);
      }
      swfdec_sandbox_unuse (stream->sandbox);
    }
  }
}

static void
swfdec_net_stream_timeout (SwfdecTimeout *timeout)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM ((void *) ((guchar *) timeout - G_STRUCT_OFFSET (SwfdecNetStream, timeout)));
  SwfdecTick timestamp;

  SWFDEC_LOG ("timeout fired");
  swfdec_net_stream_video_goto (stream, stream->next_time);
  timestamp = stream->timeout.timestamp;
  if (stream->timeout.timestamp == timestamp &&
      stream->timeout.callback) {
    SWFDEC_LOG ("readding timeout");
    stream->timeout.timestamp += SWFDEC_MSECS_TO_TICKS (stream->next_time - stream->current_time);
    swfdec_player_add_timeout (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (stream)->context), &stream->timeout);
  }
}

static void
swfdec_net_stream_update_playing (SwfdecNetStream *stream)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (stream)->context);
  gboolean should_play;
    
  should_play = stream->playing; /* checks user-set play/pause */
  should_play &= !stream->buffering; /* checks enough data is available */
  should_play &= stream->flvdecoder != NULL; /* checks there even is something to play */
  should_play &= stream->next_time > stream->current_time; /* checks if EOF */
  if (should_play && stream->timeout.callback == NULL) {
    SWFDEC_DEBUG ("starting playback");
    stream->timeout.callback = swfdec_net_stream_timeout;
    stream->timeout.timestamp = player->priv->time + SWFDEC_MSECS_TO_TICKS (stream->next_time - stream->current_time);
    swfdec_player_add_timeout (player, &stream->timeout);
    if (stream->flvdecoder->audio) {
      g_assert (stream->audio == NULL);
      SWFDEC_LOG ("starting audio");
      stream->audio = swfdec_audio_flv_new (player, 
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
    /* FIXME: timeout might or might not be be added here if
     * timestamp == player->priv->time, but we'll just remove to be sure */
    swfdec_player_remove_timeout (player, &stream->timeout);
    stream->timeout.callback = NULL;
    SWFDEC_DEBUG ("stopping playback");
  }
}

/*** SWFDEC_STREAM_TARGET interface ***/

static SwfdecPlayer *
swfdec_net_stream_stream_target_get_player (SwfdecStreamTarget *target)
{
  return SWFDEC_PLAYER (SWFDEC_AS_OBJECT (target)->context);
}

static void
swfdec_net_stream_stream_target_error (SwfdecStreamTarget *target, 
    SwfdecStream *stream)
{
  SwfdecNetStream *ns = SWFDEC_NET_STREAM (target);

  if (ns->flvdecoder == NULL)
    swfdec_net_stream_onstatus (ns, SWFDEC_AS_STR_NetStream_Play_StreamNotFound,
	SWFDEC_AS_STR_error);
}

static void
swfdec_net_stream_stream_target_recheck (SwfdecNetStream *stream)
{
  if (stream->buffering) {
    guint first, last;
    if (swfdec_flv_decoder_get_video_info (stream->flvdecoder, &first, &last)) {
      guint current = MAX (first, stream->current_time);
      if (current + stream->buffer_time <= last) {
	swfdec_net_stream_video_goto (stream, current);
	stream->buffering = FALSE;
	swfdec_net_stream_onstatus (stream, SWFDEC_AS_STR_NetStream_Buffer_Full,
	    SWFDEC_AS_STR_status);
      }
    } else {
      SWFDEC_ERROR ("no video stream, how do we update buffering?");
    }
  }
  swfdec_net_stream_update_playing (stream);
}

static gboolean
swfdec_net_stream_stream_target_parse (SwfdecStreamTarget *target, 
    SwfdecStream *stream)
{
  SwfdecNetStream *ns = SWFDEC_NET_STREAM (target);
  SwfdecBufferQueue *queue;
  SwfdecStatus status;
  
  if (ns->flvdecoder == NULL) {
    /* FIXME: add mp3 support */
    ns->flvdecoder = g_object_new (SWFDEC_TYPE_FLV_DECODER, NULL);
    g_signal_connect_swapped (ns->flvdecoder, "missing-plugin", 
	G_CALLBACK (swfdec_player_add_missing_plugin), SWFDEC_AS_OBJECT (ns)->context);
    swfdec_net_stream_onstatus (ns, SWFDEC_AS_STR_NetStream_Play_Start,
	SWFDEC_AS_STR_status);
    swfdec_loader_set_data_type (SWFDEC_LOADER (stream), SWFDEC_LOADER_DATA_FLV);
  }

  status = SWFDEC_STATUS_OK;
  queue = swfdec_stream_get_queue (stream);
  do {
    SwfdecBuffer *buffer = swfdec_buffer_queue_pull_buffer (queue);
    if (buffer == NULL)
      break;
    status &= ~SWFDEC_STATUS_NEEDBITS;
    status |= swfdec_decoder_parse (SWFDEC_DECODER (ns->flvdecoder), buffer);
  } while ((status & (SWFDEC_STATUS_ERROR | SWFDEC_STATUS_EOF | SWFDEC_STATUS_INIT)) == 0);

  if (status & SWFDEC_STATUS_INIT)
    return TRUE;

  if (status & SWFDEC_STATUS_IMAGE)
    swfdec_net_stream_stream_target_recheck (ns);
  return FALSE;
}

static void
swfdec_net_stream_stream_target_close (SwfdecStreamTarget *target, 
    SwfdecStream *stream)
{
  SwfdecNetStream *ns = SWFDEC_NET_STREAM (target);
  guint first, last;

  swfdec_decoder_eof (SWFDEC_DECODER (ns->flvdecoder));
  swfdec_net_stream_onstatus (ns, SWFDEC_AS_STR_NetStream_Buffer_Flush,
      SWFDEC_AS_STR_status);
  if (ns->flvdecoder == NULL)
    return;
  swfdec_net_stream_video_goto (ns, ns->current_time);
  ns->buffering = FALSE;
  if (swfdec_flv_decoder_get_video_info (ns->flvdecoder, &first, &last) &&
      ns->current_time + ns->buffer_time <= last) {
    swfdec_net_stream_onstatus (ns, SWFDEC_AS_STR_NetStream_Buffer_Full,
	SWFDEC_AS_STR_status);
  }
  swfdec_net_stream_stream_target_recheck (ns);
}

static void
swfdec_net_stream_stream_target_init (SwfdecStreamTargetInterface *iface)
{
  iface->get_player = swfdec_net_stream_stream_target_get_player;
  iface->parse = swfdec_net_stream_stream_target_parse;
  iface->close = swfdec_net_stream_stream_target_close;
  iface->error = swfdec_net_stream_stream_target_error;
}

/*** SWFDEC VIDEO PROVIDER ***/

static cairo_surface_t *
swfdec_net_stream_decode_video (SwfdecVideoDecoder *decoder, SwfdecRenderer *renderer,
    SwfdecBuffer *buffer, guint *width, guint *height)
{
  cairo_surface_t *surface;

  if (decoder->codec == SWFDEC_VIDEO_CODEC_VP6 ||
      decoder->codec == SWFDEC_VIDEO_CODEC_VP6_ALPHA) {
    guint wsub, hsub;
    SwfdecBuffer *tmp;
    if (buffer->length == 0) {
      SWFDEC_ERROR ("0-byte VP6 video image buffer?");
      return NULL;
    }
    wsub = *buffer->data;
    hsub = wsub & 0xF;
    wsub >>= 4;
    tmp = swfdec_buffer_new_subbuffer (buffer, 1, buffer->length - 1);
    surface = swfdec_video_decoder_decode (decoder, renderer, tmp, width, height);
    swfdec_buffer_unref (tmp);
    if (hsub || wsub) {
      if (hsub >= *height || wsub >= *width) {
	SWFDEC_ERROR ("can't reduce size by more than available");
	cairo_surface_destroy (surface);
	return NULL;
      }
      *width -= wsub;
      *height -= hsub;
    }
  } else {
    surface = swfdec_video_decoder_decode (decoder, renderer, buffer, width, height);
  }
  return surface;
}

static cairo_surface_t *
swfdec_net_stream_video_provider_get_image (SwfdecVideoProvider *provider,
    SwfdecRenderer *renderer, guint *width, guint *height)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM (provider);
  SwfdecCachedVideo *cached;
  cairo_surface_t *surface;
  SwfdecBuffer *buffer;
  guint next, format;

  cached = SWFDEC_CACHED_VIDEO (swfdec_renderer_get_cache (renderer, stream, NULL, NULL));
  if (cached != NULL && swfdec_cached_video_get_frame (cached) == stream->current_time) {
    swfdec_cached_use (SWFDEC_CACHED (cached));
    swfdec_cached_video_get_size (cached, width, height);
    return swfdec_cached_video_get_surface (cached);
  }

  if (stream->flvdecoder == NULL || stream->flvdecoder->video == NULL)
    return NULL;

  if (stream->decoder != NULL &&
      (stream->decoder_time >= stream->current_time)) {
    swfdec_video_decoder_free (stream->decoder);
    stream->decoder = NULL;
  }
  if (stream->decoder == NULL) {
    buffer = swfdec_flv_decoder_get_video (stream->flvdecoder, 
	stream->current_time, TRUE, &stream->format, &stream->decoder_time,
	&next);
    stream->decoder = swfdec_video_decoder_new (stream->format);
    if (stream->decoder == NULL)
      return NULL;
    format = stream->format;
  } else {
    swfdec_flv_decoder_get_video (stream->flvdecoder, 
	stream->decoder_time, FALSE, NULL, NULL, &next);
    if (next != stream->current_time) {
      guint key_time, key_next;
      buffer = swfdec_flv_decoder_get_video (stream->flvdecoder, 
	  stream->current_time, TRUE, &format, &key_time, &key_next);
      if (key_time > stream->decoder_time) {
	stream->decoder_time = key_time;
	next = key_next;
      } else {
	buffer = swfdec_flv_decoder_get_video (stream->flvdecoder, 
	    next, FALSE, &format, &stream->decoder_time,
	    &next);
      }
    } else {
      buffer = swfdec_flv_decoder_get_video (stream->flvdecoder, 
	  next, FALSE, &format, &stream->decoder_time,
	  &next);
    }
  }

  /* the following things hold:
   * buffer: next buffer to decode
   * format: format of that buffer
   * stream->decoder_time: timestamp of buffer to decode
   * stream->decoder: non-null, using stream->format
   */
  for (;;) {
    if (format != stream->format) {
      swfdec_video_decoder_free (stream->decoder);
      stream->format = format;
      stream->decoder = swfdec_video_decoder_new (stream->format);
      if (stream->decoder == NULL)
	return NULL;
    }
    surface = swfdec_net_stream_decode_video (stream->decoder, renderer,
	buffer, width, height);
    if (surface == NULL)
      return NULL;
    if (stream->decoder_time >= stream->current_time)
      break;

    cairo_surface_destroy (surface);
    buffer = swfdec_flv_decoder_get_video (stream->flvdecoder,
	next, FALSE, &format, &stream->decoder_time, &next);
  }

  cached = swfdec_cached_video_new (surface, *width * *height * 4);
  swfdec_cached_video_set_frame (cached, stream->decoder_time);
  swfdec_cached_video_set_size (cached, *width, *height);
  swfdec_renderer_add_cache (renderer, TRUE, stream, SWFDEC_CACHED (cached));
  g_object_unref (cached);

  return surface;
}

static void
swfdec_net_stream_video_provider_init (SwfdecVideoProviderInterface *iface)
{
  iface->get_image = swfdec_net_stream_video_provider_get_image;
}

/*** SWFDEC_NET_STREAM ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecNetStream, swfdec_net_stream, SWFDEC_TYPE_AS_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_STREAM_TARGET, swfdec_net_stream_stream_target_init)
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_VIDEO_PROVIDER, swfdec_net_stream_video_provider_init))

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
  g_assert (stream->movies == NULL);
  g_free (stream->requested_url);
  stream->requested_url = NULL;

  G_OBJECT_CLASS (swfdec_net_stream_parent_class)->dispose (object);
}

static gboolean
swfdec_net_stream_get_variable (SwfdecAsObject *object, SwfdecAsObject *orig,
    const char *variable, SwfdecAsValue *val, guint *flags)
{
  SwfdecNetStream *stream;

  if (SWFDEC_AS_OBJECT_CLASS (swfdec_net_stream_parent_class)->get (object, orig, variable, val, flags))
    return TRUE;

  stream = SWFDEC_NET_STREAM (object);
  /* FIXME: need case insensitive comparisons? */
  if (variable == SWFDEC_AS_STR_time) {
    guint msecs;
    if (stream->flvdecoder == NULL ||
	!swfdec_flv_decoder_get_video_info (stream->flvdecoder, &msecs, NULL)) {
      SWFDEC_AS_VALUE_SET_INT (val, 0);
    } else {
      if (msecs >= stream->current_time)
	msecs = 0;
      else 
	msecs = stream->current_time - msecs;
      SWFDEC_AS_VALUE_SET_NUMBER (val, msecs / 1000.);
    }
    *flags = 0;
    return TRUE;
  } else if (variable == SWFDEC_AS_STR_bytesLoaded) {
    if (stream->loader == NULL)
      SWFDEC_AS_VALUE_SET_INT (val, 0);
    else
      SWFDEC_AS_VALUE_SET_NUMBER (val, swfdec_loader_get_loaded (stream->loader));
    *flags = 0;
    return TRUE;
  } else if (variable == SWFDEC_AS_STR_bytesTotal) {
    glong bytes;
    if (stream->loader == NULL) {
      bytes = 0;
    } else { 
      bytes = swfdec_loader_get_size (stream->loader);
      if (bytes < 0)
	bytes = swfdec_loader_get_loaded (stream->loader);
    }
    SWFDEC_AS_VALUE_SET_NUMBER (val, bytes);
    *flags = 0;
    return TRUE;
  }
  return FALSE;
}

static void
swfdec_net_stream_mark (SwfdecAsObject *object)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM (object);

  if (stream->conn)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (stream->conn));
  if (stream->sandbox)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (stream->sandbox));

  SWFDEC_AS_OBJECT_CLASS (swfdec_net_stream_parent_class)->mark (object);
}

static void
swfdec_net_stream_class_init (SwfdecNetStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_net_stream_dispose;

  asobject_class->get = swfdec_net_stream_get_variable;
  asobject_class->mark = swfdec_net_stream_mark;
}

static void
swfdec_net_stream_init (SwfdecNetStream *stream)
{
  stream->buffer_time = 100; /* msecs */
}

SwfdecNetStream *
swfdec_net_stream_new (SwfdecNetConnection *conn)
{
  SwfdecAsContext *context;
  SwfdecNetStream *stream;
  
  g_return_val_if_fail (SWFDEC_IS_NET_CONNECTION (conn), NULL);

  context = SWFDEC_AS_OBJECT (conn)->context;
  if (!swfdec_as_context_use_mem (context, sizeof (SwfdecNetStream)))
    return NULL;
  stream = g_object_new (SWFDEC_TYPE_NET_STREAM, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (stream), context, sizeof (SwfdecNetStream));
  stream->conn = conn;

  return stream;
}

static void
swfdec_net_stream_load (SwfdecPlayer *player, gboolean allowed, gpointer streamp)
{
  SwfdecNetStream *stream = streamp;
  SwfdecLoader *loader;

  if (allowed) {
    loader = swfdec_player_load (player, stream->requested_url, NULL);
    swfdec_net_stream_set_loader (stream, loader);
    g_object_unref (loader);
  } else {
    SWFDEC_WARNING ("SECURITY: no access to %s from NetStream",
	stream->requested_url);
    stream->sandbox = NULL;
  }
  g_free (stream->requested_url);
  stream->requested_url = NULL;
}

void
swfdec_net_stream_set_url (SwfdecNetStream *stream, SwfdecSandbox *sandbox, const char *url_string)
{
  SwfdecPlayer *player;
  SwfdecAsContext *cx;
  SwfdecURL *url;

  g_return_if_fail (SWFDEC_IS_NET_STREAM (stream));
  g_return_if_fail (SWFDEC_IS_SANDBOX (sandbox));

  cx = SWFDEC_AS_OBJECT (stream)->context;
  player = SWFDEC_PLAYER (cx);

  if (stream->requested_url != NULL) {
    SWFDEC_FIXME ("can't load %s - already loading %s, what now?", 
	url_string, stream->requested_url);
    return;
  }
  stream->requested_url = g_strdup (url_string);
  stream->sandbox = sandbox;
#if 0
  if (swfdec_url_path_is_relative (url_string)) {
    swfdec_net_stream_load (player, TRUE, stream);
    return;
  }
#endif
  url = swfdec_player_create_url (player, url_string);
  if (url == NULL) {
    swfdec_net_stream_load (player, FALSE, stream);
    return;
  }
  if (swfdec_url_is_local (url)) {
    swfdec_net_stream_load (player, 
	sandbox->type == SWFDEC_SANDBOX_LOCAL_TRUSTED ||
	sandbox->type == SWFDEC_SANDBOX_LOCAL_FILE, stream);
  } else {
    switch (sandbox->type) {
      case SWFDEC_SANDBOX_REMOTE:
	if (swfdec_url_host_equal(url, sandbox->url)) {
	  swfdec_net_stream_load (player, TRUE, stream);
	  break;
	}
	/* fall through */
      case SWFDEC_SANDBOX_LOCAL_NETWORK:
      case SWFDEC_SANDBOX_LOCAL_TRUSTED:
	{
	  SwfdecURL *load_url = swfdec_url_new_components (
	      swfdec_url_get_protocol (url), swfdec_url_get_host (url), 
	      swfdec_url_get_port (url), "crossdomain.xml", NULL);
	  swfdec_player_allow_or_load (player, url, load_url,
	    swfdec_net_stream_load, stream);
	  swfdec_url_free (load_url);
	}
	break;
      case SWFDEC_SANDBOX_LOCAL_FILE:
	swfdec_net_stream_load (player, FALSE, stream);
	break;
      case SWFDEC_SANDBOX_NONE:
      default:
	g_assert_not_reached ();
	break;
    }
  }

  swfdec_url_free (url);
}

void
swfdec_net_stream_set_loader (SwfdecNetStream *stream, SwfdecLoader *loader)
{
  g_return_if_fail (SWFDEC_IS_NET_STREAM (stream));
  g_return_if_fail (loader == NULL || SWFDEC_IS_SANDBOX (stream->sandbox));
  g_return_if_fail (loader == NULL || SWFDEC_IS_LOADER (loader));

  if (stream->loader) {
    SwfdecStream *lstream = SWFDEC_STREAM (stream->loader);
    swfdec_stream_close (lstream);
    swfdec_stream_set_target (lstream, NULL);
    g_object_unref (lstream);
  }
  if (stream->flvdecoder) {
    g_signal_handlers_disconnect_by_func (stream->flvdecoder,
	  swfdec_player_add_missing_plugin, SWFDEC_AS_OBJECT (stream)->context);
    g_object_unref (stream->flvdecoder);
    stream->flvdecoder = NULL;
  }
  stream->loader = loader;
  stream->buffering = TRUE;
  if (loader) {
    g_object_ref (loader);
    swfdec_stream_set_target (SWFDEC_STREAM (loader), SWFDEC_STREAM_TARGET (stream));
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
  if (stream->decoder) {
    swfdec_video_decoder_free (stream->decoder);
    stream->decoder = NULL;
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
    stream->audio = swfdec_audio_flv_new (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (stream)->context), 
	stream->flvdecoder, stream->current_time);
  }
}

