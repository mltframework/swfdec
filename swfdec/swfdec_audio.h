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

#ifndef _SWFDEC_AUDIO_H_
#define _SWFDEC_AUDIO_H_

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _SwfdecAudio SwfdecAudio;
typedef struct _SwfdecAudioClass SwfdecAudioClass;

#define SWFDEC_TYPE_AUDIO                    (swfdec_audio_get_type())
#define SWFDEC_IS_AUDIO(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AUDIO))
#define SWFDEC_IS_AUDIO_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AUDIO))
#define SWFDEC_AUDIO(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AUDIO, SwfdecAudio))
#define SWFDEC_AUDIO_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AUDIO, SwfdecAudioClass))
#define SWFDEC_AUDIO_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AUDIO, SwfdecAudioClass))

GType		swfdec_audio_get_type		(void);

void		swfdec_audio_render		(SwfdecAudio *	audio,
						 gint16 *	dest,
						 guint		start_offset,
						 guint		n_samples);

G_END_DECLS
#endif
