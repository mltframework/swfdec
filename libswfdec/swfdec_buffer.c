
#ifndef HAVE_CONFIG_H
#include "config.h"
#endif

#include <swfdec_buffer.h>
#include <glib.h>
#include <string.h>
#include <swfdec_debug.h>
#include <liboil/liboil.h>

static void swfdec_buffer_free_mem (SwfdecBuffer * buffer, void *);
static void swfdec_buffer_free_subbuffer (SwfdecBuffer * buffer, void *priv);


SwfdecBuffer *
swfdec_buffer_new (void)
{
  SwfdecBuffer *buffer;

  buffer = g_new0 (SwfdecBuffer, 1);
  buffer->ref_count = 1;
  return buffer;
}

SwfdecBuffer *
swfdec_buffer_new_and_alloc (unsigned int size)
{
  SwfdecBuffer *buffer = swfdec_buffer_new ();

  buffer->data = g_malloc (size);
  buffer->length = size;
  buffer->free = swfdec_buffer_free_mem;

  return buffer;
}

SwfdecBuffer *
swfdec_buffer_new_and_alloc0 (unsigned int size)
{
  SwfdecBuffer *buffer = swfdec_buffer_new ();

  buffer->data = g_malloc0 (size);
  buffer->length = size;
  buffer->free = swfdec_buffer_free_mem;

  return buffer;
}

SwfdecBuffer *
swfdec_buffer_new_with_data (void *data, int size)
{
  SwfdecBuffer *buffer = swfdec_buffer_new ();

  buffer->data = data;
  buffer->length = size;
  buffer->free = swfdec_buffer_free_mem;

  return buffer;
}

