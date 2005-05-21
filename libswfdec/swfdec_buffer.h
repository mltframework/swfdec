
#ifndef __SWFDEC_BUFFER_H__
#define __SWFDEC_BUFFER_H__

#include <swfdec_types.h>
#include <glib.h>


struct _SwfdecBuffer
{
  unsigned char *data;
  int length;

  int ref_count;

  SwfdecBuffer *parent;

  void (*free) (SwfdecBuffer *, void *);
  void *priv;
};

struct _SwfdecBufferQueue
{
  GList *buffers;
  int depth;
  int offset;
};

SwfdecBuffer *swfdec_buffer_new (void);
SwfdecBuffer *swfdec_buffer_new_and_alloc (int size);
SwfdecBuffer *swfdec_buffer_new_with_data (void *data, int size);
SwfdecBuffer *swfdec_buffer_new_subbuffer (SwfdecBuffer * buffer, int offset,
    int length);
SwfdecBuffer * swfdec_buffer_ref (SwfdecBuffer * buffer);
void swfdec_buffer_unref (SwfdecBuffer * buffer);

SwfdecBufferQueue *swfdec_buffer_queue_new (void);
void swfdec_buffer_queue_free (SwfdecBufferQueue * queue);
int swfdec_buffer_queue_get_depth (SwfdecBufferQueue * queue);
int swfdec_buffer_queue_get_offset (SwfdecBufferQueue * queue);
void swfdec_buffer_queue_push (SwfdecBufferQueue * queue,
    SwfdecBuffer * buffer);
SwfdecBuffer *swfdec_buffer_queue_pull (SwfdecBufferQueue * queue, int len);
SwfdecBuffer *swfdec_buffer_queue_peek (SwfdecBufferQueue * queue, int len);

#endif
