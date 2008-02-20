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

#ifndef __SWFDEC_BUFFER_H__
#define __SWFDEC_BUFFER_H__

#include <glib.h>
#include <glib-object.h>

typedef struct _SwfdecBuffer SwfdecBuffer;
typedef struct _SwfdecBufferQueue SwfdecBufferQueue;

typedef void (* SwfdecBufferFreeFunc) (gpointer *priv, unsigned char *data);

struct _SwfdecBuffer
{
  unsigned char *	data;		/* memory region (consider as read only) */
  gsize			length;		/* length of the memory region pointer do by @data */

  /*< private >*/
  int			ref_count;	/* guess */

  SwfdecBufferFreeFunc	free;		/* function to call to free @data */
  gpointer		priv;	      	/* data to pass to @free */
};

#define SWFDEC_TYPE_BUFFER swfdec_buffer_get_type()
GType swfdec_buffer_get_type  (void);

struct _SwfdecBufferQueue
{
  /*< private >*/
  GSList *	first_buffer;		/* pointer to first buffer */
  GSList *	last_buffer;		/* pointer to last buffer (for fast appending) */
  gsize		depth;			/* amount of bytes in the queue */
  gsize		offset;			/* amount of data already flushed out of the queue */
  
  int		ref_count;
};

#define SWFDEC_TYPE_BUFFER_QUEUE swfdec_buffer_queue_get_type()
GType swfdec_buffer_queue_get_type  (void);

SwfdecBuffer *swfdec_buffer_new (gsize size);
SwfdecBuffer *swfdec_buffer_new0 (gsize size);
SwfdecBuffer *swfdec_buffer_new_full (unsigned char *data, gsize size, SwfdecBufferFreeFunc free_func, gpointer priv);
SwfdecBuffer *swfdec_buffer_new_subbuffer (SwfdecBuffer * buffer, gsize offset,
    gsize length);
SwfdecBuffer *swfdec_buffer_new_from_file (const char *filename, GError **error);
SwfdecBuffer *swfdec_buffer_new_for_data (unsigned char *data, gsize size);
#define swfdec_buffer_new_static(data, size) \
    swfdec_buffer_new_full ((unsigned char *) data, size, NULL, NULL)
SwfdecBuffer *swfdec_buffer_ref (SwfdecBuffer * buffer);
SwfdecBuffer *swfdec_buffer_get_super (SwfdecBuffer *buffer);
void swfdec_buffer_unref (SwfdecBuffer * buffer);

SwfdecBufferQueue *swfdec_buffer_queue_new (void);
void swfdec_buffer_queue_flush (SwfdecBufferQueue *queue, gsize n_bytes);
void swfdec_buffer_queue_clear (SwfdecBufferQueue *queue);
gsize swfdec_buffer_queue_get_depth (SwfdecBufferQueue * queue);
gsize swfdec_buffer_queue_get_offset (SwfdecBufferQueue * queue);
void swfdec_buffer_queue_push (SwfdecBufferQueue * queue,
    SwfdecBuffer * buffer);
SwfdecBuffer *swfdec_buffer_queue_pull (SwfdecBufferQueue * queue, gsize length);
SwfdecBuffer *swfdec_buffer_queue_pull_buffer (SwfdecBufferQueue * queue);
SwfdecBuffer *swfdec_buffer_queue_peek (SwfdecBufferQueue * queue, gsize length);
SwfdecBuffer *swfdec_buffer_queue_peek_buffer (SwfdecBufferQueue * queue);
SwfdecBufferQueue *swfdec_buffer_queue_ref (SwfdecBufferQueue * queue);
void swfdec_buffer_queue_unref (SwfdecBufferQueue * queue);
#endif

