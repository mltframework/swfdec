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

#include <alsa/asoundlib.h>
#include "swfdec_source.h"

/* Why ALSA sucks for beginners:
 * - snd_pcm_delay is not sample-exact, but period-exact most of the time.
 *   Yay for getting told the time every 512 samples when a human notices
 *   a delay of 100 samples (oooops)
 * - lots of functions are simply not implemented. So the super-smart idea
 *   of using snd_pcm_rewind to avoid XRUNS and still get low latency has
 *   some issues when dmix just returns -EIO all of the time. That wouldn't
 *   be so bad if there was actually a way to query if it's supported.
 * - But to make up for all this, you have 10 hardware parameters, 10 
 *   software parameters and 10 configuration parameters. All of this is
 *   naturally supported on 10 driver APIs depending on kernel. So if your
 *   sound card seems to do weird stuff, that is not my fault.
 * Welcome to Linux sound in the 21st century.
 */

/*** DEFINITIONS ***/

typedef struct {
  SwfdecPlayer *	player;
  SwfdecTime		time;		/* time manager */
  GList *		streams;	/* all Stream objects */
  GList *		waiting;	/* Stream objects waiting to get started */
  gulong		notify_cb;	/* callback id for notify::initialized */
  GMainContext *	context;	/* context we work in */
} Sound;

typedef struct {
  Sound *		sound;		/* reference to sound object */
  SwfdecAudio *		audio;		/* the audio we play back */
  snd_pcm_t *		pcm;		/* the pcm we play back to */
  GSource **		sources;	/* sources for writing data */
  guint			n_sources;	/* number of sources */
  guint			offset;		/* offset into sound */
} Stream;

#define ALSA_TRY(func,msg) G_STMT_START{ \
  int err = func; \
  if (err < 0) \
    g_printerr (msg ": %s\n", snd_strerror (err)); \
}G_STMT_END

#define ALSA_ERROR(func,msg,retval) G_STMT_START { \
  int err = func; \
  if (err < 0) { \
    g_printerr (msg ": %s\n", snd_strerror (err)); \
    return retval; \
  } \
}G_STMT_END

/*** STREAMS ***/

static snd_pcm_uframes_t
write_player (Stream *stream, const snd_pcm_channel_area_t *dst, 
    snd_pcm_uframes_t offset, snd_pcm_uframes_t avail)
{
  /* FIXME: do a long path if this doesn't hold */
  g_assert (dst[1].first - dst[0].first == 16);
  g_assert (dst[0].addr == dst[1].addr);
  g_assert (dst[0].step == dst[1].step);
  g_assert (dst[0].step == 32);

  memset (dst[0].addr + offset * dst[0].step / 8, 0, avail * 4);
  swfdec_audio_render (stream->audio, dst[0].addr + offset * dst[0].step / 8, 
      stream->offset, avail);
  //g_print ("rendering %u %u\n", stream->offset, (guint) avail);
  return avail;
}

static gboolean
try_write (Stream *stream)
{
  snd_pcm_sframes_t avail_result;
  snd_pcm_uframes_t offset, avail;
  const snd_pcm_channel_area_t *dst;

  while (TRUE) {
    avail_result = snd_pcm_avail_update (stream->pcm);
    ALSA_ERROR (avail_result, "snd_pcm_avail_update failed", FALSE);
    if (avail_result == 0)
      return TRUE;
    avail = avail_result;
    ALSA_ERROR (snd_pcm_mmap_begin (stream->pcm, &dst, &offset, &avail),
	"snd_pcm_mmap_begin failed", FALSE);
    //g_print ("  avail = %u\n", (guint) avail);

    avail = write_player (stream, dst, offset, avail);
    if (snd_pcm_mmap_commit (stream->pcm, offset, avail) < 0) {
      g_printerr ("snd_pcm_mmap_commit failed\n");
      return FALSE;
    }
    stream->offset += avail;
    //g_print ("offset: %u (+%u)\n", stream->offset, (guint) avail);
  }
  return TRUE;
}

static void
swfdec_stream_remove_handlers (Stream *stream)
{
  unsigned int i;

  for (i = 0; i < stream->n_sources; i++) {
    if (stream->sources[i]) {
      g_source_destroy (stream->sources[i]);
      g_source_unref (stream->sources[i]);
      stream->sources[i] = NULL;
    }
  }
  if (g_list_find (stream->sound->waiting, stream) == NULL)
    stream->sound->waiting = g_list_prepend (stream->sound->waiting, stream);
}

static gboolean
handle_stream (GIOChannel *source, GIOCondition cond, gpointer data)
{
  Stream *stream = data;
  snd_pcm_state_t state;

  state = snd_pcm_state (stream->pcm);
  if (state != SND_PCM_STATE_RUNNING)
    swfdec_stream_remove_handlers (stream);
  else
    try_write (stream);
  return TRUE;
}

