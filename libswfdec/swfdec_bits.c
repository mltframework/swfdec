
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "swfdec_internal.h"
#include <swfdec_bits.h>


static int
swfdec_bits_left (SwfdecBits *b)
{
  if (b->ptr == NULL)
    return 0;
  return (b->end - b->ptr) * 8 - b->idx;
}

#define SWFDEC_BITS_CHECK(b,n) G_STMT_START { \
  if (swfdec_bits_left(b) < (n)) { \
    SWFDEC_ERROR("reading past end of buffer"); \
    g_assert_not_reached(); \
    return 0; \
  } \
}G_STMT_END
#define SWFDEC_BYTES_CHECK(b,n) G_STMT_START { \
  swfdec_bits_syncbits (b); \
  SWFDEC_BITS_CHECK (b, 8 * n); \
} G_STMT_END

int
swfdec_bits_getbit (SwfdecBits * b)
{
  int r;

  SWFDEC_BITS_CHECK (b, 1);

  r = ((*b->ptr) >> (7 - b->idx)) & 1;

  b->idx++;
  if (b->idx >= 8) {
    b->ptr++;
    b->idx = 0;
  }

  return r;
}

unsigned int
swfdec_bits_getbits (SwfdecBits * b, int n)
{
  unsigned long r = 0;
  int i;

  SWFDEC_BITS_CHECK (b, n);

  while (n > 0) {
    i = MIN (n, 8 - b->idx);
    r <<= i;
    r |= ((*b->ptr) >> (8 - i - b->idx)) & ((1 << i) - 1);
    n -= i;
    if (i == 8) {
      b->ptr++;
    } else {
      b->idx += i;
      if (b->idx >= 8) {
	b->ptr++;
	b->idx = 0;
      }
    }
  }
  return r;
}

unsigned int
swfdec_bits_peekbits (SwfdecBits * b, int n)
{
  SwfdecBits tmp = *b;

  return swfdec_bits_getbits (&tmp, n);
}

int
swfdec_bits_getsbits (SwfdecBits * b, int n)
{
  unsigned long r = 0;

  SWFDEC_BITS_CHECK (b, n);

  if (n == 0)
    return 0;
  r = -swfdec_bits_getbit (b);
  r = (r << (n - 1)) | swfdec_bits_getbits (b, n - 1);
  return r;
}

unsigned int
swfdec_bits_peek_u8 (SwfdecBits * b)
{
  SWFDEC_BYTES_CHECK (b, 1);

  return *b->ptr;
}

unsigned int
swfdec_bits_get_u8 (SwfdecBits * b)
{
  SWFDEC_BYTES_CHECK (b, 1);

  return *b->ptr++;
}

unsigned int
swfdec_bits_get_u16 (SwfdecBits * b)
{
  unsigned int r;

  SWFDEC_BYTES_CHECK (b, 2);

  r = b->ptr[0] | (b->ptr[1] << 8);
  b->ptr += 2;

  return r;
}

int
swfdec_bits_get_s16 (SwfdecBits * b)
{
  short r;

  SWFDEC_BYTES_CHECK (b, 2);

  r = b->ptr[0] | (b->ptr[1] << 8);
  b->ptr += 2;

  return r;
}

unsigned int
swfdec_bits_get_be_u16 (SwfdecBits * b)
{
  unsigned int r;

  SWFDEC_BYTES_CHECK (b, 2);

  r = (b->ptr[0] << 8) | b->ptr[1];
  b->ptr += 2;

  return r;
}

unsigned int
swfdec_bits_get_u32 (SwfdecBits * b)
{
  unsigned int r;

  SWFDEC_BYTES_CHECK (b, 4);

  r = b->ptr[0] | (b->ptr[1] << 8) | (b->ptr[2] << 16) | (b->ptr[3] << 24);
  b->ptr += 4;

  return r;
}

void
swfdec_bits_syncbits (SwfdecBits * b)
{
  if (b->idx) {
    b->ptr++;
    b->idx = 0;
  }
}

void
swfdec_bits_get_color_transform (SwfdecBits * bits, SwfdecColorTransform * ct)
{
  int has_add;
  int has_mult;
  int n_bits;

  swfdec_bits_syncbits (bits);
  has_add = swfdec_bits_getbit (bits);
  has_mult = swfdec_bits_getbit (bits);
  n_bits = swfdec_bits_getbits (bits, 4);
  if (has_mult) {
    ct->mult[0] = swfdec_bits_getsbits (bits, n_bits) * SWF_COLOR_SCALE_FACTOR;
    ct->mult[1] = swfdec_bits_getsbits (bits, n_bits) * SWF_COLOR_SCALE_FACTOR;
    ct->mult[2] = swfdec_bits_getsbits (bits, n_bits) * SWF_COLOR_SCALE_FACTOR;
    ct->mult[3] = swfdec_bits_getsbits (bits, n_bits) * SWF_COLOR_SCALE_FACTOR;
  } else {
    ct->mult[0] = 1.0;
    ct->mult[1] = 1.0;
    ct->mult[2] = 1.0;
    ct->mult[3] = 1.0;
  }
  if (has_add) {
    ct->add[0] = swfdec_bits_getsbits (bits, n_bits);
    ct->add[1] = swfdec_bits_getsbits (bits, n_bits);
    ct->add[2] = swfdec_bits_getsbits (bits, n_bits);
    ct->add[3] = swfdec_bits_getsbits (bits, n_bits);
  } else {
    ct->add[0] = 0;
    ct->add[1] = 0;
    ct->add[2] = 0;
    ct->add[3] = 0;
  }

  //printf("mult %g,%g,%g,%g\n",mult[0],mult[1],mult[2],mult[3]);
  //printf("add %g,%g,%g,%g\n",add[0],add[1],add[2],add[3]);
}

