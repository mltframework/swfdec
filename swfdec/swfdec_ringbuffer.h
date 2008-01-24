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

#ifndef __SWFDEC_RING_BUFFER_H__
#define __SWFDEC_RING_BUFFER_H__

#include <glib.h>

G_BEGIN_DECLS


typedef struct _SwfdecRingBuffer SwfdecRingBuffer;

struct _SwfdecRingBuffer
{
  unsigned char *	data;		/* our data */
  gsize			element_size;	/* size of one element */
  guint			size;		/* number of elements in the buffer */

  guint			head;		/* index of first element */
  guint			tail;		/* index after last element */
  guint			n_elements;	/* number of elements in ringbuffer */
};

SwfdecRingBuffer *	swfdec_ring_buffer_new		(guint			element_size,
							 guint			size);
#define swfdec_ring_buffer_new_for_type(element_type,size) \
  swfdec_ring_buffer_new (sizeof (element_type), (size))
void			swfdec_ring_buffer_free		(SwfdecRingBuffer *	buffer);

guint			swfdec_ring_buffer_get_size	(SwfdecRingBuffer *	buffer);
void			swfdec_ring_buffer_set_size	(SwfdecRingBuffer *	buffer,
							 guint			new_size);
guint			swfdec_ring_buffer_get_n_elements (SwfdecRingBuffer *	buffer);

gpointer		swfdec_ring_buffer_push		(SwfdecRingBuffer *	buffer);
gpointer		swfdec_ring_buffer_pop		(SwfdecRingBuffer *	buffer);
gpointer		swfdec_ring_buffer_peek_nth   	(SwfdecRingBuffer *	buffer,
							 guint			id);

G_END_DECLS

#endif
