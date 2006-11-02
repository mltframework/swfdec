#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <alsa/asoundlib.h>
#include "swfdec_source.h"

G_BEGIN_DECLS

typedef struct {
  snd_pcm_t *		pcm;
  SwfdecPlayer *	player;
  guint *		sources;	/* sources for writing data */
  guint			n_sources;	/* number of sources */
  guint			offset;		/* offset in samples inside swfdec player */
  SwfdecTime		time;		/* time manager */
} Sound;

#define DEFAULT_DELAY_SAMPLES 441 /* 1/100th of a second */

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

typedef snd_pcm_uframes_t (* WriteFunc) (Sound *sound, 
    const snd_pcm_channel_area_t *dst, 
    snd_pcm_uframes_t offset, 
    snd_pcm_uframes_t avail);

static snd_pcm_uframes_t
write_player (Sound *sound, const snd_pcm_channel_area_t *dst, 
    snd_pcm_uframes_t offset, snd_pcm_uframes_t avail)
{
  /* FIXME: do a long path if this doesn't hold */
  g_assert (dst[1].first - dst[0].first == 16);
  g_assert (dst[0].addr == dst[1].addr);
  g_assert (dst[0].step == dst[1].step);
  g_assert (dst[0].step == 32);

  memset (dst[0].addr + offset * dst[0].step / 8, 0, avail * 4);
  swfdec_player_render_audio (sound->player, dst[0].addr + offset * dst[0].step / 8, 
      sound->offset, avail);
  return avail;
}

static gboolean
try_write (Sound *sound)
{
  snd_pcm_sframes_t avail_result;
  snd_pcm_uframes_t offset, avail;
  const snd_pcm_channel_area_t *dst;

  while (TRUE) {
    avail_result = snd_pcm_avail_update (sound->pcm);
    //g_print ("avail = %d\n", (int) avail_result);
    if (avail_result < 0) {
      g_printerr ("snd_pcm_avail_update failed\n");
      return FALSE;
    }
    if (avail_result == 0)
      return TRUE;
    avail = avail_result;
    ALSA_ERROR (snd_pcm_mmap_begin (sound->pcm, &dst, &offset, &avail),
	"snd_pcm_mmap_begin failed", FALSE);
    //g_print ("  avail = %u\n", (guint) avail);

    avail = write_player (sound, dst, offset, avail);
    if (snd_pcm_mmap_commit (sound->pcm, offset, avail) < 0) {
      g_printerr ("snd_pcm_mmap_commit failed\n");
      return FALSE;
    }
    sound->offset += avail;
    //g_print ("offset: %u (+%u)\n", sound->offset, (guint) avail);
  }
  return TRUE;
}

static void
iterate_before (SwfdecPlayer *player, gpointer data)
{
  Sound *sound = data;
  guint remove;

  swfdec_player_set_audio_advance (sound->player, sound->offset);
  remove = swfdec_player_get_audio_samples (sound->player);
  if (remove > sound->offset) {
    sound->offset = 0;
    //g_print ("underflow (%d frames missing)\n", (int) snd_pcm_avail_update (sound->pcm));
  } else {
    sound->offset -= remove;
  }
  swfdec_time_tick (&sound->time);
  //g_print ("offset is now %u (-%u)\n", sound->offset, remove);
}

static void
swfdec_playback_remove_handlers (Sound *sound)
{
  unsigned int i;

  for (i = 0; i < sound->n_sources; i++) {
    if (sound->sources[i]) {
      g_source_remove (sound->sources[i]);
      sound->sources[i] = 0;
    }
  }
}

static gboolean
handle_sound (GIOChannel *source, GIOCondition cond, gpointer data)
{
  Sound *sound = data;
  snd_pcm_state_t state;

  state = snd_pcm_state (sound->pcm);
  if (state != SND_PCM_STATE_RUNNING)
    swfdec_playback_remove_handlers (sound);
  else
    try_write (sound);
  return TRUE;
}

