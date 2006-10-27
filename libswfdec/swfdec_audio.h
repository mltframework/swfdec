/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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

#ifndef __SWFDEC_AUDIO_H__
#define __SWFDEC_AUDIO_H__

#include <libswfdec/swfdec_player.h>
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
  SwfdecSprite *	sprite;		/* sprite we're playing back */
  SwfdecSound *		sound;	      	/* sound we're playing */
  gpointer		decoder;	/* decoder used for this frame */
  unsigned int	      	skip;		/* samples to skip at start of stream */
  gboolean		disabled;	/* set to TRUE when we can't queue more data */
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
void			swfdec_player_iterate_audio   	(SwfdecPlayer *		player);

/* FIXME: new streams and events only start playing when iterating due to the current
 * design of the soudn engine */
void			swfdec_audio_event_init		(SwfdecPlayer *		player, 
							 SwfdecSoundChunk *	chunk);

guint			swfdec_audio_stream_new		(SwfdecPlayer *		player,
							 SwfdecSprite *		sprite,
							 guint			start_frame);
void			swfdec_audio_stream_stop      	(SwfdecPlayer *		player,
							 guint			stream_id);


G_END_DECLS

#endif
