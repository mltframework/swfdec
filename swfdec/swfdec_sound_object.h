/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_SOUND_OBJECT_H_
#define _SWFDEC_SOUND_OBJECT_H_

#include <swfdec/swfdec_as_relay.h>
#include <swfdec/swfdec_audio.h>
#include <swfdec/swfdec_load_sound.h>
#include <swfdec/swfdec_movie.h>
#include <swfdec/swfdec_sound_provider.h>

G_BEGIN_DECLS


typedef struct _SwfdecSoundObject SwfdecSoundObject;
typedef struct _SwfdecSoundObjectClass SwfdecSoundObjectClass;

#define SWFDEC_TYPE_SOUND_OBJECT                    (swfdec_sound_object_get_type())
#define SWFDEC_IS_SOUND_OBJECT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SOUND_OBJECT))
#define SWFDEC_IS_SOUND_OBJECT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SOUND_OBJECT))
#define SWFDEC_SOUND_OBJECT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SOUND_OBJECT, SwfdecSoundObject))
#define SWFDEC_SOUND_OBJECT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SOUND_OBJECT, SwfdecSoundObjectClass))
#define SWFDEC_SOUND_OBJECT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_SOUND_OBJECT, SwfdecSoundObjectClass))

struct _SwfdecSoundObject {
  SwfdecAsRelay		relay;

  const char *		target;		/* target or NULL if global */
  SwfdecSoundProvider *	provider;	/* sound that we play */
};

struct _SwfdecSoundObjectClass {
  SwfdecAsRelayClass	relay_class;
};

GType			swfdec_sound_object_get_type	(void);


G_END_DECLS
#endif
