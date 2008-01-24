/* Swfdec
 * Copyright © 2006 Benjamin Otte <otte@gnome.org>
 * Copyright © 2007 Eric Anholt <eric@anholt.net>
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

#include <stdlib.h>
#include <string.h>

#include "swfdec_playback.h"
#include "pulse/pulseaudio.h"
#include "pulse/glib-mainloop.h"

/** @file Implements swfdec audio playback by dumping swfdec streams out
 * using pulseaudio streams.
 */

/*** DEFINITIONS ***/

struct _SwfdecPlayback {
  SwfdecPlayer *	player;
  GList *		streams;	/* all Stream objects */
  GMainContext *	context;	/* glib context we work in */
  pa_glib_mainloop *	pa_mainloop;	/* PA to glib mainloop connection */
  pa_context *		pa;		/* PA context for sound rendering */
};

typedef struct {
  SwfdecPlayback *     	sound;		/* reference to sound object */
  SwfdecAudio *		audio;		/* the audio we play back */
  guint			offset;		/* offset into sound */
  pa_stream *		pa;		/* PA stream */
  pa_cvolume		volume;		/* Volume control.  Not yet used. */
  gboolean		no_more;
} Stream;

/* Size of one of our audio samples, in bytes */
#define SAMPLESIZE	2
#define CHANNELS	2

/*** STREAMS ***/

static void
stream_write_callback (pa_stream *pa,
		       size_t bytes,
		       void *data)
{
  Stream *stream = data;
  char *frag;
  unsigned int samples = bytes / SAMPLESIZE / CHANNELS;
  int err;

  if (stream->no_more)
    return;

  /* Adjust to our rounded-down number */
  bytes = samples * SAMPLESIZE * CHANNELS;

  frag = malloc (bytes);
  if (frag == NULL) {
    g_printerr ("Failed to allocate fragment of size %d\n", (int)bytes);
    return;
  }

  /* Set up our fragment and render swfdec's audio into it. The swfdec audio
   * decoder renders deltas from the existing data in the fragment.
   */
  memset (frag, 0, bytes);
  swfdec_audio_render (stream->audio, (gint16 *)frag, stream->offset,
		       samples);

  /* Send the new fragment out the PA stream */
  err = pa_stream_write (pa, frag, bytes, NULL, 0, PA_SEEK_RELATIVE);
  if (err != 0) {
    g_printerr ("Failed to write fragment to PA stream: %s\n",
		pa_strerror(pa_context_errno(stream->sound->pa)));
  }

  /* Advance playback pointer */
  stream->offset += samples;

  free(frag);
}

static void
stream_drain_complete (pa_stream *pa, int success, void *data)
{
  Stream *stream = data;

  pa_stream_disconnect (stream->pa);
  pa_stream_unref (stream->pa);
  g_object_unref (stream->audio);
  g_free (stream);
}

static void
swfdec_playback_stream_close (Stream *stream)
{
  /* Pull it off of the active stream list. */
  stream->sound->streams = g_list_remove (stream->sound->streams, stream);

  /* If we have created a PA stream, defer freeing until we drain it. */
  if (stream->pa != NULL) {
    stream->no_more = 1;
    pa_operation_unref (pa_stream_drain (stream->pa,
					 stream_drain_complete,
					 stream));
  } else {
    g_object_unref (stream->audio);
    g_free (stream);
  }
}

static void
stream_state_callback (pa_stream *pa, void *data)
{
  switch (pa_stream_get_state(pa)) {
  case PA_STREAM_CREATING:
  case PA_STREAM_TERMINATED:
  case PA_STREAM_READY:
  case PA_STREAM_UNCONNECTED:
    break;

  case PA_STREAM_FAILED:
    g_printerr("PA stream failed: %s\n",
	       pa_strerror(pa_context_errno(pa_stream_get_context(pa))));
  default:
    break;
  }
}

static void
swfdec_playback_stream_open (SwfdecPlayback *sound, SwfdecAudio *audio)
{
  Stream *stream;
  pa_sample_spec spec = {
    .format = PA_SAMPLE_S16LE,
    .rate = 44100,
    .channels = CHANNELS,
  };
  int err;

  stream = g_new0 (Stream, 1);
  stream->sound = sound;
  stream->audio = g_object_ref (audio);
  sound->streams = g_list_prepend (sound->streams, stream);

  /* If we failed to initialize the context, don't try to create the stream.
   * We still have to get put in the list, because swfdec_playback.c expects
   * to find it in the list for removal later.
   */
  if (sound->pa == NULL)
    return;

  /* Create our stream */
  stream->pa = pa_stream_new(sound->pa,
			     "swfdec stream",
			     &spec,
			     NULL /* Default channel map */
			     );
  if (stream->pa == NULL) {
    g_printerr("Failed to create PA stream\n");
    swfdec_playback_stream_close(stream);
    return;
  }

  /* Start at default volume */
  pa_cvolume_set(&stream->volume, CHANNELS, PA_VOLUME_NORM);

  /* Hook up our stream write callback for when new data is needed */
  pa_stream_set_state_callback(stream->pa, stream_state_callback, stream);
  pa_stream_set_write_callback(stream->pa, stream_write_callback, stream);

  /* Connect it up as a playback stream. */
  err = pa_stream_connect_playback(stream->pa,
				   NULL, /* Default device */
				   NULL /* Default buffering */,
				   0, /* No flags */
				   &stream->volume,
				   NULL /* Don't sync to any stream */
				   );
  if (err != 0) {
    g_printerr ("Failed to connect PA stream: %s\n",
		pa_strerror(pa_context_errno(sound->pa)));
    swfdec_playback_stream_close(stream);
    return;
  }
}

