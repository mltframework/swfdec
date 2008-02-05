/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef HAVE_CONFIG_H
#include "config.h"
#endif

#include <swfdec_buffer.h>
#include <glib.h>
#include <string.h>
#include <swfdec_debug.h>
#include <liboil/liboil.h>

/*** gtk-doc ***/

/**
 * SECTION:SwfdecBuffer
 * @title: SwfdecBuffer
 * @short_description: memory region handling
 *
 * This section describes how memory is to be handled when interacting with the 
 * Swfdec library. Memory regions are refcounted and passed using a 
 * #SwfdecBuffer. If large memory segments need to be handled that may span
 * multiple buffers, Swfdec uses a #SwfdecBufferQueue.
 */

/*** SwfdecBuffer ***/

/**
 * SwfdecBuffer:
 * @data: the data. read-only
 * @length: number of bytes in @data. read-only
 *
 * To allow for easy sharing of memory regions, #SwfdecBuffer was created. 
 * Every buffer refers to a memory region and its size and takes care of 
 * freeing that region when the buffer is no longer needed. They are 
 * reference countedto make it easy to refer to the same region from various
 * independant parts of your code. Buffers also support some advanced 
 * functionalities like extracting parts of the buffer using 
 * swfdec_buffer_new_subbuffer() or using mmapped files with 
 * swfdec_buffer_new_from_file() without the need for a different API.
 */

/**
 * SwfdecBufferFreeFunc:
 * @priv: The private data registered for passing to this function
 * @data: The data to free
 *
 * This is the function prototype for the function that is called for freeing
 * the memory pointed to by a buffer. See swfdec_buffer_new() for an example.
 */

GType
swfdec_buffer_get_type (void)
{
  static GType type_swfdec_buffer = 0;

  if (!type_swfdec_buffer)
    type_swfdec_buffer = g_boxed_type_register_static
      ("SwfdecBuffer", 
       (GBoxedCopyFunc) swfdec_buffer_ref,
       (GBoxedFreeFunc) swfdec_buffer_unref);

  return type_swfdec_buffer;
}

/**
 * swfdec_buffer_new:
 * @size: amount of bytes to allocate
 *
 * Creates a new buffer and allocates new memory of @size bytes to be used with 
 * the buffer.
 *
 * Returns: a new #SwfdecBuffer with buffer->data pointing to new data
 **/
SwfdecBuffer *
swfdec_buffer_new (gsize size)
{
  unsigned char *data = g_malloc (size);
  return swfdec_buffer_new_full (data, size, (SwfdecBufferFreeFunc) g_free, data);
}

/**
 * swfdec_buffer_new0:
 * @size: amount of bytes to allocate
 *
 * Createsa new buffer just like swfdec_buffer_new(), but ensures 
 * that the returned data gets initialized to be 0.
 *
 * Returns: a new #SwfdecBuffer with buffer->data pointing to new data
 **/
SwfdecBuffer *
swfdec_buffer_new0 (gsize size)
{
  unsigned char *data = g_malloc0 (size);
  return swfdec_buffer_new_full (data, size, (SwfdecBufferFreeFunc) g_free, data);
}

/**
 * swfdec_buffer_new_for_data:
 * @data: memory region allocated with g_malloc()
 * @size: size of @data in bytes
 *
 * Takes ownership of @data and creates a new buffer managing it.
 *
 * Returns: a new #SwfdecBuffer pointing to @data
 **/
SwfdecBuffer *
swfdec_buffer_new_for_data (guchar *data, gsize size)
{
  /* This is not a macro because a macro would evaluate the data pointer twice
   * and people like doing swfdec_buffer_new_for_data (g_malloc (10), 10);
   */
  return swfdec_buffer_new_full (data, size, (SwfdecBufferFreeFunc) g_free, data);
}
/**
 * swfdec_buffer_new_static:
 * @data: static data
 * @size: size of @data in bytes
 *
 * Creates a buffer for static data.
 *
 * Returns: a new #SwfdecBuffer pointing to @data
 **/
