
#ifndef __SWFDEC_BUFFER_H__
#define __SWFDEC_BUFFER_H__

#include <swfdec_types.h>


struct _SwfdecBuffer {
  unsigned char *data;
  int length;

  int ref_count;

  SwfdecBuffer *parent;

  void (*free) (SwfdecBuffer *, void *);
  void *priv;
};

SwfdecBuffer * swfdec_buffer_new (void);
SwfdecBuffer * swfdec_buffer_new_and_alloc (int size);
SwfdecBuffer * swfdec_buffer_new_with_data (void *data, int size);
SwfdecBuffer * swfdec_buffer_new_subbuffer (SwfdecBuffer *buffer, int offset,
    int length);
void swfdec_buffer_ref (SwfdecBuffer *buffer);
void swfdec_buffer_unref (SwfdecBuffer *buffer);


#endif