static void
swfdec_playback_install_handlers (Sound *sound)
{
  if (sound->n_sources > 0) {
    struct pollfd polls[sound->n_sources];
    unsigned int i, count;
    if (sound->n_sources > 1)
      g_printerr ("attention: more than one fd!\n");
    count = snd_pcm_poll_descriptors (sound->pcm, polls, sound->n_sources);
    for (i = 0; i < count; i++) {
      if (sound->sources[i] != 0)
	continue;
      GIOChannel *channel = g_io_channel_unix_new (polls[i].fd);
      sound->sources[i] = g_io_add_watch_full (channel, G_PRIORITY_HIGH,
	  polls[i].events, handle_sound, sound, NULL);
      g_io_channel_unref (channel);
    }
  }
}

static void
iterate_after (SwfdecPlayer *player, gpointer data)
{
  Sound *sound = data;
  GTimeVal now;
  glong delay;

  g_get_current_time (&now);
  delay = swfdec_time_get_difference (&sound->time, &now);

  /* FIXME: invent a better way to detect when to recover */
  /* this checks if the next frame follows without delay */
  //g_print ("delay: %ld - length: %g\n", delay, 1000 / swfdec_player_get_rate (sound->player));
  if (delay < 1000 / swfdec_player_get_rate (sound->player)) {
    snd_pcm_state_t state = snd_pcm_state (sound->pcm);
    switch (state) {
      case SND_PCM_STATE_XRUN:
	ALSA_ERROR (snd_pcm_prepare (sound->pcm), "no prepare",);
	//g_print ("XRUN!\n");
	/* fall through */
      case SND_PCM_STATE_SUSPENDED:
      case SND_PCM_STATE_PREPARED:
	sound->offset = delay <= 0 ? 0 : delay * 44100 / 1000;
	if (try_write (sound)) {
	  ALSA_ERROR (snd_pcm_start (sound->pcm), "error starting",);
	  swfdec_playback_install_handlers (sound);
	}
	break;
      default:
	break;
    }
  }
}

gpointer
swfdec_playback_open (SwfdecPlayer *player)
{
  snd_pcm_t *ret;
  snd_pcm_hw_params_t *hw_params;
  unsigned int rate;
  Sound *sound;
  snd_pcm_uframes_t uframes;
  GTimeVal tv;

  /* "default" uses dmix, and dmix ticks way slow, so this thingy here stutters */
  ALSA_ERROR (snd_pcm_open (&ret, FALSE ? "default" : "plughw:0", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK),
      "Failed to open sound device", NULL);

  snd_pcm_hw_params_alloca (&hw_params);
  if (snd_pcm_hw_params_any (ret, hw_params) < 0) {
    g_printerr ("No sound format available\n");
    return NULL;
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
  sound = g_new0 (Sound, 1);
  sound->pcm = ret;
  sound->player = g_object_ref (player);
  sound->n_sources = snd_pcm_poll_descriptors_count (ret);
  if (sound->n_sources > 0)
    sound->sources = g_new0 (guint, sound->n_sources);
  g_get_current_time (&tv);
  swfdec_time_init (&sound->time, &tv, swfdec_player_get_rate (player));
  g_signal_connect (player, "iterate", G_CALLBACK (iterate_before), sound);
  g_signal_connect_after (player, "iterate", G_CALLBACK (iterate_after), sound);

  if (snd_pcm_prepare (ret) < 0) {
    g_printerr ("no prepare\n");
  }

  return sound;

fail:
  snd_pcm_close (ret);
  return NULL;
}

void
swfdec_playback_close (gpointer data)
{
  Sound *sound = data;

  ALSA_TRY (snd_pcm_close (sound->pcm), "failed closing");
  if (g_signal_handlers_disconnect_by_func (sound->player, 
	G_CALLBACK (iterate_before), sound) != 1) {
    g_assert_not_reached ();
  }
  if (g_signal_handlers_disconnect_by_func (sound->player, 
	G_CALLBACK (iterate_after), sound) != 1) {
    g_assert_not_reached ();
  }
  if (sound->sources) {
    swfdec_playback_remove_handlers (sound);
    g_free (sound->sources);
  }
  g_free (sound);
}
