
#ifndef HAVE_CONFIG_H
#include "config.h"
#endif

#include <swfdec_buffer.h>
#include <glib.h>
#include <string.h>
#include <swfdec_debug.h>

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


SwfdecBufferQueue *
swfdec_buffer_queue_new (void)
{
  return g_new0(SwfdecBufferQueue, 1);
}

int
swfdec_buffer_queue_get_depth (SwfdecBufferQueue *queue)
{
  return queue->depth;
}

void
swfdec_buffer_queue_free (SwfdecBufferQueue *queue)
{
  GList *g;
  for (g=g_list_first(queue->buffers);g;g=g_list_next(g)) {
    swfdec_buffer_unref((SwfdecBuffer *)g->data);
  }
  g_list_free (queue->buffers);
}

void
swfdec_buffer_queue_push (SwfdecBufferQueue *queue, SwfdecBuffer *buffer)
{
  queue->buffers = g_list_append (queue->buffers, buffer);
  queue->depth += buffer->length;
}

SwfdecBuffer *
swfdec_buffer_queue_pull (SwfdecBufferQueue *queue, int length)
{
  GList *g;
  SwfdecBuffer *newbuffer;
  SwfdecBuffer *buffer;
  SwfdecBuffer *subbuffer;

  g_return_val_if_fail (length > 0, NULL);

  if (queue->depth < length) {
    return NULL;
  }

  SWFDEC_DEBUG ("pulling %d, %d available", length, queue->depth);

  g = g_list_first (queue->buffers);
  buffer = g->data;

  if (buffer->length > length) {
    newbuffer = swfdec_buffer_new_subbuffer (buffer, 0, length);

    subbuffer = swfdec_buffer_new_subbuffer (buffer, length,
        buffer->length - length);
    g->data = subbuffer;
    swfdec_buffer_unref (buffer);
  } else {
    int offset = 0;

    newbuffer = swfdec_buffer_new_and_alloc (length);

    while(offset < length) {
      g = g_list_first (queue->buffers);
      buffer = g->data;
      
      if (buffer->length > length - offset) {
        int n = length - offset;

        memcpy (newbuffer->data + offset, buffer->data, n);
        subbuffer = swfdec_buffer_new_subbuffer (buffer, n, buffer->length - n);
        g->data = subbuffer;
        swfdec_buffer_unref (buffer);
        offset += n;
      } else {
        memcpy (newbuffer->data + offset, buffer->data, buffer->length);

        queue->buffers = g_list_delete_link (queue->buffers, g);
        offset += buffer->length;
      }
    }
  }

  queue->depth -= length;

  return newbuffer;
}

SwfdecBuffer *
swfdec_buffer_queue_peek (SwfdecBufferQueue *queue, int length)
{
  GList *g;
  SwfdecBuffer *newbuffer;
  SwfdecBuffer *buffer;
  int offset = 0;

  g_return_val_if_fail (length > 0, NULL);

  if (queue->depth < length) {
    return NULL;
  }

  SWFDEC_DEBUG ("peeking %d, %d available", length, queue->depth);

  g = g_list_first (queue->buffers);
  buffer = g->data;
  if (buffer->length > length) {
    newbuffer = swfdec_buffer_new_subbuffer (buffer, 0, length);
  } else {
    newbuffer = swfdec_buffer_new_and_alloc (length);
    while(offset < length) {
      buffer = g->data;
      
      if (buffer->length > length - offset) {
        int n = length - offset;

        memcpy (newbuffer->data + offset, buffer->data, n);
        offset += n;
      } else {
        memcpy (newbuffer->data + offset, buffer->data, buffer->length);
        offset += buffer->length;
      }
      g = g_list_next (g);
    }
  }

  return newbuffer;
}

