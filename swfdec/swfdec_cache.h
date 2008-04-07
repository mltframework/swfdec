/* Swfdec
 * Copyright (c) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_CACHE_H_
#define _SWFDEC_CACHE_H_

#include <cairo.h>
#include <swfdec/swfdec_cache.h>
#include <swfdec/swfdec_cached.h>

G_BEGIN_DECLS

typedef struct _SwfdecCache SwfdecCache;
typedef struct _SwfdecCacheClass SwfdecCacheClass;

#define SWFDEC_TYPE_CACHE                    (swfdec_cache_get_type())
#define SWFDEC_IS_CACHE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_CACHE))
#define SWFDEC_IS_CACHE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_CACHE))
#define SWFDEC_CACHE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_CACHE, SwfdecCache))
#define SWFDEC_CACHE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_CACHE, SwfdecCacheClass))
#define SWFDEC_CACHE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_CACHE, SwfdecCacheClass))


struct _SwfdecCache {
  GObject		object;

  gsize			max_size;	/* maximum amount of data allowed in cache, before starting to purge */
  gsize			size;		/* current amount of data in cache */

  GQueue *		queue;		/* queue of SwfdecCached, most recently used first */
};

struct _SwfdecCacheClass
{
  GObjectClass		object_class;
};

GType			swfdec_cache_get_type		(void);

SwfdecCache *		swfdec_cache_new		(gsize		max_size);

gsize			swfdec_cache_get_cache_size   	(SwfdecCache *	cache);
gsize			swfdec_cache_get_max_cache_size	(SwfdecCache *	cache);
void			swfdec_cache_set_max_cache_size	(SwfdecCache *	cache,
							 gsize		max_size);

void			swfdec_cache_shrink             (SwfdecCache *	cache,
							 gsize		size);

void			swfdec_cache_add		(SwfdecCache *	cache,
							 SwfdecCached *	cached);



G_END_DECLS
#endif
