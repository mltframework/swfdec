#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <alsa/asoundlib.h>
#include "swfdec_sound.h"
#include "swfdec_buffer.h"

G_BEGIN_DECLS

typedef struct {
  snd_pcm_t *		pcm;
  GList *		channels;
  guint *		sources;
  GSourceFunc		func;
  gpointer		func_data;
  GList *		buffers;
  guint			timeout;
} Sound;

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

static void
do_the_write_thing (Sound *sound)
{
  snd_pcm_state_t state;
  int err;
  SwfdecBuffer *buffer;

retry:
  if (sound->buffers == NULL) {
    if (!sound->func (sound->func_data))
      return;
    if (sound->buffers == NULL) {
      g_printerr ("callback didn't fill buffer!\n");
      return;
    }
  }
  buffer = sound->buffers->data;
  err = snd_pcm_writei (sound->pcm, buffer->data, buffer->length / 4);
  //g_print ("write returned %d\n", err);
  state = snd_pcm_state (sound->pcm);
  if (state == SND_PCM_STATE_SUSPENDED || state == SND_PCM_STATE_PREPARED) {
    ALSA_ERROR (snd_pcm_start (sound->pcm), "error starting",);
  }
  if (err >= (int) (buffer->length / 4)) {
    swfdec_buffer_unref (buffer);
    sound->buffers = g_list_remove (sound->buffers, buffer);
    goto retry;
  }
  if (err > 0) {
    SwfdecBuffer *buf;
    err *= 4;
    buf = swfdec_buffer_new_subbuffer (buffer, err, buffer->length - err);
    swfdec_buffer_unref (buffer);
    sound->buffers->data = buf;
    return;
  }
  if (err == -EAGAIN)
    return;
  if (err == -EPIPE) {
    if (state == SND_PCM_STATE_XRUN) {
      ALSA_ERROR (snd_pcm_prepare (sound->pcm), "no prepare",);
      goto retry;
    }
  }
  g_printerr ("Failed writing sound: %s\n", snd_strerror (err));
}

static gboolean
check_timeout (gpointer data)
{
  do_the_write_thing (data);

  return TRUE;
}

static gboolean
handle_sound (GIOChannel *source, GIOCondition cond, gpointer data)
{
  Sound *sound = data;

  //g_printerr ("condition %u\n", (guint) cond);
  do_the_write_thing (sound);
  return TRUE;
}

gpointer
swfdec_playback_open (GSourceFunc func, gpointer data, guint usecs_per_frame, guint *buffer_size)
{
  snd_pcm_t *ret;
  snd_pcm_hw_params_t *hw_params;
  unsigned int rate, count;
  Sound *sound;
  snd_pcm_uframes_t uframes;

  /* "default" uses dmix, and dmix ticks way slow, so this thingy here stutters */
  ALSA_ERROR (snd_pcm_open (&ret, /*"default"*/"plughw:0", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK),
      "Failed to open sound device", NULL);

  snd_pcm_hw_params_alloca (&hw_params);
  if (snd_pcm_hw_params_any (ret, hw_params) < 0) {
    g_printerr ("No sound format available\n");
    return NULL;
  }
  if (snd_pcm_hw_params_set_access (ret, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED) < 0) {
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
  if (buffer_size) {
    snd_pcm_uframes_t ret;
    if (snd_pcm_hw_params_get_buffer_size (hw_params, &ret) < 0) {
      g_printerr ("Could not query buffer size\n");
      *buffer_size = 0;
    } else {
      *buffer_size = ret;
    }
  }

  if (snd_pcm_prepare (ret) < 0) {
    g_printerr ("no prepare\n");
  }

  sound = g_new0 (Sound, 1);
  sound->pcm = ret;
  sound->func = func;
  sound->func_data = data;
  count = snd_pcm_poll_descriptors_count (ret);
  if (count > 0) {
    struct pollfd polls[count];
    unsigned int i;
    if (count > 1)
      g_printerr ("more than one fd!\n");
    count = snd_pcm_poll_descriptors (ret, polls, count);
    sound->sources = g_new (guint, count + 1);
    for (i = 0; i < count; i++) {
      GIOChannel *channel = g_io_channel_unix_new (polls[i].fd);
      sound->sources[i] = g_io_add_watch (channel, polls[i].events, handle_sound, sound);
      g_io_channel_unref (channel);
    }
    sound->sources[i] = 0;
  }
  sound->timeout = g_timeout_add (usecs_per_frame / 2000, check_timeout, sound);
  return sound;

fail:
  snd_pcm_close (ret);
  if (buffer_size)
    *buffer_size = 0;
  return NULL;
}

void
swfdec_playback_write (gpointer data, SwfdecBuffer *buffer)
{
  Sound *sound = data;

  swfdec_buffer_ref (buffer);
  sound->buffers = g_list_append (sound->buffers, buffer);
  //g_print ("write of %u bytes\n", buffer->length);

  do_the_write_thing (sound);
}

void
swfdec_playback_close (gpointer data)
{
  guint i;
  Sound *sound = data;

  ALSA_TRY (snd_pcm_close (sound->pcm), "failed closing");
  g_source_remove (sound->timeout);
  if (sound->sources) {
    for (i = 0; sound->sources[i]; i++)
      g_source_remove (sound->sources[i]);
    g_free (sound->sources);
  }
  g_free (sound);
}
