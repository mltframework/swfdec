
#include "swfdec_internal.h"

#include <string.h>

#include <liboil/liboil.h>

void
swfdec_rect_union_to_masked (SwfdecRect * rect, SwfdecRect * a,
    SwfdecRect * mask)
{
  if (swfdec_rect_is_empty (rect)) {
    swfdec_rect_intersect (rect, a, mask);
  } else {
    SwfdecRect tmp1, tmp2;

    swfdec_rect_copy (&tmp1, rect);
    swfdec_rect_intersect (&tmp2, a, mask);
    swfdec_rect_union (rect, &tmp1, &tmp2);
  }
}

void
swfdec_rect_intersect (SwfdecRect * dest, SwfdecRect * a, SwfdecRect * b)
{
  dest->x0 = MAX (a->x0, b->x0);
  dest->y0 = MAX (a->y0, b->y0);
  dest->x1 = MIN (a->x1, b->x1);
  dest->y1 = MIN (a->y1, b->y1);
}

void
swfdec_rect_union (SwfdecRect * dest, SwfdecRect * a, SwfdecRect * b)
{
  dest->x0 = MIN (a->x0, b->x0);
  dest->y0 = MIN (a->y0, b->y0);
  dest->x1 = MAX (a->x1, b->x1);
  dest->y1 = MAX (a->y1, b->y1);
}

void
swfdec_rect_copy (SwfdecRect * dest, SwfdecRect * a)
{
  memcpy (dest, a, sizeof (SwfdecRect));
}

gboolean
swfdec_rect_is_empty (SwfdecRect * a)
{
  return (a->x1 <= a->x0) || (a->y1 <= a->y0);
}
