
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_internal.h"
#include <string.h>

static void merge (int16_t *dest, int16_t *src, double multiplier, int n);

void
swfdec_audio_stream_free (SwfdecAudioStream *stream)
{
  swfdec_buffer_queue_free (stream->queue);
  g_free(stream);
}

int
swfdec_audio_add_sound (SwfdecDecoder *decoder, SwfdecSound *sound,
    int n_loops)
{
  SwfdecAudioStream *stream;

  stream = g_new0(SwfdecAudioStream, 1);
  stream->id = decoder->audio_stream_index++;
  stream->queue = swfdec_buffer_queue_new ();
  stream->sound = sound;
  stream->n_loops = n_loops;
  stream->volume = 1.0;

  decoder->audio_streams = g_list_append (decoder->audio_streams, stream);

  return stream->id;
}

void
swfdec_audio_remove_stream (SwfdecDecoder *decoder, int id)
{
  GList *g;
  GList *gnext;

  for(g=g_list_first(decoder->audio_streams);g;g=gnext){
    SwfdecAudioStream *stream = g->data;

    gnext = g_list_next(g);

    if (stream->id == id) {
      decoder->audio_streams = g_list_delete_link (decoder->audio_streams, g);
      swfdec_audio_stream_free(stream);
    }
  }
}

int
swfdec_audio_add_stream (SwfdecDecoder *decoder)
{
  SwfdecAudioStream *stream;

  stream = g_new0(SwfdecAudioStream, 1);
  stream->id = decoder->audio_stream_index++;
  stream->queue = swfdec_buffer_queue_new ();
  stream->volume = 1.0;

  decoder->audio_streams = g_list_append (decoder->audio_streams, stream);

  return stream->id;
}

void
swfdec_audio_remove_all_streams (SwfdecDecoder *decoder)
{
  GList *g;

  while((g=g_list_first(decoder->audio_streams))) {
    SwfdecAudioStream *stream = g->data;

    decoder->audio_streams = g_list_delete_link (decoder->audio_streams, g);
    swfdec_audio_stream_free(stream);
  }
}

void
swfdec_audio_stream_push_buffer (SwfdecDecoder *decoder, int id,
    SwfdecBuffer *buffer)
{
  GList *g;

  for(g=g_list_first(decoder->audio_streams);g;g=g_list_next(g)){
    SwfdecAudioStream *stream = g->data;

    if (stream->id == id) {
      swfdec_buffer_queue_push (stream->queue, buffer);
      return;
    }
  }
  g_warning("not reached");
  swfdec_buffer_unref(buffer);
}


void
swfdec_audio_set_volume (SwfdecDecoder *decoder, int id, double volume)
{
  g_warning("unimplemented");
}

SwfdecBuffer *
swfdec_audio_render (SwfdecDecoder *decoder, int n_samples)
{
  GList *g;
  SwfdecBuffer *out_buffer;
  SwfdecBuffer *buffer;
  GList *gnext;
  int n;

  out_buffer = swfdec_buffer_new_and_alloc (n_samples * 4);
  memset (out_buffer->data, 0, n_samples * 4);
  for(g = g_list_first(decoder->audio_streams); g; g = gnext) {
    SwfdecAudioStream *stream = g->data;

    gnext = g_list_next(g);
    if (stream->sound) {
      while (swfdec_buffer_queue_get_depth (stream->queue) < n_samples * 4 && 
          stream->n_loops > 0) {
        GList *g2;
        for(g2=g_list_first(stream->sound->decoded_buffers);g2;
            g2=g_list_next(g2)) {
          SwfdecBuffer *buffer = (SwfdecBuffer *)g2->data;
          swfdec_buffer_queue_push (stream->queue, swfdec_buffer_ref(buffer));
        }
        stream->n_loops--;
      }
    }
    n = swfdec_buffer_queue_get_depth (stream->queue) / 4;
    if (n > n_samples) n = n_samples;
    if (n > 0) {
      buffer = swfdec_buffer_queue_pull (stream->queue, n * 4);
      merge((int16_t *)out_buffer->data, (int16_t *)buffer->data,
          stream->volume, n*2);
      swfdec_buffer_unref(buffer);
    }
    if (stream->sound && stream->n_loops == 0 &&
        swfdec_buffer_queue_get_depth (stream->queue) == 0) {
      decoder->audio_streams = g_list_delete_link (decoder->audio_streams, g);
      swfdec_audio_stream_free(stream);
    }
  }

  return out_buffer;
}

static void
merge (int16_t *dest, int16_t *src, double multiplier, int n)
{
  int i;
  int x;
  for(i=0;i<n;i++){
    x = dest[i] + src[i] * multiplier;
    if (x<-32768) x = -32768;
    if (x>32767) x = 32767;
    dest[i] = x;
  }
}

