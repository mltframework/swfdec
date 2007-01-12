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

#include "swfdec_net_stream.h"
#include "swfdec_audio_flv.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"

static void
swfdec_net_stream_video_goto (SwfdecNetStream *stream, guint timestamp)
{
  SwfdecBuffer *buffer;
  SwfdecVideoFormat format;

  SWFDEC_LOG ("goto %ums\n", timestamp);
  buffer = swfdec_flv_decoder_get_video (stream->flvdecoder, timestamp,
      FALSE, &format, &stream->current_time, &stream->next_time);
  if (stream->surface) {
    cairo_surface_destroy (stream->surface);
    stream->surface = NULL;
  }
  if (buffer == NULL) {
    SWFDEC_ERROR ("got no buffer?!");
  } else {
    if (format != stream->format) {
      if (stream->decoder)
	swfdec_video_codec_finish (stream->codec, stream->decoder);
      stream->format = format;
      stream->codec = swfdec_codec_get_video (format);
      if (stream->codec)
	stream->decoder = swfdec_video_codec_init (stream->codec);
      else
	stream->decoder = NULL;
    }
    if (stream->decoder) {
      SwfdecBuffer *decoded = swfdec_video_codec_decode (stream->codec, stream->decoder, buffer);
      if (decoded) {
	static const cairo_user_data_key_t key;
	guint w, h;
	if (!swfdec_video_codec_get_size (stream->codec, stream->decoder, &w, &h)) {
	    g_assert_not_reached ();
	}
	stream->surface = cairo_image_surface_create_for_data (decoded->data, 
	    CAIRO_FORMAT_ARGB32, w, h, w * 4);
	cairo_surface_set_user_data (stream->surface, &key, 
	    decoded, (cairo_destroy_func_t) swfdec_buffer_unref);
      }
    }
  }
  if (stream->input.movie)
    swfdec_movie_invalidate (SWFDEC_MOVIE (stream->input.movie));
}

static void
swfdec_net_stream_timeout (SwfdecTimeout *timeout)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM ((guchar *) timeout - G_STRUCT_OFFSET (SwfdecNetStream, timeout));

  SWFDEC_LOG ("timeout fired");
  swfdec_net_stream_video_goto (stream, stream->next_time);
  if (stream->next_time > stream->current_time) {
    SWFDEC_LOG ("readding timeout");
    stream->timeout.timestamp += SWFDEC_MSECS_TO_TICKS (stream->next_time - stream->current_time);
    swfdec_player_add_timeout (stream->player, &stream->timeout);
  } else {
    stream->timeout.callback = NULL;
  }
}

static void
swfdec_net_stream_update_playing (SwfdecNetStream *stream)
{
  gboolean should_play;
    
  should_play = stream->playing;
  should_play &= stream->flvdecoder != NULL;
  should_play &= stream->next_time > stream->current_time;
  if (should_play && stream->timeout.callback == NULL) {
    SWFDEC_DEBUG ("starting playback");
    stream->timeout.callback = swfdec_net_stream_timeout;
    stream->timeout.timestamp = stream->player->time + SWFDEC_MSECS_TO_TICKS (stream->next_time - stream->current_time);
    swfdec_player_add_timeout (stream->player, &stream->timeout);
    if (stream->flvdecoder->audio) {
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

static SwfdecDecoder *
swfdec_net_stream_loader_target_get_decoder (SwfdecLoaderTarget *target)
{
  return SWFDEC_DECODER (SWFDEC_NET_STREAM (target)->flvdecoder);
}

static gboolean
swfdec_net_stream_loader_target_set_decoder (SwfdecLoaderTarget *target,
    SwfdecDecoder *decoder)
{
  if (!SWFDEC_IS_FLV_DECODER (decoder)) {
    g_object_unref (decoder);
    return FALSE;
  }
  SWFDEC_NET_STREAM (target)->flvdecoder = SWFDEC_FLV_DECODER (decoder);
  return TRUE;
}

static gboolean
swfdec_net_stream_loader_target_image (SwfdecLoaderTarget *target)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM (target);
  guint current, next;
  SwfdecBuffer *buffer;
  SwfdecVideoFormat format;

  if (!stream->playing)
    return TRUE;

  buffer = swfdec_flv_decoder_get_video (stream->flvdecoder, 
      stream->current_time, FALSE, &format, &current, &next);
  if (format != stream->format ||
      stream->current_time != current ||
      stream->next_time != next)
    swfdec_net_stream_video_goto (stream, current);
  swfdec_net_stream_update_playing (stream);

  return TRUE;
}

static void
swfdec_net_stream_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_net_stream_loader_target_get_player;
  iface->get_decoder = swfdec_net_stream_loader_target_get_decoder;
  iface->set_decoder = swfdec_net_stream_loader_target_set_decoder;

  iface->init = swfdec_net_stream_loader_target_image;
  iface->image = swfdec_net_stream_loader_target_image;
}

/*** SWFDEC VIDEO MOVIE INPUT ***/

static cairo_surface_t *
swfdec_net_stream_input_get_image (SwfdecVideoMovieInput *input)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM ((guchar *) input - G_STRUCT_OFFSET (SwfdecNetStream, input));

  return stream->surface;
}

static void
swfdec_net_stream_input_finalize (SwfdecVideoMovieInput *input)
{
  SwfdecNetStream *stream = SWFDEC_NET_STREAM ((guchar *) input - G_STRUCT_OFFSET (SwfdecNetStream, input));

  g_object_unref (stream);
}

/*** SWFDEC_NET_STREAM ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecNetStream, swfdec_net_stream, G_TYPE_OBJECT,
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
  if (stream->decoder)
    swfdec_video_codec_finish (stream->codec, stream->decoder);
  swfdec_net_stream_set_loader (stream, NULL);

  G_OBJECT_CLASS (swfdec_net_stream_parent_class)->dispose (object);
}

static void
swfdec_net_stream_class_init (SwfdecNetStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_net_stream_dispose;
}

static void
swfdec_net_stream_init (SwfdecNetStream *stream)
{
  stream->input.get_image = swfdec_net_stream_input_get_image;
  stream->input.finalize = swfdec_net_stream_input_finalize;
}

SwfdecNetStream *
swfdec_net_stream_new (SwfdecPlayer *player)
{
  SwfdecNetStream *stream = g_object_new (SWFDEC_TYPE_NET_STREAM, NULL);

  stream->player = player;

  return stream;
}

void
swfdec_net_stream_set_loader (SwfdecNetStream *stream, SwfdecLoader *loader)
{
  g_return_if_fail (SWFDEC_IS_NET_STREAM (stream));
  g_return_if_fail (loader == NULL || SWFDEC_IS_LOADER (loader));

  if (stream->loader) {
    g_object_unref (stream->loader);
  }
  if (stream->flvdecoder) {
    g_object_unref (stream->flvdecoder);
    stream->flvdecoder = NULL;
  }
  stream->loader = loader;
  if (loader) {
    g_object_ref (loader);
    swfdec_loader_set_target (loader, SWFDEC_LOADER_TARGET (stream));
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

