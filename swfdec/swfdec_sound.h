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

#ifndef _SWFDEC_SOUND_H_
#define _SWFDEC_SOUND_H_

#include <swfdec/swfdec_character.h>
#include <swfdec/swfdec_swf_decoder.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS

//typedef struct _SwfdecSoundChunk SwfdecSoundChunk;
//typedef struct _SwfdecSound SwfdecSound;
typedef struct _SwfdecSoundClass SwfdecSoundClass;
typedef struct _SwfdecSoundEnvelope SwfdecSoundEnvelope;

#define SWFDEC_TYPE_SOUND                    (swfdec_sound_get_type())
#define SWFDEC_IS_SOUND(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SOUND))
#define SWFDEC_IS_SOUND_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SOUND))
#define SWFDEC_SOUND(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SOUND, SwfdecSound))
#define SWFDEC_SOUND_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SOUND, SwfdecSoundClass))

struct _SwfdecSoundEnvelope {
  guint		offset;			/* offset in frames */
  guint16		volume[2];		/* volume to use */
};

struct _SwfdecSoundChunk
{
  SwfdecSound *		sound;			/* sound to play */

  int			stop;	      		/* stop the sample being played */
  int			no_restart;	      	/* don't restart if already playing */

  guint			start_sample; 		/* sample at which to start playing */
  guint			stop_sample;	      	/* first sample to not play anymore or 0 for playing all */
  guint			loop_count;		/* amount of times this sample should be played back */
  guint			n_envelopes;		/* amount of points in the envelope */
  SwfdecSoundEnvelope *	envelope;		/* volume envelope or NULL if none */
};

struct _SwfdecSound
{
  SwfdecCharacter	character;

  guint			codec;			/* codec in use */
  SwfdecAudioFormat	format;	        	/* channel/rate/width information for codec */
  guint			n_samples;		/* total number of samples when decoded to 44100kHz */
  guint			skip;			/* samples to skip at start */
  SwfdecBuffer *	encoded;		/* encoded data */

  SwfdecBuffer *	decoded;		/* decoded data */
};

struct _SwfdecSoundClass
{
  SwfdecCharacterClass	character_class;
};

GType swfdec_sound_get_type (void);

int tag_func_define_sound (SwfdecSwfDecoder * s, guint tag);
int tag_func_start_sound (SwfdecSwfDecoder * s, guint tag);
int tag_func_define_button_sound (SwfdecSwfDecoder * s, guint tag);

SwfdecBuffer *		swfdec_sound_get_decoded	(SwfdecSound *		sound);
void			swfdec_sound_buffer_render	(gint16 *		dest, 
							 const SwfdecBuffer *	source, 
							 guint	  		offset,
							 guint			n_samples);
guint			swfdec_sound_buffer_get_n_samples (const SwfdecBuffer * buffer, 
                                                         SwfdecAudioFormat	format);

SwfdecSoundChunk *	swfdec_sound_parse_chunk	(SwfdecSwfDecoder *	s,
							 SwfdecBits *		bits,
							 int			id);
void			swfdec_sound_chunk_free		(SwfdecSoundChunk *	chunk);


G_END_DECLS
#endif
