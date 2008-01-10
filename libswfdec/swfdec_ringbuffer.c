/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "swfdec_ringbuffer.h"

SwfdecRingBuffer *
swfdec_ring_buffer_new (guint element_size, guint size)
{
  SwfdecRingBuffer *buffer;

  g_return_val_if_fail (element_size > 0, NULL);
  g_return_val_if_fail (size > 1, NULL);

  buffer = g_new0 (SwfdecRingBuffer, 1);
  buffer->element_size = element_size;
  buffer->size = size;
  buffer->data = g_malloc (element_size * size);

  return buffer;
}

void
swfdec_ring_buffer_free (SwfdecRingBuffer *buffer)
{
  g_return_if_fail (buffer != NULL);

  g_free (buffer->data);
  g_free (buffer);
}

guint
swfdec_ring_buffer_get_size (SwfdecRingBuffer *buffer)
{
  g_return_val_if_fail (buffer != NULL, 0);

  return buffer->size;
}

#define GET_ELEMENT(buffer,idx) (buffer->data + buffer->element_size * (idx))
void
swfdec_ring_buffer_set_size (SwfdecRingBuffer *buffer, guint new_size)
{
  g_return_if_fail (buffer != NULL);
  g_return_if_fail (buffer->size < new_size);

  buffer->data = g_realloc (buffer->data, buffer->element_size * new_size);
  if (buffer->tail <= buffer->head && buffer->n_elements) {
    memmove (GET_ELEMENT (buffer, buffer->head + new_size - buffer->size), 
	GET_ELEMENT (buffer, buffer->head), 
	buffer->element_size * (buffer->size - buffer->head));
    buffer->head += new_size - buffer->size;
  }
  buffer->size = new_size;
}

guint
swfdec_ring_buffer_get_n_elements (SwfdecRingBuffer *buffer)
{
  g_return_val_if_fail (buffer != NULL, 0);

  return buffer->n_elements;
}

gpointer
swfdec_ring_buffer_push (SwfdecRingBuffer *buffer)
{
  gpointer ret;

  g_return_val_if_fail (buffer != NULL, NULL);

  if (buffer->n_elements == buffer->size)
    return NULL;

  ret = GET_ELEMENT (buffer, buffer->tail);
  buffer->tail = (buffer->tail + 1) % buffer->size;
  buffer->n_elements++;
  return ret;
}

gpointer
swfdec_ring_buffer_pop (SwfdecRingBuffer *buffer)
{
  gpointer ret;

  g_return_val_if_fail (buffer != NULL, NULL);

  if (buffer->n_elements == 0)
    return NULL;

  ret = GET_ELEMENT (buffer, buffer->head);
  buffer->head = (buffer->head + 1) % buffer->size;
  buffer->n_elements--;
  return ret;
}

gpointer
swfdec_ring_buffer_peek_nth (SwfdecRingBuffer *buffer, guint id)
{
  g_return_val_if_fail (buffer != NULL, NULL);
  g_return_val_if_fail (id < buffer->n_elements, NULL);

  id = (buffer->head + id) % buffer->size;
  return GET_ELEMENT (buffer, id);
}
