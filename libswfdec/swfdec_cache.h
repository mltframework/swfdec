
#ifndef _SWFDEC_CACHE_H_
#define _SWFDEC_CACHE_H_

#include <swfdec_object.h>
#include "swfdec_types.h"

G_BEGIN_DECLS

//typedef struct _SwfdecCache SwfdecCache;
//typedef struct _SwfdecHandle SwfdecHandle;

typedef void (*SwfdecHandleLoadFunc)(SwfdecHandle *handle);
typedef void (*SwfdecHandleFreeFunc)(SwfdecHandle *handle);

struct _SwfdecCache
{
  GList *handles;

};

struct _SwfdecHandle
{
  void *data;
  int size;

  SwfdecHandleLoadFunc load;
  SwfdecHandleFreeFunc free;
  void *priv;
};

SwfdecCache *swfdec_cache_new (void);
void swfdec_cache_free (SwfdecCache *cache);
int swfdec_cache_get_usage (SwfdecCache *cache);
void swfdec_cache_unload_all (SwfdecCache *cache);
void swfdec_cache_add_handle (SwfdecCache *cache, SwfdecHandle *handle);
void swfdec_cache_remove_handle (SwfdecCache *cache, SwfdecHandle *handle);


void *swfdec_handle_get_data (SwfdecHandle *handle);
int swfdec_handle_get_size (SwfdecHandle *handle);
void *swfdec_handle_get_private (SwfdecHandle *handle);
gboolean swfdec_handle_is_loaded (SwfdecHandle *handle);

SwfdecHandle *swfdec_handle_new (SwfdecHandleLoadFunc load_func,
    SwfdecHandleFreeFunc free_func, void *priv);
void swfdec_handle_free (SwfdecHandle *handle);
void swfdec_handle_unload (SwfdecHandle *handle);
void swfdec_handle_add_size (SwfdecHandle *handle, int size);

void swfdec_handle_set_data (SwfdecHandle *handle, void *data);


G_END_DECLS

#endif