/**
 * swfdec_buffer_new_full:
 * @data: memory region to reference
 * @size: size of the provided memory region
 * @free_func: function to call for freeing the @data
 * @priv: private data to bass to @free_func
 *
 * Creates a new #SwfdecBuffer for managing @data. The provided @free_func
 * will be called when the returned buffer is not referenced anymore, the 
 * provided data needs to stay valid until that point.
 *
 * Returns: a new #SwfdecBuffer pointing to @data
 **/
SwfdecBuffer *
swfdec_buffer_new_full (unsigned char *data, gsize size, 
    SwfdecBufferFreeFunc free_func, gpointer priv)
{
  SwfdecBuffer *buffer;
  
  buffer = g_slice_new (SwfdecBuffer);
  buffer->ref_count = 1;
  buffer->data = data;
  buffer->length = size;
  buffer->free = free_func;
  buffer->priv = priv;

  return buffer;
}

/**
 * swfdec_buffer_new_subbuffer:
 * @buffer: #SwfdecBuffer managing the region of memory
 * @offset: starting offset into data
 * @length: amount of bytes to manage
 *
 * Creates a #SwfdecBuffer for managing a partial section of the memory pointed
 * to by @buffer.
 *
 * Returns: a new #SwfdecBuffer managing the indicated region.
 **/
SwfdecBuffer *
swfdec_buffer_new_subbuffer (SwfdecBuffer *buffer, gsize offset, gsize length)
{
  SwfdecBuffer *subbuffer;
  
  g_return_val_if_fail (buffer != NULL, NULL);
  g_return_val_if_fail (offset + length <= buffer->length, NULL);

  if (offset == 0 && length == buffer->length)
    return swfdec_buffer_ref (buffer);

  subbuffer = swfdec_buffer_new_full (buffer->data + offset, length,
      (SwfdecBufferFreeFunc) swfdec_buffer_unref, 
      swfdec_buffer_ref (swfdec_buffer_get_super (buffer)));

  return subbuffer;
}

/**
 * swfdec_buffer_get_super:
 * @buffer: a #SwfdecBuffer
 *
 * Returns the largest buffer that contains the memory pointed to by @buffer.
 * This will either be the passed @buffer itself, or if the buffer was created
 * via swfdec_buffer_new_subbuffer(), the buffer used for that.
 *
 * Returns: The largest @buffer available that contains the memory pointed to 
 *          by @buffer.
 **/
SwfdecBuffer *
swfdec_buffer_get_super (SwfdecBuffer *buffer)
{
  g_return_val_if_fail (buffer != NULL, NULL);

  if (buffer->free == (SwfdecBufferFreeFunc) swfdec_buffer_unref)
    buffer = buffer->priv;

  return buffer;
}

/**
 * swfdec_buffer_new_from_file:
 * @filename: file to read
 * @error: return location for a #GError or %NULL
 *
 * Creates a buffer containing the contents of the given file. If loading the
 * file fails, %NULL is returned and @error is set. The error can be
 * any of the errors that are valid for g_file_get_contents().
 *
 * Returns: a new #SwfdecBuffer or %NULL on failure
 **/
SwfdecBuffer *
swfdec_buffer_new_from_file (const char *filename, GError **error)
{
  GMappedFile *file;
  char *data;
  gsize length;

  g_return_val_if_fail (filename != NULL, NULL);

  file = g_mapped_file_new (filename, FALSE, NULL);
  if (file != NULL) {
    return swfdec_buffer_new_full ((guchar *) g_mapped_file_get_contents (file),
	g_mapped_file_get_length (file),
	(SwfdecBufferFreeFunc) g_mapped_file_free, file);
  }

  if (!g_file_get_contents (filename, &data, &length, error))
    return NULL;

  return swfdec_buffer_new_for_data ((guint8 *) data, length);
}

/**
 * swfdec_buffer_ref:
 * @buffer: a #SwfdecBuffer
 *
 * increases the reference count of @buffer by one.
 *
 * Returns: The passed in @buffer.
 **/
SwfdecBuffer *
swfdec_buffer_ref (SwfdecBuffer * buffer)
{
  g_return_val_if_fail (buffer != NULL, NULL);
  g_return_val_if_fail (buffer->ref_count > 0, NULL);

  buffer->ref_count++;
  return buffer;
}

