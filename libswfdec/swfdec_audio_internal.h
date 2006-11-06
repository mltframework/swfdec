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

#ifndef _SWFDEC_AUDIO_INTERNAL_H_
#define _SWFDEC_AUDIO_INTERNAL_H_

#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_audio.h>
#include <libswfdec/swfdec_types.h>

G_BEGIN_DECLS


struct _SwfdecAudio {
  GObject		object;

  SwfdecPlayer *	player;		/* the player that plays us */
  guint			start_offset;	/* offset from player in number of samples */
};

struct _SwfdecAudioClass {
  GObjectClass		object_class;

  guint			(* iterate)		(SwfdecAudio *	audio,
						 guint		n_samples);
  void			(* render)		(SwfdecAudio *	audio,
						 gint16 *	dest,
						 guint		start, 
						 guint		n_samples);
};

SwfdecAudio *	swfdec_audio_new		(SwfdecPlayer *	player,
						 GType		type);
void		swfdec_audio_remove		(SwfdecAudio *	audio);

guint		swfdec_audio_iterate		(SwfdecAudio *	audio,
						 guint		n_samples);

void		swfdec_player_iterate_audio   	(SwfdecPlayer *	player);

G_END_DECLS
#endif
