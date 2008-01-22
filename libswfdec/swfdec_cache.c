/* Swfdec
 * Copyright (C) 2005 David Schleef <ds@schleef.org>
 *		 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_cache.h"
#include "swfdec_debug.h"

SwfdecCache *
swfdec_cache_new (gulong max_size)
{
  SwfdecCache *cache;
  
  g_return_val_if_fail (max_size > 0, NULL);

  cache = g_new0 (SwfdecCache, 1);
  cache->refcount = 1;
  cache->queue = g_queue_new ();
  cache->max_size = max_size;

  return cache;
}

void
swfdec_cache_ref (SwfdecCache *cache)
{
  g_return_if_fail (cache != NULL);

  cache->refcount++;
}

void
swfdec_cache_unref (SwfdecCache *cache)
{
  g_return_if_fail (cache != NULL);
  g_return_if_fail (cache->refcount > 0);

  cache->refcount--;
  if (cache->refcount > 0)
    return;

  g_queue_free (cache->queue);
  g_free (cache);
}

gulong
swfdec_cache_get_usage (SwfdecCache *cache)
{
  g_return_val_if_fail (cache != NULL, 0);

  return cache->usage;
}

void
swfdec_cache_shrink (SwfdecCache *cache, gulong max_usage)
{
  g_return_if_fail (cache != NULL);

  while (cache->usage > max_usage) {
    SwfdecCacheHandle *handle = g_queue_pop_tail (cache->queue);
    g_assert (handle);
    cache->usage -= handle->size;
    SWFDEC_LOG ("%p removing %p (%lu => %lu)", cache, handle, 
	cache->usage + handle->size, cache->usage);
    handle->unload (handle);
  }
}

gulong
swfdec_cache_get_size (SwfdecCache *cache)
{
  g_return_val_if_fail (cache != NULL, 0);

  return cache->max_size;
}

void
swfdec_cache_set_size (SwfdecCache *cache, gulong max_usage)
{
  g_return_if_fail (cache != NULL);

  swfdec_cache_shrink (cache, max_usage);
  cache->max_size = max_usage;
}

/**
 * swfdec_cache_add_handle:
 * @cache: a #SwfdecCache
 * @handle: a handle to add
 *
 * Adds @handle to @cache. If not enough space is available in the cache,
 * the cache will unload existing handles first. If handle is already part
 * of cache, its usage information will be updated. This will make it less
 * likely that it gets unloaded.
 **/
void
swfdec_cache_add_handle (SwfdecCache *cache, const SwfdecCacheHandle *handle)
{
  GList *list;

  g_return_if_fail (cache != NULL);
  g_return_if_fail (handle != NULL);
  g_return_if_fail (handle->size > 0);
  g_return_if_fail (handle->unload != NULL);

  list = g_queue_find (cache->queue, handle);
  if (list) {
    /* bring to front of queue */
    g_queue_unlink (cache->queue, list);
    g_queue_push_head_link (cache->queue, list);
  } else {
    swfdec_cache_shrink (cache, cache->max_size - handle->size);
    g_queue_push_head (cache->queue, (gpointer) handle);
    cache->usage += handle->size;
    SWFDEC_LOG ("%p adding %p (%lu => %lu)", cache, handle, 
	cache->usage - handle->size, cache->usage);
  }
}

/**
 * swfdec_cache_remove_handle:
 * @cache: a #SwfdecCache
 * @handle: the handle to remove
 *
 * Removes the given @handle from the @cache, if it was part of it. If the 
 * handle wasn't part of the cache, nothing happens.
 **/
void
swfdec_cache_remove_handle (SwfdecCache *cache, const SwfdecCacheHandle *handle)
{
  GList *list;

  g_return_if_fail (cache != NULL);
  g_return_if_fail (handle != NULL);
  g_return_if_fail (handle->size > 0);
  g_return_if_fail (handle->unload != NULL);

  list = g_queue_find (cache->queue, handle);
  if (list) {
    g_queue_delete_link (cache->queue, list);
    cache->usage -= handle->size;
    SWFDEC_LOG ("%p removing %p (%lu => %lu)", cache, handle, 
	cache->usage + handle->size, cache->usage);
  }
}