/**
 * swfdec_buffer_unref:
 * @buffer: a #SwfdecBuffer
 *
 * Decreases the reference count of @buffer by one. If no reference to this
 * buffer exists anymore, the buffer and the memory it manages are freed.
 **/
void
swfdec_buffer_unref (SwfdecBuffer * buffer)
{
  g_return_if_fail (buffer != NULL);
  g_return_if_fail (buffer->ref_count > 0);

  buffer->ref_count--;
  if (buffer->ref_count == 0) {
    if (buffer->free)
      buffer->free (buffer->priv, buffer->data);
    g_slice_free (SwfdecBuffer, buffer);
  }
}

/*** SwfdecBufferQueue ***/

/**
 * SwfdecBufferQueue:
 *
 * A #SwfdecBufferQueue is a queue of continuous buffers that allows reading
 * its data in chunks of pre-defined sizes. It is used to transform a data 
 * stream that was provided by buffers of random sizes to buffers of the right
 * size.
 */

GType
swfdec_buffer_queue_get_type (void)
{
  static GType type_swfdec_buffer_queue = 0;

  if (!type_swfdec_buffer_queue)
    type_swfdec_buffer_queue = g_boxed_type_register_static
      ("SwfdecBufferQueue", 
       (GBoxedCopyFunc) swfdec_buffer_queue_ref,
       (GBoxedFreeFunc) swfdec_buffer_queue_unref);

  return type_swfdec_buffer_queue;
}

/**
 * swfdec_buffer_queue_new:
 *
 * Creates a new empty buffer queue.
 *
 * Returns: a new buffer queue. Use swfdec_buffer_queue_unref () to free it.
 **/
SwfdecBufferQueue *
swfdec_buffer_queue_new (void)
{
  SwfdecBufferQueue *buffer_queue;

  buffer_queue = g_new0 (SwfdecBufferQueue, 1);
  buffer_queue->ref_count = 1;
  return buffer_queue;
}

/**
 * swfdec_buffer_queue_get_depth:
 * @queue: a #SwfdecBufferQueue
 *
 * Returns the number of bytes currently in @queue.
 *
 * Returns: amount of bytes in @queue.
 **/
gsize
swfdec_buffer_queue_get_depth (SwfdecBufferQueue * queue)
{
  g_return_val_if_fail (queue != NULL, 0);

  return queue->depth;
}

/**
 * swfdec_buffer_queue_get_offset:
 * @queue: a #SwfdecBufferQueue
 *
 * Queries the amount of bytes that has already been pulled out of
 * @queue using functions like swfdec_buffer_queue_pull().
 *
 * Returns: Number of bytes that were already pulled from this queue.
 **/
gsize
swfdec_buffer_queue_get_offset (SwfdecBufferQueue * queue)
{
  g_return_val_if_fail (queue != NULL, 0);

  return queue->offset;
}

/**
 * swfdec_buffer_queue_flush:
 * @queue: a #SwfdecBufferQueue
 * @n_bytes: amount of bytes to flush from the queue
 *
 * Removes the first @n_bytes bytes from the queue.
 */
void
swfdec_buffer_queue_flush (SwfdecBufferQueue *queue, gsize n_bytes)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (n_bytes <= queue->depth);

  queue->depth -= n_bytes;
  queue->offset += n_bytes;

  SWFDEC_LOG ("flushing %zu bytes (%zu left)", n_bytes, queue->depth);

  while (n_bytes > 0) {
    SwfdecBuffer *buffer = queue->first_buffer->data;

    if (buffer->length <= n_bytes) {
      n_bytes -= buffer->length;
      queue->first_buffer = g_slist_remove (queue->first_buffer, buffer);
    } else {
      queue->first_buffer->data = swfdec_buffer_new_subbuffer (buffer, 
	  n_bytes, buffer->length - n_bytes);
      n_bytes = 0;
    }
    swfdec_buffer_unref (buffer);
  }
  if (queue->first_buffer == NULL)
    queue->last_buffer = NULL;
}

/**
 * swfdec_buffer_queue_clear:
 * @queue: a #SwfdecBufferQueue
 *
 * Resets @queue into to initial state. All buffers it contains will be 
 * released and the offset will be reset to 0.
 **/
