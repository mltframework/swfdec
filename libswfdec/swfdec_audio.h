
#ifndef __SWFDEC_AUDIO_H__
#define __SWFDEC_AUDIO_H__

#include "config.h"

#include <libswfdec/swfdec_types.h>
#include <libswfdec/swfdec_ringbuffer.h>

G_BEGIN_DECLS

typedef union _SwfdecAudio SwfdecAudio;
typedef struct _SwfdecAudioEvent SwfdecAudioEvent;
typedef struct _SwfdecAudioStream SwfdecAudioStream;

typedef enum {
  SWFDEC_AUDIO_UNUSED,
  SWFDEC_AUDIO_EVENT,
  SWFDEC_AUDIO_STREAM
} SwfdecAudioType;

struct _SwfdecAudioEvent {
  SwfdecAudioType	type;
  SwfdecSound *		sound;	      	/* sound we're playing */
  unsigned int		skip;		/* samples to skip */
  SwfdecSoundChunk *	chunk;		/* chunk we're playing back */
  unsigned int		offset;		/* current offset */
  unsigned int		loop;		/* current loop we're in */
};

struct _SwfdecAudioStream {
  SwfdecAudioType	type;
  SwfdecSound *		sound;	      	/* sound we're playing */
  gpointer		decoder;	/* decoder used for this frame */
  unsigned int		skip;		/* samples to skip */
  gboolean		disabled;	/* set to TRUE when we can't queue more data */
  SwfdecSprite *	sprite;		/* sprite we're playing back */
  unsigned int		playback_samples; /* number of samples in queue */
  SwfdecRingBuffer *	playback_queue;	/* all the samples we've decoded so far */
  unsigned int		current_frame;	/* last decoded frame */
};

union _SwfdecAudio {
  SwfdecAudioType	type;
  SwfdecAudioEvent	event;
  SwfdecAudioStream	stream;
};

void			swfdec_audio_finish		(SwfdecAudio *		audio);
void			swfdec_audio_iterate		(SwfdecDecoder *	dec);

void			swfdec_audio_event_init		(SwfdecDecoder *	dec, 
							 SwfdecSoundChunk *	chunk);

guint			swfdec_audio_stream_new		(SwfdecSprite *		sprite,
							 guint			start_frame);
void			swfdec_audio_stream_stop      	(SwfdecDecoder *	dec,
							 guint			stream_id);


G_END_DECLS

#endif
