
#ifndef __SWFDEC_BITS_H__
#define __SWFDEC_BITS_H__

#include <cairo.h>
#include "color.h"
#include "swfdec_buffer.h"

typedef struct _SwfdecBits SwfdecBits;

struct _SwfdecBits
{
  SwfdecBuffer *buffer;
  unsigned char *ptr;
  int idx;
  unsigned char *end;
};

int swfdec_bits_needbits (SwfdecBits * b, int n_bytes);
int swfdec_bits_getbit (SwfdecBits * b);
unsigned int swfdec_bits_getbits (SwfdecBits * b, int n);
unsigned int swfdec_bits_peekbits (SwfdecBits * b, int n);
int swfdec_bits_getsbits (SwfdecBits * b, int n);
unsigned int swfdec_bits_peek_u8 (SwfdecBits * b);
unsigned int swfdec_bits_get_u8 (SwfdecBits * b);
unsigned int swfdec_bits_get_u16 (SwfdecBits * b);
int swfdec_bits_get_s16 (SwfdecBits * b);
unsigned int swfdec_bits_get_be_u16 (SwfdecBits * b);
unsigned int swfdec_bits_get_u32 (SwfdecBits * b);
void swfdec_bits_syncbits (SwfdecBits * b);

void swfdec_bits_get_color_transform (SwfdecBits * bits,
    SwfdecColorTransform * ct);
void swfdec_bits_get_matrix (SwfdecBits * bits, cairo_matrix_t *matrix);
char *swfdec_bits_get_string (SwfdecBits * bits);
unsigned int swfdec_bits_get_color (SwfdecBits * bits);
unsigned int swfdec_bits_get_rgba (SwfdecBits * bits);
SwfdecGradient *swfdec_bits_get_gradient (SwfdecBits * bits);
SwfdecGradient *swfdec_bits_get_gradient_rgba (SwfdecBits * bits);
SwfdecGradient *swfdec_bits_get_morph_gradient (SwfdecBits * bits);
void swfdec_bits_get_rect (SwfdecBits * bits, SwfdecRect *rect, double scale);


#endif
