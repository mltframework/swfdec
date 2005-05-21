
#ifndef __SWFDEC_AUDIO_H__
#define __SWFDEC_AUDIO_H__

#include "config.h"

#include "swfdec_types.h"

G_BEGIN_DECLS

typedef struct _SwfdecAudioStream SwfdecAudioStream;

struct _SwfdecAudioStream {
  int id;

  SwfdecBufferQueue *queue;
  double volume;
  gboolean streaming;

  int n_loops;
  SwfdecSound *sound;

  gboolean reap_me;
};


int swfdec_audio_add_sound (SwfdecDecoder *decoder, SwfdecSound *sound,
    int n_loops);
void swfdec_audio_remove_stream (SwfdecDecoder *decoder, int id);
int swfdec_audio_add_stream (SwfdecDecoder *decoder);
void swfdec_audio_remove_all_streams (SwfdecDecoder *decoder);
void swfdec_audio_stream_push_buffer (SwfdecDecoder *decoder, int id,
    SwfdecBuffer *buffer);

void swfdec_audio_set_volume (SwfdecDecoder *decoder, int id, double volume);

SwfdecBuffer * swfdec_audio_render (SwfdecDecoder *decoder, int n_samples);


G_END_DECLS

#endif
