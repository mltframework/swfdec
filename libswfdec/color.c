
#include <math.h>

#include "swfdec_internal.h"

unsigned int
swfdec_color_apply_transform (unsigned int in, SwfdecColorTransform * trans)
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
