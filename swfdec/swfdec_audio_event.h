/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_AUDIO_EVENT_H_
#define _SWFDEC_AUDIO_EVENT_H_

#include <swfdec/swfdec_audio_internal.h>
#include <swfdec/swfdec_sound.h>

G_BEGIN_DECLS

typedef struct _SwfdecAudioEvent SwfdecAudioEvent;
typedef struct _SwfdecAudioEventClass SwfdecAudioEventClass;

#define SWFDEC_TYPE_AUDIO_EVENT                    (swfdec_audio_event_get_type())
#define SWFDEC_IS_AUDIO_EVENT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AUDIO_EVENT))
#define SWFDEC_IS_AUDIO_EVENT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AUDIO_EVENT))
#define SWFDEC_AUDIO_EVENT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AUDIO_EVENT, SwfdecAudioEvent))
#define SWFDEC_AUDIO_EVENT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AUDIO_EVENT, SwfdecAudioEventClass))
#define SWFDEC_AUDIO_EVENT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AUDIO_EVENT, SwfdecAudioEventClass))

struct _SwfdecAudioEvent
{
  SwfdecAudio		audio;

  /* static data */
  SwfdecSound *		sound;		      	/* sound we're playing */
  guint			start_sample; 		/* sample at which to start playing */
  guint			stop_sample;	      	/* first sample to not play anymore or 0 for playing all */
  guint			n_loops;		/* amount of times this sample still needs to be played back */
  guint			n_envelopes;		/* amount of points in the envelope */
  SwfdecSoundEnvelope *	envelope;		/* volume envelope or NULL if none */
  /* dynamic data */
  SwfdecBuffer *	decoded;		/* the decoded buffer we play back or NULL if failure */
  guint			offset;			/* current offset in 44.1kHz */
  guint			loop;			/* current loop we're in */
  guint			n_samples;	      	/* length of decoded buffer in 44.1kHz samples - can be 0 */
};

struct _SwfdecAudioEventClass
{
  SwfdecAudioClass	audio_class;
};

GType		swfdec_audio_event_get_type		(void);

SwfdecAudio *	swfdec_audio_event_new			(SwfdecPlayer *		player,
							 SwfdecSound *		sound,
							 guint			offset,
							 guint			n_loops);
SwfdecAudio *	swfdec_audio_event_new_from_chunk     	(SwfdecPlayer *		player,
							 SwfdecSoundChunk *	chunk);

G_END_DECLS
#endif
