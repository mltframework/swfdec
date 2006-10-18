
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "color.h"
#include "swfdec_debug.h"

swf_color 
swfdec_color_apply_morph (swf_color start, swf_color end, unsigned int ratio)
{
  unsigned int r, g, b, a;
  unsigned int start_ratio, end_ratio;

  g_assert (ratio < 65536);
  if (ratio == 0)
    return start;
  if (ratio == 65535)
    return end;
  start_ratio = 65535 - ratio;
  end_ratio = ratio;
  r = (SWF_COLOR_R (start) * start_ratio + SWF_COLOR_R (end) * end_ratio) / 65535;
  g = (SWF_COLOR_G (start) * start_ratio + SWF_COLOR_G (end) * end_ratio) / 65535;
  b = (SWF_COLOR_B (start) * start_ratio + SWF_COLOR_B (end) * end_ratio) / 65535;
  a = (SWF_COLOR_A (start) * start_ratio + SWF_COLOR_A (end) * end_ratio) / 65535;

  return SWF_COLOR_COMBINE (r, g, b, a);
}

void 
swfdec_color_set_source (cairo_t *cr, swf_color color)
{
  cairo_set_source_rgba (cr, 
      SWF_COLOR_R (color) / 255.0, SWF_COLOR_G (color) / 255.0,
      SWF_COLOR_B (color) / 255.0, SWF_COLOR_A (color) / 255.0);
}

unsigned int
swfdec_color_apply_transform (unsigned int in, const SwfdecColorTransform * trans)
{
  int r, g, b, a;

  r = SWF_COLOR_R (in);
  g = SWF_COLOR_G (in);
  b = SWF_COLOR_B (in);
  a = SWF_COLOR_A (in);

  SWFDEC_LOG ("in rgba %d,%d,%d,%d", r, g, b, a);

  r = rint ((r * trans->mult[0] + trans->add[0]));
  g = rint ((g * trans->mult[1] + trans->add[1]));
  b = rint ((b * trans->mult[2] + trans->add[2]));
  a = rint ((a * trans->mult[3] + trans->add[3]));

  r = CLAMP (r, 0, 255);
  g = CLAMP (g, 0, 255);
  b = CLAMP (b, 0, 255);
  a = CLAMP (a, 0, 255);

  SWFDEC_LOG ("out rgba %d,%d,%d,%d", r, g, b, a);

  return SWF_COLOR_COMBINE (r, g, b, a);
}

/**
 * swfdec_color_transform_init_identity:
 * @trans: a #SwfdecColorTransform
 *
 * Initializes the color transform so it doesn't transform any colors.
 **/
void
swfdec_color_transform_init_identity (SwfdecColorTransform * trans)
{
  g_return_if_fail (trans != NULL);
  
  trans->mult[0] = 1.0;
  trans->mult[1] = 1.0;
  trans->mult[2] = 1.0;
  trans->mult[3] = 1.0;
  trans->add[0] = 0.0;
  trans->add[1] = 0.0;
  trans->add[2] = 0.0;
  trans->add[3] = 0.0;
}

/**
 * swfdec_color_transform_init_color:
 * @trans: a #SwfdecColorTransform
 * @color: a #SwfdecColor to transform to
 *
 * Initializes this color transform so it results in exactly @color no matter 
 * the input.
 **/
void
swfdec_color_transform_init_color (SwfdecColorTransform *trans, swf_color color)
{
  trans->mult[0] = 0.0;
  trans->mult[1] = 0.0;
  trans->mult[2] = 0.0;
  trans->mult[3] = 0.0;
  trans->add[0] = SWF_COLOR_R (color);
  trans->add[1] = SWF_COLOR_G (color);
  trans->add[2] = SWF_COLOR_B (color);
  trans->add[3] = SWF_COLOR_A (color);
}

/**
 * swfdec_color_transform_chain:
 * @dest: #SwfdecColorTransform to take the result
 * @last: a #SwfdecColorTransform
 * @first: a #SwfdecColorTransform
 *
 * Computes a color transform that has the same effect as if the color 
 * transforms @first and @last would have been applied one after another.
 **/
void
swfdec_color_transform_chain (SwfdecColorTransform *dest,
    const SwfdecColorTransform *last, const SwfdecColorTransform *first)
{
  unsigned int i;

  g_return_if_fail (dest != NULL);
  g_return_if_fail (last != NULL);
  g_return_if_fail (first != NULL);
  
  for (i = 0; i < 4; i++) {
    dest->add[i] = last->mult[i] * first->add[i] + last->add[i];
    dest->mult[i] = last->mult[i] * first->mult[i];
  }
}

