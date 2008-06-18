/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_LOAD_SOUND_H_
#define _SWFDEC_LOAD_SOUND_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_audio.h>
#include <swfdec/swfdec_sound_matrix.h>
#include <swfdec/swfdec_stream.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS


typedef struct _SwfdecLoadSound SwfdecLoadSound;
typedef struct _SwfdecLoadSoundClass SwfdecLoadSoundClass;

#define SWFDEC_TYPE_LOAD_SOUND                    (swfdec_load_sound_get_type())
#define SWFDEC_IS_LOAD_SOUND(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_LOAD_SOUND))
#define SWFDEC_IS_LOAD_SOUND_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_LOAD_SOUND))
#define SWFDEC_LOAD_SOUND(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_LOAD_SOUND, SwfdecLoadSound))
#define SWFDEC_LOAD_SOUND_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_LOAD_SOUND, SwfdecLoadSoundClass))
#define SWFDEC_LOAD_SOUND_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_LOAD_SOUND, SwfdecLoadSoundClass))

struct _SwfdecLoadSound
{
  GObject		object;

  SwfdecAsObject *	target;		/* target using us that we emit events on, a SwfdecSoundObject */
  SwfdecSandbox *	sandbox;	/* sandbox we use for emission of events */
  char *		url;		/* URL we are loading - FIXME: make the security stuff hand us a loader */
  SwfdecStream *	stream;		/* stream we're parsing or NULL when done parsing */
  GPtrArray *		frames;		/* buffers pointing to the frames of this file */
  SwfdecAudio *		audio;		/* the stream currently running */
  SwfdecSoundMatrix	sound_matrix;	/* matrix we reference */
};

struct _SwfdecLoadSoundClass
{
  GObjectClass    	object_class;
};

GType			swfdec_load_sound_get_type	(void);

SwfdecLoadSound *	swfdec_load_sound_new		(SwfdecAsObject *	target,
							 const char *		url);


G_END_DECLS
#endif
