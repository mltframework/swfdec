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
 * @data: The data to free
 * @priv: The private data registered for passing to this function
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
 *
 * Creates a new #SwfdecBuffer to be filled by the user. Use like this:
 * <informalexample><programlisting>SwfdecBuffer *buffer = swfdec_buffer_new ();
 * buffer->data = mydata;
 * buffer->length = mydata_length;
 * buffer->free = mydata_freefunc;
 * buffer->priv = mydata_private;</programlisting></informalexample>
 *
 * Returns: a new #SwfdecBuffer referencing nothing.
 **/
SwfdecBuffer *
swfdec_buffer_new (void)
{
  SwfdecBuffer *buffer;

  buffer = g_new0 (SwfdecBuffer, 1);
  buffer->ref_count = 1;
  return buffer;
}

/**
 * swfdec_buffer_new_and_alloc:
 * @size: amount of bytes to allocate
 *
 * Creates a new buffer and allocates new memory of @size bytes to be used with 
 * the buffer.
 *
 * Returns: a new #SwfdecBuffer with buffer->data pointing to new data
 **/
SwfdecBuffer *
swfdec_buffer_new_and_alloc (guint size)
{
  SwfdecBuffer *buffer = swfdec_buffer_new ();

  buffer->data = g_malloc (size);
  buffer->length = size;
  buffer->free = (SwfdecBufferFreeFunc) g_free;

  return buffer;
}

/**
 * swfdec_buffer_new_and_alloc0:
 * @size: amount of bytes to allocate
 *
 * Createsa new buffer just like swfdec_buffer_new_and_alloc(), but ensures 
 * that the returned data gets initialized to be 0.
 *
 * Returns: a new #SwfdecBuffer with buffer->data pointing to new data
 **/
SwfdecBuffer *
swfdec_buffer_new_and_alloc0 (guint size)
{
  SwfdecBuffer *buffer = swfdec_buffer_new ();

  buffer->data = g_malloc0 (size);
  buffer->length = size;
  buffer->free = (SwfdecBufferFreeFunc) g_free;

  return buffer;
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
swfdec_buffer_new_for_data (unsigned char *data, guint size)
{
  SwfdecBuffer *buffer;
  
  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (size > 0, NULL);

  buffer = swfdec_buffer_new ();
  buffer->data = data;
  buffer->length = size;
  buffer->free = (SwfdecBufferFreeFunc) g_free;

  return buffer;
}

static void
swfdec_buffer_free_subbuffer (unsigned char *data, gpointer priv)
{
  swfdec_buffer_unref (priv);
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
swfdec_buffer_new_subbuffer (SwfdecBuffer * buffer, guint offset, guint length)
{
  SwfdecBuffer *subbuffer;
  
  g_return_val_if_fail (buffer != NULL, NULL);
  g_return_val_if_fail (offset + length <= buffer->length, NULL);

  subbuffer = swfdec_buffer_new ();

  subbuffer->priv = swfdec_buffer_ref (swfdec_buffer_get_super (buffer));
  subbuffer->data = buffer->data + offset;
  subbuffer->length = length;
  subbuffer->free = swfdec_buffer_free_subbuffer;

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

  if (buffer->free == swfdec_buffer_free_subbuffer)
    buffer = buffer->priv;

  g_assert (buffer->free != swfdec_buffer_free_subbuffer);
  return buffer;
}

static void
swfdec_buffer_free_mapped (unsigned char *data, gpointer priv)
{
  g_mapped_file_free (priv);
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
    SwfdecBuffer *buffer = swfdec_buffer_new ();
    buffer->data = (unsigned char *) g_mapped_file_get_contents (file), 
    buffer->length = g_mapped_file_get_length (file);
    buffer->free = swfdec_buffer_free_mapped;
    buffer->priv = file;
    return buffer;
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
      buffer->free (buffer->data, buffer->priv);
    g_free (buffer);
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
guint
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
guint
swfdec_buffer_queue_get_offset (SwfdecBufferQueue * queue)
{
  g_return_val_if_fail (queue != NULL, 0);

  return queue->offset;
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

  g_list_foreach (queue->buffers, (GFunc) swfdec_buffer_unref, NULL);
  g_list_free (queue->buffers);
  memset (queue, 0, sizeof (SwfdecBufferQueue));
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
  queue->buffers = g_list_append (queue->buffers, buffer);
  queue->depth += buffer->length;
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
swfdec_buffer_queue_pull_buffer (SwfdecBufferQueue * queue)
{
  SwfdecBuffer *buffer;

  g_return_val_if_fail (queue != NULL, NULL);
  if (queue->buffers == NULL)
    return NULL;

  buffer = queue->buffers->data;

  return swfdec_buffer_queue_pull (queue, buffer->length);
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
swfdec_buffer_queue_pull (SwfdecBufferQueue * queue, guint length)
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
    guint offset = 0;

    newbuffer = swfdec_buffer_new_and_alloc (length);

    while (offset < length) {
      g = g_list_first (queue->buffers);
      buffer = g->data;

      if (buffer->length > length - offset) {
        guint n = length - offset;

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
swfdec_buffer_queue_peek (SwfdecBufferQueue * queue, guint length)
{
  GList *g;
  SwfdecBuffer *newbuffer;
  SwfdecBuffer *buffer;
  guint offset = 0;

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

