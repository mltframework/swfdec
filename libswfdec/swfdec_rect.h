
#ifndef __SWFDEC_RECT_H__
#define __SWFDEC_RECT_H__

#include <swfdec_types.h>

struct _SwfdecRect
{
  double x0;
  double y0;
  double x1;
  double y1;
};

void swfdec_rect_init_empty (SwfdecRect *rect);
void swfdec_rect_init (SwfdecRect *rect, double x, double y, double width, double height);
gboolean swfdec_rect_intersect (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b);
void swfdec_rect_union (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b);
void swfdec_rect_copy (SwfdecRect * dest, const SwfdecRect * a);
gboolean swfdec_rect_is_empty (const SwfdecRect * a);
void swfdec_rect_apply_matrix (SwfdecRect *dest, const SwfdecRect *src, const cairo_matrix_t *matrix);

#endif