/*** SOUND ***/

static void
advance_before (SwfdecPlayer *player, guint msecs, guint audio_samples, gpointer data)
{
  SwfdecPlayback *sound = data;
  GList *walk;

  for (walk = sound->streams; walk; walk = walk->next) {
    Stream *stream = walk->data;
    if (audio_samples >= stream->offset) {
      stream->offset = 0;
    } else {
      stream->offset -= audio_samples;
    }
  }
}
 
static void
audio_added (SwfdecPlayer *player, SwfdecAudio *audio, SwfdecPlayback *sound)
{
  swfdec_playback_stream_open (sound, audio);
}

static void
audio_removed (SwfdecPlayer *player, SwfdecAudio *audio, SwfdecPlayback *sound)
{
  GList *walk;

  for (walk = sound->streams; walk; walk = walk->next) {
    Stream *stream = walk->data;
    if (stream->audio == audio) {
      swfdec_playback_stream_close (stream);
      return;
    }
  }
  g_assert_not_reached ();
}
static void
context_state_callback (pa_context *pa, void *data)
{
  SwfdecPlayback *sound = data;

  switch (pa_context_get_state(pa)) {
  case PA_CONTEXT_FAILED:
    g_printerr ("PA context failed\n");
    pa_context_unref (pa);
    sound->pa = NULL;
    break;

  default:
  case PA_CONTEXT_TERMINATED:
  case PA_CONTEXT_UNCONNECTED:
  case PA_CONTEXT_CONNECTING:
  case PA_CONTEXT_AUTHORIZING:
  case PA_CONTEXT_SETTING_NAME:
  case PA_CONTEXT_READY:
    break;

  }
}

SwfdecPlayback *
swfdec_playback_open (SwfdecPlayer *player, GMainContext *context)
{
  SwfdecPlayback *sound;
  const GList *walk;
  pa_mainloop_api *pa_api;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (context != NULL, NULL);

  sound = g_new0 (SwfdecPlayback, 1);
  sound->player = player;
  g_signal_connect (player, "advance", G_CALLBACK (advance_before), sound);
  g_signal_connect (player, "audio-added", G_CALLBACK (audio_added), sound);
  g_signal_connect (player, "audio-removed", G_CALLBACK (audio_removed), sound);

  /* Create our mainloop attachment to glib.  XXX: I hope this means we don't
   * have to run the main loop using pa functions.
   */
  sound->pa_mainloop = pa_glib_mainloop_new (context);
  pa_api = pa_glib_mainloop_get_api (sound->pa_mainloop);

  sound->pa = pa_context_new (pa_api, "swfdec");

  pa_context_set_state_callback (sound->pa, context_state_callback, sound);
  pa_context_connect (sound->pa,
		      NULL, /* default server */
		      0, /* default flags */
		      NULL /* spawning api */
		      );

  for (walk = swfdec_player_get_audio (player); walk; walk = walk->next) {
    swfdec_playback_stream_open (sound, walk->data);
  }
  g_main_context_ref (context);
  sound->context = context;
  return sound;
}

static void
context_drain_complete (pa_context *pa, void *data)
{
  pa_context_disconnect (pa);
  pa_context_unref (pa);
}

void
swfdec_playback_close (SwfdecPlayback *sound)
{
  pa_operation *op;

#define REMOVE_HANDLER_FULL(obj,func,data,count) G_STMT_START {\
  if (g_signal_handlers_disconnect_by_func ((obj), \
	G_CALLBACK (func), (data)) != (count)) { \
    g_assert_not_reached (); \
  } \
} G_STMT_END
#define REMOVE_HANDLER(obj,func,data) REMOVE_HANDLER_FULL (obj, func, data, 1)

  while (sound->streams)
    swfdec_playback_stream_close (sound->streams->data);
  REMOVE_HANDLER (sound->player, advance_before, sound);
  REMOVE_HANDLER (sound->player, audio_added, sound);
  REMOVE_HANDLER (sound->player, audio_removed, sound);

  if (sound->pa != NULL) {
    op = pa_context_drain (sound->pa, context_drain_complete, NULL);
    if (op == NULL) {
      pa_context_disconnect (sound->pa);
      pa_context_unref (sound->pa);
    } else {
      pa_operation_unref (op);
    }
    pa_glib_mainloop_free (sound->pa_mainloop);
  }

  g_main_context_unref (sound->context);
  g_free (sound);
}


