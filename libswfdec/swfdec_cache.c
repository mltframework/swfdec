
#include <swfdec_cache.h>

SwfdecCache *
swfdec_cache_new (void)
{
  return g_new0(SwfdecCache, 1);
}

void
swfdec_cache_free (SwfdecCache *cache)
{
  GList *g;

  for(g=cache->handles; g; g=g->next) {
    swfdec_handle_free(g->data);
  }
  g_free (cache);
}

int
swfdec_cache_get_usage (SwfdecCache *cache)
{
  int size = 0;
  GList *g;

  for(g=cache->handles; g; g=g->next) {
    SwfdecHandle *handle = g->data;
    size += handle->size;
  }

  return size;
}

void
swfdec_cache_unload_all (SwfdecCache *cache)
{
  GList *g;

  for(g=cache->handles; g; g=g->next) {
    swfdec_handle_unload(g->data);
  }
  g_free (cache);
}

void
swfdec_cache_add_handle (SwfdecCache *cache, SwfdecHandle *handle)
{
  cache->handles = g_list_prepend (cache->handles, handle);
}

void
swfdec_cache_remove_handle (SwfdecCache *cache, SwfdecHandle *handle)
{
  cache->handles = g_list_remove (cache->handles, handle);

}



void *
swfdec_handle_get_data (SwfdecHandle *handle)
{
  g_return_val_if_fail (handle != NULL, NULL);

  if (!handle->data) {
    handle->load (handle);
    if (handle->data == NULL) {
      g_warning ("handle load function did not load anything");
    }
  }

  return handle->data;
}

int
swfdec_handle_get_size (SwfdecHandle *handle)
{
  g_return_val_if_fail (handle != NULL, 0);
  return handle->size;
}

void *
swfdec_handle_get_private (SwfdecHandle *handle)
{
  g_return_val_if_fail (handle != NULL, NULL);
  return handle->priv;
}

gboolean
swfdec_handle_is_loaded (SwfdecHandle *handle)
{
  g_return_val_if_fail (handle != NULL, FALSE);
  return (handle->data != NULL);
}


SwfdecHandle *
swfdec_handle_new (SwfdecHandleLoadFunc load_func,
    SwfdecHandleFreeFunc free_func, void *priv)
{
  SwfdecHandle *handle;

  g_return_val_if_fail (free_func != NULL, NULL);
  g_return_val_if_fail (load_func != NULL, NULL);

  handle = g_new0 (SwfdecHandle, 1);

  handle->load = load_func;
  handle->free = free_func;
  handle->priv = priv;

  return handle;
}

void
swfdec_handle_free (SwfdecHandle *handle)
{
  g_return_if_fail (handle != NULL);
  if (handle->data) {
    handle->free (handle);
  }
  g_free(handle);
}

void
swfdec_handle_unload (SwfdecHandle *handle)
{
  g_return_if_fail (handle != NULL);
  if (handle->data) {
    handle->free (handle);
    handle->data = NULL;
    handle->size = 0;
  }
}

void
swfdec_handle_add_size (SwfdecHandle *handle, int size)
{
  g_return_if_fail (handle != NULL);
  handle->size += size;
}


void
swfdec_handle_set_data (SwfdecHandle *handle, void *data)
{
  g_return_if_fail (handle != NULL);
  handle->data = data;
}

