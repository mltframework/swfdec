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

#ifndef _SWFDEC_CACHE_H_
#define _SWFDEC_CACHE_H_

#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS

//typedef struct _SwfdecCache SwfdecCache;
//typedef struct _SwfdecCacheHandle SwfdecCacheHandle;

struct _SwfdecCache {
  guint		refcount;		/* reference count */
  gulong	max_size;		/* max size of cache */
  gulong	usage;			/* current size of cache */

  GQueue *	queue;			/* queue of loaded SwfdecCacheHandle, sorted by most recently used */
};

struct _SwfdecCacheHandle {
  gulong	size;	          	/* size of this item */

  GDestroyNotify	unload;		/* function called when unloading this handle */
};

SwfdecCache *	swfdec_cache_new		(gulong			max_size);
void		swfdec_cache_ref		(SwfdecCache *		cache);
void		swfdec_cache_unref		(SwfdecCache *		cache);

gulong		swfdec_cache_get_size		(SwfdecCache *		cache);
void		swfdec_cache_set_size		(SwfdecCache *		cache,
						 gulong			max_usage);
gulong		swfdec_cache_get_usage	  	(SwfdecCache *		cache);
void		swfdec_cache_shrink		(SwfdecCache *		cache,
						 gulong			max_usage);
void		swfdec_cache_add_handle		(SwfdecCache *	  	cache,
						 const SwfdecCacheHandle *handle);
void		swfdec_cache_remove_handle    	(SwfdecCache *	  	cache,
						 const SwfdecCacheHandle *handle);


G_END_DECLS

#endif
