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

#ifndef _SWFDEC_AUDIO_INTERNAL_H_
#define _SWFDEC_AUDIO_INTERNAL_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_audio.h>
#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS

typedef guint SwfdecAudioFormat;
#define SWFDEC_IS_AUDIO_FORMAT(format) ((format) <= 0xF)
#define SWFDEC_AUDIO_FORMAT_INVALID ((SwfdecAudioFormat) -1)


struct _SwfdecAudio {
  GObject		object;

  SwfdecPlayer *	player;		/* the player that plays us */
  gboolean		added;		/* set to TRUE after the added signal has been emitted */
};

struct _SwfdecAudioClass {
  GObjectClass		object_class;

  guint			(* iterate)	  		(SwfdecAudio *		audio,
							 guint			n_samples);
  guint			(* render)			(SwfdecAudio *		audio,
							 gint16 *		dest,
							 guint			start, 
							 guint			n_samples);
};

void			swfdec_audio_add		(SwfdecAudio *		audio,
							 SwfdecPlayer *		player);
void			swfdec_audio_remove		(SwfdecAudio *		audio);

guint			swfdec_audio_iterate		(SwfdecAudio *		audio,
							 guint			n_samples);

SwfdecAudioFormat	swfdec_audio_format_parse	(SwfdecBits *	  	bits);
SwfdecAudioFormat	swfdec_audio_format_new		(guint			rate,
							 guint			channels,
							 gboolean		is_16bit);
guint			swfdec_audio_format_get_channels(SwfdecAudioFormat	format);
gboolean		swfdec_audio_format_is_16bit	(SwfdecAudioFormat	format);
guint			swfdec_audio_format_get_rate	(SwfdecAudioFormat	format);
guint			swfdec_audio_format_get_granularity
							(SwfdecAudioFormat	format);
guint			swfdec_audio_format_get_bytes_per_sample
							(SwfdecAudioFormat	format);
const char *		swfdec_audio_format_to_string	(SwfdecAudioFormat	format);


G_END_DECLS
#endif
