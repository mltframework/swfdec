
#include "swfdec_internal.h"
#include <math.h>


gboolean
swfdec_transform_is_translation (SwfdecTransform * a, SwfdecTransform * b)
{
  return (a->trans[0] == b->trans[0]) &&
      (a->trans[1] == b->trans[1]) &&
      (a->trans[2] == b->trans[2]) && (a->trans[3] == b->trans[3]);
}

void
swfdec_transform_multiply (SwfdecTransform * dest, SwfdecTransform * a,
    SwfdecTransform * b)
{
  g_return_if_fail (dest != a);
  g_return_if_fail (dest != b);

  dest->trans[0] = a->trans[0] * b->trans[0] + a->trans[1] * b->trans[2];
  dest->trans[1] = a->trans[0] * b->trans[1] + a->trans[1] * b->trans[3];
  dest->trans[2] = a->trans[2] * b->trans[0] + a->trans[3] * b->trans[2];
  dest->trans[3] = a->trans[2] * b->trans[1] + a->trans[3] * b->trans[3];
  dest->trans[4] = a->trans[4] * b->trans[0] + a->trans[5] * b->trans[2] +
      b->trans[4];
  dest->trans[5] = a->trans[4] * b->trans[1] + a->trans[5] * b->trans[3] +
      b->trans[5];
}

void
swfdec_transform_invert (SwfdecTransform * dest, SwfdecTransform * a)
{
  double inverse_det;

  g_return_if_fail (dest != a);

  inverse_det = 1.0 / (a->trans[0] * a->trans[3] - a->trans[1] * a->trans[2]);
  dest->trans[0] = a->trans[3] * inverse_det;
  dest->trans[1] = -a->trans[1] * inverse_det;
  dest->trans[2] = -a->trans[2] * inverse_det;
  dest->trans[3] = a->trans[0] * inverse_det;
  dest->trans[4] = -a->trans[4] * dest->trans[0] - a->trans[5] * dest->trans[2];
  dest->trans[5] = -a->trans[4] * dest->trans[1] - a->trans[5] * dest->trans[3];
}

void
swfdec_transform_init_identity (SwfdecTransform * transform)
{
  transform->trans[0] = 1.0;
  transform->trans[1] = 0.0;
  transform->trans[2] = 0.0;
  transform->trans[3] = 1.0;
  transform->trans[4] = 0.0;
  transform->trans[5] = 0.0;

}

void
swfdec_transform_translate (SwfdecTransform * transform, double x, double y)
{
  transform->trans[4] += x;
  transform->trans[5] += y;
}

double
swfdec_transform_get_expansion (SwfdecTransform * transform)
{
  return sqrt (fabs (transform->trans[0] * transform->trans[3] -
          transform->trans[1] * transform->trans[2]));
}