SwfdecBuffer *
swfdec_buffer_new_subbuffer (SwfdecBuffer * buffer, unsigned int offset, unsigned int length)
{
  SwfdecBuffer *subbuffer;
  
  g_return_val_if_fail (offset + length <= buffer->length, NULL);

  subbuffer = swfdec_buffer_new ();

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

static void
swfdec_buffer_free_mapped (SwfdecBuffer * buffer, void *priv)
{
  g_mapped_file_free (priv);
}

SwfdecBuffer *
swfdec_buffer_new_from_file (const char *filename, GError **error)
{
  GMappedFile *file;
  SwfdecBuffer *buffer;

  g_return_val_if_fail (filename != NULL, NULL);

  file = g_mapped_file_new (filename, FALSE, error);
  if (file == NULL) {
    return NULL;
  }

  buffer = swfdec_buffer_new ();
  buffer->data = (unsigned char *) g_mapped_file_get_contents (file), 
  buffer->length = g_mapped_file_get_length (file);
  buffer->free = swfdec_buffer_free_mapped;
  buffer->priv = file;

  return buffer;
}

SwfdecBuffer *
swfdec_buffer_ref (SwfdecBuffer * buffer)
{
  buffer->ref_count++;
  return buffer;
}

void
swfdec_buffer_unref (SwfdecBuffer * buffer)
{
  buffer->ref_count--;
  if (buffer->ref_count == 0) {
    if (buffer->free)
      buffer->free (buffer, buffer->priv);
    g_free (buffer);
  }
}

static void
swfdec_buffer_free_mem (SwfdecBuffer * buffer, void *priv)
{
  g_free (buffer->data);
}

static void
swfdec_buffer_free_subbuffer (SwfdecBuffer * buffer, void *priv)
{
  swfdec_buffer_unref (buffer->parent);
}


SwfdecBufferQueue *
swfdec_buffer_queue_new (void)
{
  return g_new0 (SwfdecBufferQueue, 1);
}

int
swfdec_buffer_queue_get_depth (SwfdecBufferQueue * queue)
{
  return queue->depth;
}

int
swfdec_buffer_queue_get_offset (SwfdecBufferQueue * queue)
{
  return queue->offset;
}

void
swfdec_buffer_queue_clear (SwfdecBufferQueue *queue)
{
  g_list_foreach (queue->buffers, (GFunc) swfdec_buffer_unref, NULL);
  g_list_free (queue->buffers);
  memset (queue, 0, sizeof (SwfdecBufferQueue));
}

void
swfdec_buffer_queue_free (SwfdecBufferQueue * queue)
{
  swfdec_buffer_queue_clear (queue);
  g_free (queue);
}

void
swfdec_buffer_queue_push (SwfdecBufferQueue * queue, SwfdecBuffer * buffer)
{
  queue->buffers = g_list_append (queue->buffers, buffer);
  queue->depth += buffer->length;
}

SwfdecBuffer *
swfdec_buffer_queue_pull_buffer (SwfdecBufferQueue * queue)
{
  SwfdecBuffer *buffer;

  g_return_val_if_fail (queue != NULL, NULL);
  if (queue->buffers == NULL)
    return NULL;

  buffer = queue->buffers->data;

  return swfdec_buffer_queue_pull (queue, buffer->length);
}

SwfdecBuffer *
swfdec_buffer_queue_pull (SwfdecBufferQueue * queue, unsigned int length)
{
  GList *g;
  SwfdecBuffer *newbuffer;
  SwfdecBuffer *buffer;
  SwfdecBuffer *subbuffer;

  g_return_val_if_fail (queue != NULL, NULL);
  g_return_val_if_fail (length > 0, NULL);

  if (queue->depth < length) 
    return NULL;

  /* FIXME: This function should share code with swfdec_buffer_queue_peek */
  SWFDEC_LOG ("pulling %d, %d available", length, queue->depth);

  g = g_list_first (queue->buffers);
  buffer = g->data;

  if (buffer->length > length) {
    newbuffer = swfdec_buffer_new_subbuffer (buffer, 0, length);

    subbuffer = swfdec_buffer_new_subbuffer (buffer, length,
        buffer->length - length);
    g->data = subbuffer;
    swfdec_buffer_unref (buffer);
  } else if (buffer->length == length) {
    queue->buffers = g_list_remove (queue->buffers, buffer);
    newbuffer = buffer;
  } else {
    unsigned int offset = 0;

    newbuffer = swfdec_buffer_new_and_alloc (length);

    while (offset < length) {
      g = g_list_first (queue->buffers);
      buffer = g->data;

      if (buffer->length > length - offset) {
        unsigned int n = length - offset;

        oil_copy_u8 (newbuffer->data + offset, buffer->data, n);
        subbuffer = swfdec_buffer_new_subbuffer (buffer, n, buffer->length - n);
        g->data = subbuffer;
        swfdec_buffer_unref (buffer);
        offset += n;
      } else {
        oil_copy_u8 (newbuffer->data + offset, buffer->data, buffer->length);

        queue->buffers = g_list_delete_link (queue->buffers, g);
        offset += buffer->length;
	swfdec_buffer_unref (buffer);
      }
    }
  }

  queue->depth -= length;
  queue->offset += length;

  return newbuffer;
}

/**
 * swfdec_buffer_queue_peek:
 * @queue: a #SwfdecBufferQueue to read from
 * @length: amount of bytes to peek
 *
 * Creates a new buffer with the first @length bytes from @queue, but unlike 
 * swfdec_buffer_queue_pull(), does not remove them from @queue.
 *
 * Returns: NULL if the requested amount of data wasn't available or a new 
 *          readonly #SwfdecBuffer. Use swfdec_buffer_unref() after use.
 **/
SwfdecBuffer *
swfdec_buffer_queue_peek (SwfdecBufferQueue * queue, unsigned int length)
{
  GList *g;
  SwfdecBuffer *newbuffer;
  SwfdecBuffer *buffer;
  unsigned int offset = 0;

  g_return_val_if_fail (length > 0, NULL);

  if (queue->depth < length)
    return NULL;

  SWFDEC_LOG ("peeking %d, %d available", length, queue->depth);

  g = g_list_first (queue->buffers);
  buffer = g->data;
  if (buffer->length > length) {
    newbuffer = swfdec_buffer_new_subbuffer (buffer, 0, length);
  } else {
    newbuffer = swfdec_buffer_new_and_alloc (length);
    while (offset < length) {
      buffer = g->data;

      if (buffer->length > length - offset) {
        int n = length - offset;

        oil_copy_u8 (newbuffer->data + offset, buffer->data, n);
        offset += n;
      } else {
        oil_copy_u8 (newbuffer->data + offset, buffer->data, buffer->length);
        offset += buffer->length;
      }
      g = g_list_next (g);
    }
  }

  return newbuffer;
}
