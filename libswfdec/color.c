
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

