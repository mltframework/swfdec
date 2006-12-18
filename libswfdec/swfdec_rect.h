
#ifndef __SWFDEC_RECT_H__
#define __SWFDEC_RECT_H__

#include <libswfdec/swfdec_types.h>

struct _SwfdecRect
{
  double x0;
  double y0;
  double x1;
  double y1;
};

void swfdec_rect_init_empty (SwfdecRect *rect);

gboolean swfdec_rect_intersect (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b);
void swfdec_rect_round (SwfdecRect *dest, SwfdecRect *src);
void swfdec_rect_union (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b);
void swfdec_rect_subtract (SwfdecRect *dest, const SwfdecRect *a, const SwfdecRect *b);
void swfdec_rect_scale (SwfdecRect *dest, const SwfdecRect *src, double factor);
gboolean swfdec_rect_is_empty (const SwfdecRect * a);
/* FIXME: rename to _contains_point and _contains instead of _inside? */
gboolean swfdec_rect_contains (const SwfdecRect *rect, double x, double y);
gboolean swfdec_rect_inside (const SwfdecRect *outer, const SwfdecRect *inner);
void swfdec_rect_transform (SwfdecRect *dest, const SwfdecRect *src, const cairo_matrix_t *matrix);

#endif
