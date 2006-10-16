
#ifndef __SWFDEC_BITS_H__
#define __SWFDEC_BITS_H__

#include <cairo.h>
#include "color.h"
#include "swfdec_buffer.h"

#define SWFDEC_COLOR_SCALE_FACTOR		(256.0)
#define SWFDEC_FIXED_SCALE_FACTOR		(65536.0)
#define SWFDEC_SCALE_FACTOR			(20.0)
#define SWFDEC_TEXT_SCALE_FACTOR		(1024.0)

typedef struct _SwfdecBits SwfdecBits;

struct _SwfdecBits
{
  SwfdecBuffer *buffer;
  unsigned char *ptr;
  unsigned int idx;
  unsigned char *end;
};

unsigned int swfdec_bits_left (SwfdecBits *b);
int swfdec_bits_getbit (SwfdecBits * b);
unsigned int swfdec_bits_getbits (SwfdecBits * b, unsigned int n);
unsigned int swfdec_bits_peekbits (SwfdecBits * b, unsigned int n);
int swfdec_bits_getsbits (SwfdecBits * b, unsigned int n);
unsigned int swfdec_bits_peek_u8 (SwfdecBits * b);
unsigned int swfdec_bits_get_u8 (SwfdecBits * b);
unsigned int swfdec_bits_get_u16 (SwfdecBits * b);
int swfdec_bits_get_s16 (SwfdecBits * b);
unsigned int swfdec_bits_get_be_u16 (SwfdecBits * b);
unsigned int swfdec_bits_get_u32 (SwfdecBits * b);
float swfdec_bits_get_float (SwfdecBits * b);
double swfdec_bits_get_double (SwfdecBits * b);
void swfdec_bits_syncbits (SwfdecBits * b);

void swfdec_bits_get_color_transform (SwfdecBits * bits,
    SwfdecColorTransform * ct);
void swfdec_bits_get_matrix (SwfdecBits * bits, cairo_matrix_t *matrix);
const char *swfdec_bits_skip_string (SwfdecBits * bits);
char *swfdec_bits_get_string (SwfdecBits * bits);
char *swfdec_bits_get_string_length (SwfdecBits * bits, unsigned int len);
unsigned int swfdec_bits_get_color (SwfdecBits * bits);
unsigned int swfdec_bits_get_rgba (SwfdecBits * bits);
SwfdecGradient *swfdec_bits_get_gradient (SwfdecBits * bits);
SwfdecGradient *swfdec_bits_get_gradient_rgba (SwfdecBits * bits);
SwfdecGradient *swfdec_bits_get_morph_gradient (SwfdecBits * bits);
void swfdec_bits_get_rect (SwfdecBits * bits, SwfdecRect *rect);
SwfdecBuffer *swfdec_bits_get_buffer (SwfdecBits *bits, int len);


#endif
