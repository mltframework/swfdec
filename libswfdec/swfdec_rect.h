
#ifndef __SWFDEC_RECT_H__
#define __SWFDEC_RECT_H__

#include <swfdec_types.h>

struct _SwfdecRect
{
  int x0;
  int y0;
  int x1;
  int y1;
};

void swfdec_rect_union_to_masked (SwfdecRect * rect, SwfdecRect * a,
    SwfdecRect * mask);
void swfdec_rect_intersect (SwfdecRect * dest, SwfdecRect * a, SwfdecRect * b);
void swfdec_rect_union (SwfdecRect * dest, SwfdecRect * a, SwfdecRect * b);
void swfdec_rect_copy (SwfdecRect * dest, SwfdecRect * a);
gboolean swfdec_rect_is_empty (SwfdecRect * a);

#endif