void
swfdec_buffer_queue_clear (SwfdecBufferQueue *queue)
{
  g_return_if_fail (queue != NULL);

  g_slist_foreach (queue->first_buffer, (GFunc) swfdec_buffer_unref, NULL);
  g_slist_free (queue->first_buffer);
  queue->first_buffer = NULL;
  queue->last_buffer = NULL;
  queue->depth = 0;
  queue->offset = 0;
}

/**
 * swfdec_buffer_queue_push:
 * @queue: a #SwfdecBufferQueue
 * @buffer: #SwfdecBuffer to append to @queue
 *
 * Appends the given @buffer to the buffers already in @queue. This function
 * will take ownership of the given @buffer. Use swfdec_buffer_ref () before
 * calling this function to keep a reference.
 **/
void
swfdec_buffer_queue_push (SwfdecBufferQueue * queue, SwfdecBuffer * buffer)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (buffer != NULL);

  if (buffer->length == 0) {
    swfdec_buffer_unref (buffer);
    return;
  }
  queue->last_buffer = g_slist_append (queue->last_buffer, buffer);
  if (queue->first_buffer == NULL) {
    queue->first_buffer = queue->last_buffer;
  } else {
    queue->last_buffer = queue->last_buffer->next;
  }
  queue->depth += buffer->length;
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
swfdec_buffer_queue_peek (SwfdecBufferQueue * queue, gsize length)
{
  GSList *g;
  SwfdecBuffer *newbuffer;
  SwfdecBuffer *buffer;

  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->depth < length)
    return NULL;

  SWFDEC_LOG ("peeking %zu, %zu available", length, queue->depth);

  /* need to special case here, because the queue may be empty */
  if (length == 0)
    return swfdec_buffer_new (0);

  g = queue->first_buffer;
  buffer = g->data;
  if (buffer->length >= length) {
    newbuffer = swfdec_buffer_new_subbuffer (buffer, 0, length);
  } else {
    gsize amount, offset;
    newbuffer = swfdec_buffer_new (length);
    offset = 0;
    while (offset < length) {
      buffer = g->data;
      amount = MIN (length - offset, buffer->length);
      oil_copy_u8 (newbuffer->data + offset, buffer->data, amount);
      offset += amount;
      g = g->next;
    }
  }

  return newbuffer;
}

/**
 * swfdec_buffer_queue_pull:
 * @queue: a #SwfdecBufferQueue
 * @length: amount of bytes to pull
 *
 * If enough data is still available in @queue, the first @length bytes are 
 * put into a new buffer and that buffer is returned. The @length bytes are
 * removed from the head of the queue. If not enough data is available, %NULL
 * is returned.
 *
 * Returns: a new #SwfdecBuffer or %NULL
 **/
SwfdecBuffer *
swfdec_buffer_queue_pull (SwfdecBufferQueue * queue, gsize length)
{
  SwfdecBuffer *ret;

  g_return_val_if_fail (queue != NULL, NULL);

  ret = swfdec_buffer_queue_peek (queue, length);
  if (ret == NULL)
    return NULL;

  swfdec_buffer_queue_flush (queue, length);
  return ret;
}

/**
 * swfdec_buffer_queue_peek_buffer:
 * @queue: a #SwfdecBufferQueue
 *
 * Gets the first buffer out of @queue and returns it. This function is 
 * equivalent to calling swfdec_buffer_queue_peek() with the size of the
 * first buffer in it.
 *
 * Returns: The first buffer in @queue or %NULL if @queue is empty. Use 
 *          swfdec_buffer_unref() after use.
 **/
SwfdecBuffer *
swfdec_buffer_queue_peek_buffer (SwfdecBufferQueue * queue)
{
  SwfdecBuffer *buffer;

  g_return_val_if_fail (queue != NULL, NULL);

  if (queue->first_buffer == NULL)
    return NULL;

  buffer = queue->first_buffer->data;
  SWFDEC_LOG ("peeking one buffer: %zu bytes, %zu available", buffer->length, queue->depth);

  return swfdec_buffer_ref (buffer);
}