static void
swfdec_stream_install_handlers (Stream *stream)
{
  if (stream->n_sources > 0) {
    struct pollfd polls[stream->n_sources];
    unsigned int i, count;
    if (stream->n_sources > 1)
      g_printerr ("attention: more than one fd!\n");
    count = snd_pcm_poll_descriptors (stream->pcm, polls, stream->n_sources);
    for (i = 0; i < count; i++) {
      if (stream->sources[i] != NULL)
	continue;
      GIOChannel *channel = g_io_channel_unix_new (polls[i].fd);
      stream->sources[i] = g_io_create_watch (channel, polls[i].events);
      g_source_set_priority (stream->sources[i], G_PRIORITY_HIGH);
      g_source_set_callback (stream->sources[i], (GSourceFunc) handle_stream, stream, NULL);
      g_io_channel_unref (channel);
      g_source_attach (stream->sources[i], stream->sound->context);
    }
  }
  stream->sound->waiting = g_list_remove (stream->sound->waiting, stream);
}

static void
swfdec_stream_start (Stream *stream, guint delay_samples)
{
  snd_pcm_state_t state = snd_pcm_state (stream->pcm);
  switch (state) {
    case SND_PCM_STATE_XRUN:
      ALSA_ERROR (snd_pcm_prepare (stream->pcm), "no prepare",);
      //g_print ("XRUN!\n");
      /* fall through */
    case SND_PCM_STATE_SUSPENDED:
    case SND_PCM_STATE_PREPARED:
      stream->offset = delay_samples;
      //g_print ("offset: %u (delay: %ld)\n", sound->offset, delay);
      if (try_write (stream)) {
	ALSA_ERROR (snd_pcm_start (stream->pcm), "error starting",);
	swfdec_stream_install_handlers (stream);
      }
      break;
    default:
      break;
  }
}

static void
swfdec_stream_open (Sound *sound, SwfdecAudio *audio)
{
  Stream *stream;
  snd_pcm_t *ret;
  snd_pcm_hw_params_t *hw_params;
  unsigned int rate;
  snd_pcm_uframes_t uframes;

  /* "default" uses dmix, and dmix ticks way slow, so this thingy here stutters */
  ALSA_ERROR (snd_pcm_open (&ret, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK),
      "Failed to open sound device", );

  snd_pcm_hw_params_alloca (&hw_params);
  if (snd_pcm_hw_params_any (ret, hw_params) < 0) {
    g_printerr ("No sound format available\n");
    return;
  }
  if (snd_pcm_hw_params_set_access (ret, hw_params, SND_PCM_ACCESS_MMAP_INTERLEAVED) < 0) {
    g_printerr ("Failed setting access\n");
    goto fail;
  }
  if (snd_pcm_hw_params_set_format (ret, hw_params, SND_PCM_FORMAT_S16) < 0) {
    g_printerr ("Failed setting format\n");
    goto fail;
  }
  if (snd_pcm_hw_params_set_channels (ret, hw_params, 2) < 0) {
    g_printerr ("Failed setting channels\n");
    goto fail;
  }
  rate = 44100;
  if (snd_pcm_hw_params_set_rate_near (ret, hw_params, &rate, 0) < 0) {
    g_printerr ("Failed setting rate\n");
    goto fail;
  }
  uframes = 16384;
  if (snd_pcm_hw_params_set_buffer_size_near (ret, hw_params, &uframes) < 0) {
    g_printerr ("Failed setting buffer size\n");
    goto fail;
  }
  if (snd_pcm_hw_params (ret, hw_params) < 0) {
    g_printerr ("Could not set hardware parameters\n");
    goto fail;
  }
#if 0
  {
    snd_output_t *log;
    snd_output_stdio_attach (&log, stderr, 0);
    snd_pcm_hw_params_dump (hw_params, log);
  }
#endif
  stream = g_new0 (Stream, 1);
  stream->sound = sound;
  stream->audio = g_object_ref (audio);
  stream->pcm = ret;
  stream->n_sources = snd_pcm_poll_descriptors_count (ret);
  if (stream->n_sources > 0)
    stream->sources = g_new0 (GSource *, stream->n_sources);
  sound->streams = g_list_prepend (sound->streams, stream);
  sound->waiting = g_list_prepend (sound->waiting, stream);
  return;

fail:
  snd_pcm_close (ret);
}

static void
swfdec_stream_close (Stream *stream)
{
  ALSA_TRY (snd_pcm_close (stream->pcm), "failed closing");
  swfdec_stream_remove_handlers (stream);
  g_free (stream->sources);
  stream->sound->streams = g_list_remove (stream->sound->streams, stream);
  stream->sound->waiting = g_list_remove (stream->sound->waiting, stream);
  g_object_unref (stream->audio);
  g_free (stream);
}

