
#ifndef HAVE_CONFIG_H
#include "config.h"
#endif

#include <swfdec_buffer.h>
#include <glib.h>

static void swfdec_buffer_free_mem (SwfdecBuffer *buffer, void *);
static void swfdec_buffer_free_subbuffer (SwfdecBuffer *buffer, void *priv);


SwfdecBuffer *
swfdec_buffer_new (void)
{
  SwfdecBuffer *buffer;
  buffer = g_new0(SwfdecBuffer, 1);
  buffer->ref_count = 1;
  return buffer;
}

SwfdecBuffer *
swfdec_buffer_new_and_alloc (int size)
{
  SwfdecBuffer *buffer = swfdec_buffer_new();

  buffer->data = g_malloc (size);
  buffer->length = size;
  buffer->free = swfdec_buffer_free_mem;

  return buffer;
}

SwfdecBuffer *
swfdec_buffer_new_with_data (void *data, int size)
{
  SwfdecBuffer *buffer = swfdec_buffer_new();

  buffer->data = data;
  buffer->length = size;
  buffer->free = swfdec_buffer_free_mem;

  return buffer;
}

SwfdecBuffer *
swfdec_buffer_new_subbuffer (SwfdecBuffer *buffer, int offset, int length)
{
  SwfdecBuffer *subbuffer = swfdec_buffer_new();

  if (buffer->parent) {
    swfdec_buffer_ref (buffer->parent);
    subbuffer->parent = buffer->parent;
  } else {
    swfdec_buffer_ref (buffer);
    subbuffer->parent = buffer;
  }
  subbuffer->data = buffer->data + offset;
  subbuffer->length = length;
  subbuffer->free = swfdec_buffer_free_subbuffer;

  return subbuffer;
}

void
swfdec_buffer_ref (SwfdecBuffer *buffer)
{
  buffer->ref_count++;
}

void
swfdec_buffer_unref (SwfdecBuffer *buffer)
{
  buffer->ref_count--;
  if (buffer->ref_count == 0) {
    if (buffer->free) buffer->free (buffer, buffer->priv);
    g_free (buffer);
  }
}

static void
swfdec_buffer_free_mem (SwfdecBuffer *buffer, void *priv)
{
  g_free(buffer->data);
}

static void
swfdec_buffer_free_subbuffer (SwfdecBuffer *buffer, void *priv)
{
  swfdec_buffer_unref (buffer->parent);
}



