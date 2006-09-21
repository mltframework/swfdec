
#ifndef __SWFDEC_AUDIO_H__
#define __SWFDEC_AUDIO_H__

#include "config.h"

#include "swfdec_types.h"

G_BEGIN_DECLS

typedef struct _SwfdecAudioEvent SwfdecAudioEvent;

struct _SwfdecAudioEvent {
  SwfdecSound *		sound;	      	/* sound we're playing */
  /* for events */
  SwfdecSoundChunk *	chunk;		/* chunk we're playing back */
  unsigned int		offset;		/* current offset */
  unsigned int		loop;		/* current loop we're in */
};


void		swfdec_audio_iterate_start    	(SwfdecDecoder *	dec);
void		swfdec_audio_iterate_finish	(SwfdecDecoder *	dec);

void		swfdec_audio_event_init		(SwfdecDecoder *	dec, 
						 SwfdecSoundChunk *	chunk);


G_END_DECLS

#endif
