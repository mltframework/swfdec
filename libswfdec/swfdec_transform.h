
#ifndef __SWFDEC_TRANSFORM_H__
#define __SWFDEC_TRANSFORM_H__

#include "swfdec_types.h"
#include <glib.h>

struct _SwfdecTransform
{
  double trans[6];
};

gboolean swfdec_transform_is_translation (SwfdecTransform * a,
    SwfdecTransform * b);
void swfdec_transform_multiply (SwfdecTransform * dest, SwfdecTransform * a,
    SwfdecTransform * b);
void swfdec_transform_invert (SwfdecTransform * dest, SwfdecTransform * a);
void swfdec_transform_init_identity (SwfdecTransform * transform);
void swfdec_transform_translate (SwfdecTransform * transform, double x,
    double y);
double swfdec_transform_get_expansion (SwfdecTransform * transform);

#endif
