
#ifndef _SWFDEC_CACHE_H_
#define _SWFDEC_CACHE_H_

#include <libswfdec/swfdec_types.h>

G_BEGIN_DECLS

//typedef struct _SwfdecCache SwfdecCache;
//typedef struct _SwfdecCacheHandle SwfdecCacheHandle;

struct _SwfdecCache {
  unsigned int	refcount;		/* reference count */
  unsigned int	max_size;		/* max size of cache */
  unsigned int	usage;			/* current size of cache */

  GQueue *	queue;			/* queue of loaded SwfdecCacheHandle, sorted by most recently used */
};

struct _SwfdecCacheHandle {
  unsigned int		size;	      	/* size of this item */

  GDestroyNotify	unload;		/* function called when unloading this handle */
};

SwfdecCache *	swfdec_cache_new		(unsigned int		max_size);
void		swfdec_cache_ref		(SwfdecCache *		cache);
void		swfdec_cache_unref		(SwfdecCache *		cache);

unsigned int	swfdec_cache_get_usage	  	(SwfdecCache *		cache);
void		swfdec_cache_shrink		(SwfdecCache *		cache,
						 unsigned int		max_usage);
void		swfdec_cache_add_handle		(SwfdecCache *	  	cache,
						 const SwfdecCacheHandle *handle);
void		swfdec_cache_remove_handle    	(SwfdecCache *	  	cache,
						 const SwfdecCacheHandle *handle);


G_END_DECLS

#endif
