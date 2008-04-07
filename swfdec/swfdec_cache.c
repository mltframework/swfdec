/* Swfdec
 * Copyright (C) 2005 David Schleef <ds@schleef.org>
 *		 2007-2008 Benjamin Otte <otte@gnome.org>
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


G_DEFINE_TYPE (SwfdecCache, swfdec_cache, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_CACHE_SIZE,
  PROP_MAX_CACHE_SIZE,
};

/* NB: assumes that the cached was already removed from cache->list */
static void
swfdec_cache_remove (SwfdecCache *cache, SwfdecCached *cached)
{
  cache->size -= swfdec_cached_get_size (cached);
  g_signal_handlers_disconnect_matched (cached, 
      G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, cache);
  g_object_unref (cached);
}

static void
swfdec_cache_dispose (GObject *object)
{
  SwfdecCache *cache = SWFDEC_CACHE (object);
  SwfdecCached *cached;

  if (cache->queue) {
    while ((cached = g_queue_pop_tail (cache->queue)))
      swfdec_cache_remove (cache, cached);
    g_queue_free (cache->queue);
  }
  g_assert (cache->size == 0);

  G_OBJECT_CLASS (swfdec_cache_parent_class)->dispose (object);
}

static void
swfdec_cache_get_property (GObject *object, guint param_id, GValue *value,
    GParamSpec *pspec)
{
  SwfdecCache *cache = SWFDEC_CACHE (object);

  switch (param_id) {
    case PROP_CACHE_SIZE:
      g_value_set_ulong (value, cache->size);
      break;
    case PROP_MAX_CACHE_SIZE:
      g_value_set_ulong (value, cache->max_size);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_cache_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecCache *cache = SWFDEC_CACHE (object);

  switch (param_id) {
    case PROP_MAX_CACHE_SIZE:
      swfdec_cache_set_max_cache_size (cache, g_value_get_ulong (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_cache_class_init (SwfdecCacheClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_cache_dispose;
  object_class->get_property = swfdec_cache_get_property;
  object_class->set_property = swfdec_cache_set_property;

  /* FIXME: should be g_param_spec_size(), but no such thing exists */
  g_object_class_install_property (object_class, PROP_CACHE_SIZE,
      g_param_spec_ulong ("cache-size", "cache-size", "current size of cache",
	  0, G_MAXULONG, 0, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_MAX_CACHE_SIZE,
      g_param_spec_ulong ("max-cache-size", "max-cache-size", "maximum allowed size of cache",
	  0, G_MAXULONG, 1024 * 1024, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
}

static void
swfdec_cache_init (SwfdecCache *cache)
{
  cache->queue = g_queue_new ();
}

SwfdecCache *
swfdec_cache_new (gsize max_size)
{
  return g_object_new (SWFDEC_TYPE_CACHE, "max-cache-size", max_size);
}

gsize
swfdec_cache_get_cache_size (SwfdecCache *cache)
{
  g_return_val_if_fail (SWFDEC_IS_CACHE (cache), 0);

  return cache->size;
}

gsize
swfdec_cache_get_max_cache_size (SwfdecCache *cache)
{
  g_return_val_if_fail (SWFDEC_IS_CACHE (cache), 0);

  return cache->max_size;
}

void
swfdec_cache_set_max_cache_size (SwfdecCache *cache, gsize max_size)
{
  g_return_if_fail (SWFDEC_IS_CACHE (cache));

  cache->max_size = max_size;
  swfdec_cache_shrink (cache, max_size);
  g_object_notify (G_OBJECT (cache), "max-cache-size");
}

void
swfdec_cache_shrink (SwfdecCache *cache, gsize size)
{
  SwfdecCached *cached;

  g_return_if_fail (SWFDEC_IS_CACHE (cache));

  if (size >= cache->size)
    return;

  do {
    cached = g_queue_pop_tail (cache->queue);
    g_assert (cached);
    swfdec_cache_remove (cache, cached);
  } while (size < cache->size);
  g_object_notify (G_OBJECT (cache), "cache-size");
}

static void
swfdec_cache_use_cached (SwfdecCached *cached, SwfdecCache *cache)
{
  /* move cached item to the front of the queue */
  g_queue_remove (cache->queue, cached);
  g_queue_push_head (cache->queue, cached);
}

static void
swfdec_cache_unuse_cached (SwfdecCached *cached, SwfdecCache *cache)
{
  /* move cached item to the front of the queue */
  g_queue_remove (cache->queue, cached);
  swfdec_cache_remove (cache, cached);
}

void
swfdec_cache_add (SwfdecCache *cache, SwfdecCached *cached)
{
  gsize needed_size;

  g_return_if_fail (SWFDEC_IS_CACHE (cache));
  g_return_if_fail (SWFDEC_IS_CACHED (cached));

  needed_size = swfdec_cached_get_size (cached);
  if (needed_size > cache->max_size)
    return;

  g_object_ref (cached);
  swfdec_cache_shrink (cache, cache->max_size - needed_size);
  cache->size += needed_size;
  g_signal_connect (cached, "use", G_CALLBACK (swfdec_cache_use_cached), cache);
  g_signal_connect (cached, "unuse", G_CALLBACK (swfdec_cache_unuse_cached), cache);
  g_queue_push_head (cache->queue, cached);
}