void
swfdec_bits_get_matrix (SwfdecBits * bits, cairo_matrix_t *matrix)
{
  int has_scale;
  int has_rotate;
  int n_translate_bits;
  int translate_x;
  int translate_y;

  swfdec_bits_syncbits (bits);

  has_scale = swfdec_bits_getbit (bits);
  if (has_scale) {
    int n_scale_bits = swfdec_bits_getbits (bits, 5);
    int scale_x = swfdec_bits_getsbits (bits, n_scale_bits);
    int scale_y = swfdec_bits_getsbits (bits, n_scale_bits);

    matrix->xx = scale_x * SWF_TRANS_SCALE_FACTOR;
    matrix->yy = scale_y * SWF_TRANS_SCALE_FACTOR;
  } else {
    SWFDEC_LOG ("no scalefactors given");
  }
  has_rotate = swfdec_bits_getbit (bits);
  if (has_rotate) {
    int n_rotate_bits = swfdec_bits_getbits (bits, 5);
    int rotate_skew0 = swfdec_bits_getsbits (bits, n_rotate_bits);
    int rotate_skew1 = swfdec_bits_getsbits (bits, n_rotate_bits);

    matrix->xy = rotate_skew0 * SWF_TRANS_SCALE_FACTOR;
    matrix->yx = rotate_skew1 * SWF_TRANS_SCALE_FACTOR;
  }
  n_translate_bits = swfdec_bits_getbits (bits, 5);
  translate_x = swfdec_bits_getsbits (bits, n_translate_bits);
  translate_y = swfdec_bits_getsbits (bits, n_translate_bits);

  matrix->x0 = translate_x;
  matrix->y0 = translate_y;
}

char *
swfdec_bits_get_string (SwfdecBits * bits)
{
  char *s = g_strdup ((char *)bits->ptr);

  bits->ptr += strlen ((char *)bits->ptr) + 1;

  return s;
}

unsigned int
swfdec_bits_get_color (SwfdecBits * bits)
{
  int r, g, b;

  r = swfdec_bits_get_u8 (bits);
  g = swfdec_bits_get_u8 (bits);
  b = swfdec_bits_get_u8 (bits);

  return SWF_COLOR_COMBINE (r, g, b, 0xff);
}

unsigned int
swfdec_bits_get_rgba (SwfdecBits * bits)
{
  int r, g, b, a;

  r = swfdec_bits_get_u8 (bits);
  g = swfdec_bits_get_u8 (bits);
  b = swfdec_bits_get_u8 (bits);
  a = swfdec_bits_get_u8 (bits);

  return SWF_COLOR_COMBINE (r, g, b, a);
}

SwfdecGradient *
swfdec_bits_get_gradient (SwfdecBits * bits)
{
  SwfdecGradient *grad;
  int n_gradients;
  int i;

  swfdec_bits_syncbits (bits);
  n_gradients = swfdec_bits_getbits (bits, 8);
  grad = g_malloc (sizeof (SwfdecGradient) +
      sizeof (SwfdecGradientEntry) * (n_gradients - 1));
  grad->n_gradients = n_gradients;
  for (i = 0; i < n_gradients; i++) {
    grad->array[i].ratio = swfdec_bits_getbits (bits, 8);
    grad->array[i].color = swfdec_bits_get_color (bits);
  }
  return grad;
}

SwfdecGradient *
swfdec_bits_get_gradient_rgba (SwfdecBits * bits)
{
  SwfdecGradient *grad;
  int n_gradients;
  int i;

  swfdec_bits_syncbits (bits);
  n_gradients = swfdec_bits_getbits (bits, 8);
  grad = g_malloc (sizeof (SwfdecGradient) +
      sizeof (SwfdecGradientEntry) * (n_gradients - 1));
  grad->n_gradients = n_gradients;
  for (i = 0; i < n_gradients; i++) {
    grad->array[i].ratio = swfdec_bits_getbits (bits, 8);
    grad->array[i].color = swfdec_bits_get_rgba (bits);
  }
  return grad;
}

SwfdecGradient *
swfdec_bits_get_morph_gradient (SwfdecBits * bits)
{
  SwfdecGradient *grad;
  int n_gradients;
  int i;
  int end_ratio;
  unsigned int end_color;

  swfdec_bits_syncbits (bits);
  n_gradients = swfdec_bits_getbits (bits, 8);
  grad = g_malloc (sizeof (SwfdecGradient) +
      sizeof (SwfdecGradientEntry) * (n_gradients - 1));
  grad->n_gradients = n_gradients;
  for (i = 0; i < n_gradients; i++) {
    grad->array[i].ratio = swfdec_bits_getbits (bits, 8);
    grad->array[i].color = swfdec_bits_get_rgba (bits);
    end_ratio = swfdec_bits_getbits (bits, 8);
    end_color = swfdec_bits_get_rgba (bits);
  }
  return grad;
}

void
swfdec_bits_get_rect (SwfdecBits * bits, SwfdecRect *rect)
{
  int nbits = swfdec_bits_getbits (bits, 5);

  rect->x0 = swfdec_bits_getsbits (bits, nbits);
  rect->x1 = swfdec_bits_getsbits (bits, nbits);
  rect->y0 = swfdec_bits_getsbits (bits, nbits);
  rect->y1 = swfdec_bits_getsbits (bits, nbits);
}