/*** SOUND ***/

static void
iterate_before (SwfdecPlayer *player, gpointer data)
{
  Sound *sound = data;
  guint remove;
  GList *walk;

  remove = swfdec_player_get_audio_samples (sound->player);
  for (walk = sound->streams; walk; walk = walk->next) {
    Stream *stream = walk->data;
    if (remove > stream->offset) {
      stream->offset = 0;
    } else {
      stream->offset -= remove;
    }
  }
  swfdec_time_tick (&sound->time);
}

static void
do_after (SwfdecPlayer *player, Sound *sound)
{
  GTimeVal now;
  glong delay;

  if (sound->waiting == NULL)
    return;
  g_get_current_time (&now);
  delay = swfdec_time_get_difference (&sound->time, &now);
  if (delay > 0) {
    return;
  }

  delay = swfdec_player_get_audio_samples (sound->player)
      + delay * 44100 / 1000;
  delay = MAX (delay, 0);
  while (sound->waiting)
    swfdec_stream_start (sound->waiting->data, delay);
}

static gboolean
handle_mouse_after (SwfdecPlayer *player, double x, double y, int button, Sound *sound)
{
  do_after (player, sound);

  return FALSE;
}

static void
audio_added (SwfdecPlayer *player, SwfdecAudio *audio, Sound *sound)
{
  swfdec_stream_open (sound, audio);
}

static void
audio_removed (SwfdecPlayer *player, SwfdecAudio *audio, Sound *sound)
{
  GList *walk;

  for (walk = sound->streams; walk; walk = walk->next) {
    Stream *stream = walk->data;
    if (stream->audio == audio) {
      swfdec_stream_close (stream);
      return;
    }
  }
  g_assert_not_reached ();
}

static void
swfdec_playback_initialized (SwfdecPlayer *player, GParamSpec *pspec, Sound *sound)
{
  GTimeVal tv;
  g_get_current_time (&tv);
  swfdec_time_init (&sound->time, &tv, swfdec_player_get_rate (player));
  g_signal_handler_disconnect (sound->player, sound->notify_cb);
  sound->notify_cb = 0;
}

gpointer
swfdec_playback_open (SwfdecPlayer *player, GMainContext *context)
{
  Sound *sound;
  const GList *walk;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (context != NULL, NULL);

  sound = g_new0 (Sound, 1);
  sound->player = g_object_ref (player);
  g_signal_connect (player, "iterate", G_CALLBACK (iterate_before), sound);
  g_signal_connect_after (player, "iterate", G_CALLBACK (do_after), sound);
  g_signal_connect_after (player, "handle-mouse", G_CALLBACK (handle_mouse_after), sound);
  g_signal_connect (player, "audio-added", G_CALLBACK (audio_added), sound);
  g_signal_connect (player, "audio-removed", G_CALLBACK (audio_removed), sound);
  for (walk = swfdec_player_get_audio (player); walk; walk = walk->next) {
    swfdec_stream_open (sound, walk->data);
  }
  if (swfdec_player_is_initialized (player)) {
    GTimeVal tv;
    g_get_current_time (&tv);
    swfdec_time_init (&sound->time, &tv, swfdec_player_get_rate (player));
  } else {
    sound->notify_cb = g_signal_connect (player, "notify::initialized",
	G_CALLBACK (swfdec_playback_initialized), sound);
  }
  g_main_context_ref (context);
  sound->context = context;
  return sound;
}

void
swfdec_playback_close (gpointer data)
{
  Sound *sound = data;
#define REMOVE_HANDLER_FULL(obj,func,data,count) G_STMT_START {\
  if (g_signal_handlers_disconnect_by_func ((obj), \
	G_CALLBACK (func), (data)) != (count)) { \
    g_assert_not_reached (); \
  } \
} G_STMT_END
#define REMOVE_HANDLER(obj,func,data) REMOVE_HANDLER_FULL (obj, func, data, 1)

  while (sound->streams)
    swfdec_stream_close (sound->streams->data);
  if (sound->notify_cb) {
    g_signal_handler_disconnect (sound->player, sound->notify_cb);
  }
  REMOVE_HANDLER (sound->player, iterate_before, sound);
  REMOVE_HANDLER (sound->player, do_after, sound);
  REMOVE_HANDLER (sound->player, handle_mouse_after, sound);
  REMOVE_HANDLER (sound->player, audio_added, sound);
  REMOVE_HANDLER (sound->player, audio_removed, sound);
  g_object_unref (sound->player);
  g_main_context_unref (sound->context);
  g_free (sound);
}


