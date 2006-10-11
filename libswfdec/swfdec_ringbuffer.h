
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
