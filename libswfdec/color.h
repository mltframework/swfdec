
#ifndef __SWFDEC_COLOR_H__
#define __SWFDEC_COLOR_H__

#include <libswfdec/swfdec_types.h>

typedef unsigned int swf_color;

struct _SwfdecColorTransform
{
  double mult[4];
  double add[4];
};

typedef struct swfdec_gradient_struct SwfdecGradient;
typedef struct swfdec_gradient_entry_struct SwfdecGradientEntry;

struct swfdec_gradient_entry_struct
{
  int ratio;
  swf_color color;
};

struct swfdec_gradient_struct
{
  unsigned int n_gradients;
  SwfdecGradientEntry array[1];
};

#define SWF_COLOR_COMBINE(r,g,b,a)	(((r)<<24) | ((g)<<16) | ((b)<<8) | (a))
#define SWF_COLOR_R(x)		(((x)>>24)&0xff)
#define SWF_COLOR_G(x)		(((x)>>16)&0xff)
#define SWF_COLOR_B(x)		(((x)>>8)&0xff)
#define SWF_COLOR_A(x)		((x)&0xff)

#define RGB565_COMBINE(r,g,b) (((r)&0xf8)<<8)|(((g)&0xfc)<<3)|(((b)&0xf8)>>3)
#define RGB565_R(color) (((color)&0xf800)>>8)
#define RGB565_G(color) (((color)&0x07e0)>>3)
#define RGB565_B(color) (((color)&0x001f)<<3)


void swfdec_color_set_source (cairo_t *cr, swf_color color);
void swfdec_color_transform_init_identity (SwfdecColorTransform * trans);
void swfdec_color_transform_init_color (SwfdecColorTransform *trans, swf_color color);
void swfdec_color_transform_chain (SwfdecColorTransform *dest,
    const SwfdecColorTransform *last, const SwfdecColorTransform *first);
unsigned int swfdec_color_apply_transform (unsigned int in,
    const SwfdecColorTransform * trans);

#endif
