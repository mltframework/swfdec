
#include "swfdec_internal.h"

#include <string.h>

void 
swfdec_rect_init_empty (SwfdecRect *rect)
{
  g_return_if_fail (rect != NULL);

  rect->x0 = 0;
  rect->y0 = 0;
  rect->x1 = 0;
  rect->y1 = 0;
}

void 
swfdec_rect_init (SwfdecRect *rect, double x, double y, 
    double width, double height)
{
  g_return_if_fail (rect != NULL);
  g_return_if_fail (width >= 0);
  g_return_if_fail (height >= 0);

  rect->x0 = x;
  rect->y0 = y;
  rect->x1 = x + width;
  rect->y1 = y + height;
}

void
swfdec_rect_init_whole (SwfdecRect *rect)
{
  g_return_if_fail (rect != NULL);

  rect->x0 = rect->y0 = -G_MAXDOUBLE;
  rect->x1 = rect->y1 = G_MAXDOUBLE;
}

gboolean
swfdec_rect_intersect (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b)
{
  SwfdecRect tmp;

  g_return_val_if_fail (a != NULL, FALSE);
  g_return_val_if_fail (b != NULL, FALSE);
  if (dest == NULL)
    dest = &tmp;

  dest->x0 = MAX (a->x0, b->x0);
  dest->y0 = MAX (a->y0, b->y0);
  dest->x1 = MIN (a->x1, b->x1);
  dest->y1 = MIN (a->y1, b->y1);

  return !swfdec_rect_is_empty (dest);
}

/**
 * swfdec_rect_union:
 * @dest: destination rectangle
 * @a: first source rectangle, may be emtpy
 * @b: second source rectangle, may be empty
 *
 * Stores the union of @a and @b into @dest. The union is the smallest 
 * rectangle that includes both source rectangles. @a, @b and @dest may point 
 * to the same rectangle.
 **/
void
swfdec_rect_union (SwfdecRect * dest, const SwfdecRect * a, const SwfdecRect * b)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (a != NULL);
  g_return_if_fail (b != NULL);

  if (swfdec_rect_is_empty (a)) {
    *dest = *b;
  } else if (swfdec_rect_is_empty (b)) {
    *dest = *a;
  } else {
    dest->x0 = MIN (a->x0, b->x0);
    dest->y0 = MIN (a->y0, b->y0);
    dest->x1 = MAX (a->x1, b->x1);
    dest->y1 = MAX (a->y1, b->y1);
  }
}

void
swfdec_rect_copy (SwfdecRect * dest, const SwfdecRect * a)
{
  g_return_if_fail (dest != NULL);
  g_return_if_fail (a != NULL);

  *dest = *a;
}

gboolean
swfdec_rect_is_empty (const SwfdecRect * a)
{
  return (a->x1 <= a->x0) || (a->y1 <= a->y0);
}

gboolean 
swfdec_rect_contains (const SwfdecRect *rect, double x, double y)
{
  return x >= rect->x0 &&
    x <= rect->x1 &&
    y >= rect->y0 &&
    y <= rect->y1;
}

void
swfdec_rect_transform (SwfdecRect *dest, const SwfdecRect *src, const cairo_matrix_t *matrix)
{
  SwfdecRect tmp;

  g_return_if_fail (dest != NULL);
  g_return_if_fail (src != NULL);
  g_return_if_fail (matrix != NULL);

  tmp = *src;
  cairo_matrix_transform_point (matrix, &tmp.x0, &tmp.y0);
  cairo_matrix_transform_point (matrix, &tmp.x1, &tmp.y1);

  dest->x0 = MIN (tmp.x0, tmp.x1);
  dest->y0 = MIN (tmp.y0, tmp.y1);
  dest->x1 = MAX (tmp.x0, tmp.x1);
  dest->y1 = MAX (tmp.y0, tmp.y1);
}

void
swfdec_rect_transform_inverse (SwfdecRect *dest, const SwfdecRect *src, const cairo_matrix_t *matrix)
{
  cairo_matrix_t tmp = *matrix;
  if (cairo_matrix_invert (&tmp)) {
    /* FIXME: do we need to handle this sanely? */
    g_assert_not_reached ();
  }
  swfdec_rect_transform (dest, src, &tmp);
}

void 
swfdec_matrix_transform_point_inverse (cairo_matrix_t *matrix, double *x, double *y)
{
  cairo_matrix_t tmp = *matrix;
  if (cairo_matrix_invert (&tmp)) {
    /* FIXME: do we need to handle this sanely? */
    g_assert_not_reached ();
  }
  cairo_matrix_transform_point (&tmp, x, y);
}
