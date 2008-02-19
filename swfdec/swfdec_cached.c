/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_cached.h"
#include "swfdec_debug.h"


G_DEFINE_ABSTRACT_TYPE (SwfdecCached, swfdec_cached, SWFDEC_TYPE_CHARACTER)

static void
swfdec_cached_dispose (GObject *object)
{
  SwfdecCached * cached = SWFDEC_CACHED (object);

  swfdec_cached_unload (cached);
  swfdec_cached_set_cache (cached, NULL);

  G_OBJECT_CLASS (swfdec_cached_parent_class)->dispose (object);
}

static void
swfdec_cached_class_init (SwfdecCachedClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_cached_dispose;
}

static void
swfdec_cached_init (SwfdecCached * cached)
{
}

void
swfdec_cached_set_cache (SwfdecCached *cached, SwfdecCache *cache)
{
  g_return_if_fail (SWFDEC_IS_CACHED (cached));

  if (cached->cache) {
    if (cached->handle.unload)
      swfdec_cache_remove_handle (cached->cache, &cached->handle);
    swfdec_cache_unref (cached->cache);
  }
  cached->cache = cache;
  if (cache) {
    swfdec_cache_ref (cache);
    if (cached->handle.unload)
      swfdec_cache_add_handle (cached->cache, &cached->handle);
  }
}

static void
swfdec_cached_unload_func (gpointer data)
{
  SwfdecCached *cached = SWFDEC_CACHED ((void *) ((guint8 *) data - G_STRUCT_OFFSET (SwfdecCached, handle)));

  cached->handle.unload = NULL;
  swfdec_cached_unload (cached);
}

void
swfdec_cached_load (SwfdecCached *cached, guint size)
{
  g_return_if_fail (SWFDEC_IS_CACHED (cached));
  g_return_if_fail (cached->handle.unload == NULL);
  g_return_if_fail (size > 0);

  cached->handle.unload = swfdec_cached_unload_func;
  cached->handle.size = size;
  if (cached->cache)
    swfdec_cache_add_handle (cached->cache, &cached->handle);
}

void
swfdec_cached_use (SwfdecCached *cached)
{
  g_return_if_fail (SWFDEC_IS_CACHED (cached));
  g_return_if_fail (cached->handle.unload != NULL);

  if (cached->cache)
    swfdec_cache_add_handle (cached->cache, &cached->handle);
}

void
swfdec_cached_unload (SwfdecCached *cached)
{
  g_return_if_fail (SWFDEC_IS_CACHED (cached));

  if (cached->handle.unload) {
    if (cached->cache)
      swfdec_cache_remove_handle (cached->cache, &cached->handle);
    cached->handle.unload = NULL;
  }
  if (cached->handle.size) {
    SwfdecCachedClass *klass;

    klass = SWFDEC_CACHED_GET_CLASS (cached);
    cached->handle.size = 0;
    g_return_if_fail (klass->unload != NULL);
    klass->unload (cached);
  }
}

