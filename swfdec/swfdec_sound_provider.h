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

#ifndef __SWFDEC_SOUND_PROVIDER_H__
#define __SWFDEC_SOUND_PROVIDER_H__

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_sound_matrix.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS


#define SWFDEC_TYPE_SOUND_PROVIDER                (swfdec_sound_provider_get_type ())
#define SWFDEC_SOUND_PROVIDER(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SOUND_PROVIDER, SwfdecSoundProvider))
#define SWFDEC_IS_SOUND_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SOUND_PROVIDER))
#define SWFDEC_SOUND_PROVIDER_GET_INTERFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), SWFDEC_TYPE_SOUND_PROVIDER, SwfdecSoundProviderInterface))

typedef struct _SwfdecSoundProvider SwfdecSoundProvider; /* dummy object */
typedef struct _SwfdecSoundProviderInterface SwfdecSoundProviderInterface;

struct _SwfdecSoundProviderInterface {
  GTypeInterface	interface;

  void			(* start)				(SwfdecSoundProvider *  provider,
								 SwfdecActor *		actor, 
								 gsize			samples_offset,
								 guint			loops);
  void			(* stop)				(SwfdecSoundProvider *  provider,
								 SwfdecActor *		actor);
  SwfdecSoundMatrix *	(* get_matrix)				(SwfdecSoundProvider *  provider);
};

GType			swfdec_sound_provider_get_type		(void) G_GNUC_CONST;

void			swfdec_sound_provider_start		(SwfdecSoundProvider *	provider,
								 SwfdecActor *		actor, 
								 gsize			samples_offset,
								 guint			loops);
void			swfdec_sound_provider_stop		(SwfdecSoundProvider *	provider,
								 SwfdecActor *		actor); 
SwfdecSoundMatrix *	swfdec_sound_provider_get_matrix	(SwfdecSoundProvider *  provider);


G_END_DECLS

#endif /* __SWFDEC_SOUND_PROVIDER_H__ */
