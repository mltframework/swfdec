/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_actor.h"
#include "swfdec_debug.h"


G_DEFINE_ABSTRACT_TYPE (SwfdecActor, swfdec_actor, SWFDEC_TYPE_MOVIE)

static gboolean
swfdec_actor_iterate_end (SwfdecActor *actor)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (actor);

  return movie->parent == NULL || 
	 movie->state < SWFDEC_MOVIE_STATE_REMOVED;
}

static void
swfdec_actor_class_init (SwfdecActorClass *klass)
{
  klass->iterate_end = swfdec_actor_iterate_end;
}

static void
swfdec_actor_init (SwfdecActor *actor)
{
}