/**
 * swfdec_buffer_queue_pull_buffer:
 * @queue: a #SwfdecBufferQueue
 *
 * Pulls the first buffer out of @queue and returns it. This function is 
 * equivalent to calling swfdec_buffer_queue_pull() with the size of the
 * first buffer in it.
 *
 * Returns: The first buffer in @queue or %NULL if @queue is empty.
 **/
SwfdecBuffer *
swfdec_buffer_queue_pull_buffer (SwfdecBufferQueue *queue)
{
  SwfdecBuffer *buffer;

  g_return_val_if_fail (queue != NULL, NULL);

  buffer = swfdec_buffer_queue_peek_buffer (queue);
  if (buffer)
    swfdec_buffer_queue_flush (queue, buffer->length);

  return buffer;
}

/**
 * swfdec_buffer_queue_ref:
 * @queue: a #SwfdecBufferQueue
 *
 * increases the reference count of @queue by one.
 *
 * Returns: The passed in @queue.
 **/
SwfdecBufferQueue *
swfdec_buffer_queue_ref (SwfdecBufferQueue * queue)
{
  g_return_val_if_fail (queue != NULL, NULL);
  g_return_val_if_fail (queue->ref_count > 0, NULL);

  queue->ref_count++;
  return queue;
}

/**
 * swfdec_buffer_queue_unref:
 * @queue: a #SwfdecBufferQueue
 *
 * Decreases the reference count of @queue by one. If no reference 
 * to this buffer exists anymore, the buffer and the memory 
 * it manages are freed.
 **/
void
swfdec_buffer_queue_unref (SwfdecBufferQueue * queue)
{
  g_return_if_fail (queue != NULL);
  g_return_if_fail (queue->ref_count > 0);

  queue->ref_count--;
  if (queue->ref_count == 0) {
    swfdec_buffer_queue_clear (queue);
    g_free (queue);
  }
}

typedef struct {
  const char		*name;
  guint			length;
  guchar		data[4];
} ByteOrderMark;

static ByteOrderMark boms[] = {
  { "UTF-8", 3, {0xEF, 0xBB, 0xBF, 0} },
  { "UTF-16BE", 2, {0xFE, 0xFF, 0, 0} },
  { "UTF-16LE", 2, {0xFF, 0xFE, 0, 0} },
  { "UTF-8", 0, {0, 0, 0, 0} }
};

char *
swfdec_buffer_queue_pull_text (SwfdecBufferQueue *queue, guint version)
{
  SwfdecBuffer *buffer;
  char *text;
  guint size, i, j;

  size = swfdec_buffer_queue_get_depth (queue);
  if (size == 0) {
    SWFDEC_LOG ("empty loader, returning empty string");
    return g_strdup ("");
  }

  buffer = swfdec_buffer_queue_pull (queue, size);
  g_assert (buffer);

  if (version > 5) {
    for (i = 0; boms[i].length > 0; i++) {
      // FIXME: test what happens if we have BOM and nothing else
      if (size < boms[i].length)
	continue;

      for (j = 0; j < boms[i].length; j++) {
	if (buffer->data[j] != boms[i].data[j])
	  break;
      }
      if (j == boms[i].length)
	break;
    }

    if (!strcmp (boms[i].name, "UTF-8")) {
      if (!g_utf8_validate ((char *)buffer->data + boms[i].length,
	    size - boms[i].length, NULL)) {
	SWFDEC_ERROR ("downloaded data is not valid UTF-8");
	text = NULL;
      } else {
	text =
	  g_strndup ((char *)buffer->data + boms[i].length,
	      size - boms[i].length);
      }
    } else {
      text = g_convert ((char *)buffer->data + boms[i].length,
	  size - boms[i].length, "UTF-8", boms[i].name, NULL, NULL, NULL);
      if (text == NULL)
	SWFDEC_ERROR ("downloaded data is not valid %s", boms[i].name);
    }
  } else {
    text = g_convert ((char *)buffer->data, size, "UTF-8", "LATIN1", NULL,
	NULL, NULL);
    if (text == NULL)
      SWFDEC_ERROR ("downloaded data is not valid LATIN1");
  }

  swfdec_buffer_unref (buffer);

  return text;
}

