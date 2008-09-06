/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "swfdec_path.h"
#include "swfdec_debug.h"

void
swfdec_path_init (cairo_path_t *path)
{
  path->status = CAIRO_STATUS_SUCCESS;
  path->data = NULL;
  path->num_data = 0;
}

void
swfdec_path_reset (cairo_path_t *path)
{
  path->status = CAIRO_STATUS_SUCCESS;
  g_free (path->data);
  path->data = NULL;
  path->num_data = 0;
}

void
swfdec_path_copy (cairo_path_t *dest, const cairo_path_t *source)
{
  dest->status = source->status;
  swfdec_path_ensure_size (dest, source->num_data);
  memcpy (dest->data, source->data, sizeof (cairo_path_data_t) * source->num_data);
  dest->num_data = source->num_data;
}

void
swfdec_path_ensure_size (cairo_path_t *path, int size)
{
#define SWFDEC_PATH_STEPS 32
  /* round up to next multiple of SWFDEC_PATH_STEPS */
  int current_size = path->num_data - path->num_data % SWFDEC_PATH_STEPS;
  if (path->num_data % SWFDEC_PATH_STEPS)
    current_size += SWFDEC_PATH_STEPS;

  if (size % SWFDEC_PATH_STEPS)
    size += SWFDEC_PATH_STEPS - size % SWFDEC_PATH_STEPS;
  g_assert (current_size % SWFDEC_PATH_STEPS == 0);
  g_assert (size % SWFDEC_PATH_STEPS == 0);
  while (size <= current_size)
    return;
  SWFDEC_LOG ("extending size of %p from %u to %u", path, current_size, size);
  path->data = g_renew (cairo_path_data_t, path->data, size);
}

void
swfdec_path_move_to (cairo_path_t *path, double x, double y)
{
  cairo_path_data_t *cur;

  swfdec_path_require_size (path, 2);
  cur = &path->data[path->num_data++];
  cur->header.type = CAIRO_PATH_MOVE_TO;
  cur->header.length = 2;
  cur = &path->data[path->num_data++];
  cur->point.x = x;
  cur->point.y = y;
}

void
swfdec_path_line_to (cairo_path_t *path, double x, double y)
{
  cairo_path_data_t *cur;

  swfdec_path_require_size (path, 2);
  cur = &path->data[path->num_data++];
  cur->header.type = CAIRO_PATH_LINE_TO;
  cur->header.length = 2;
  cur = &path->data[path->num_data++];
  cur->point.x = x;
  cur->point.y = y;
}

void
swfdec_path_curve_to (cairo_path_t *path, double start_x, double start_y,
    double control_x, double control_y, double end_x, double end_y)
{
  cairo_path_data_t *cur;

  swfdec_path_require_size (path, 4);
  cur = &path->data[path->num_data++];
  cur->header.type = CAIRO_PATH_CURVE_TO;
  cur->header.length = 4;
#define WEIGHT (2.0/3.0)
  cur = &path->data[path->num_data++];
  cur->point.x = control_x * WEIGHT + (1-WEIGHT) * start_x;
  cur->point.y = control_y * WEIGHT + (1-WEIGHT) * start_y;
  cur = &path->data[path->num_data++];
  cur->point.x = control_x * WEIGHT + (1-WEIGHT) * end_x;
  cur->point.y = control_y * WEIGHT + (1-WEIGHT) * end_y;
  cur = &path->data[path->num_data++];
  cur->point.x = end_x;
  cur->point.y = end_y;
}

void
swfdec_path_append (cairo_path_t *path, const cairo_path_t *append)
{
  swfdec_path_require_size (path, append->num_data);
  memcpy (&path->data[path->num_data], append->data, sizeof (cairo_path_data_t) * append->num_data);
  path->num_data += append->num_data;
}

void
swfdec_path_append_reverse (cairo_path_t *path, const cairo_path_t *append,
    double x, double y)
{
  cairo_path_data_t *out, *in;
  int i;

  swfdec_path_require_size (path, append->num_data);
  path->num_data += append->num_data;
  out = &path->data[path->num_data - 1];
  in = append->data;
  for (i = 0; i < append->num_data; i++) {
    switch (in[i].header.type) {
      case CAIRO_PATH_LINE_TO:
	out[-i].point.x = x;
	out[-i].point.y = y;
	out[-i - 1].header = in[i].header;
	i++;
	break;
      case CAIRO_PATH_CURVE_TO:
	out[-i].point.x = x;
	out[-i].point.y = y;
	out[-i - 3].header = in[i].header;
	out[-i - 1].point = in[i + 1].point;
	out[-i - 2].point = in[i + 2].point;
	i += 3;
	break;
      case CAIRO_PATH_CLOSE_PATH:
      case CAIRO_PATH_MOVE_TO:
	/* these two don't exist in our code */
      default:
	g_assert_not_reached ();
    }
    x = in[i].point.x;
    y = in[i].point.y;
  }
}

void
swfdec_path_get_extents (const cairo_path_t *path, SwfdecRect *extents)
{
  cairo_path_data_t *data = path->data;
  int i;
  double x = 0, y = 0;
  gboolean need_current = TRUE;
  gboolean start = TRUE;
#define ADD_POINT(extents, x, y) G_STMT_START { \
  if (x < extents->x0) \
    extents->x0 = x; \
  else if (x > extents->x1) \
    extents->x1 = x; \
  if (y < extents->y0) \
    extents->y0 = y; \
  else if (y > extents->y1) \
    extents->y1 = y; \
} G_STMT_END
  for (i = 0; i < path->num_data; i++) {
    switch (data[i].header.type) {
      case CAIRO_PATH_CURVE_TO:
	if (need_current) {
	  if (start) {
	    start = FALSE;
	    extents->x0 = x;
	    extents->x1 = x;
	    extents->y0 = y;
	    extents->y1 = y;
	  } else {
	    ADD_POINT (extents, x, y);
	  }
	  need_current = FALSE;
	}
	ADD_POINT (extents, data[i+1].point.x, data[i+1].point.y);
	ADD_POINT (extents, data[i+2].point.x, data[i+2].point.y);
	ADD_POINT (extents, data[i+3].point.x, data[i+3].point.y);
	i += 3;
	break;
      case CAIRO_PATH_LINE_TO:
        if (need_current) {
	  if (start) {
	    start = FALSE;
	    extents->x0 = x;
	    extents->x1 = x;
	    extents->y0 = y;
	    extents->y1 = y;
	  } else {
	    ADD_POINT (extents, x, y);
	  }
          need_current = FALSE;
        }
        ADD_POINT (extents, data[i+1].point.x, data[i+1].point.y);
        i++;
        break;
      case CAIRO_PATH_CLOSE_PATH:
        x = 0;
        y = 0;
	need_current = TRUE;
        break;
      case CAIRO_PATH_MOVE_TO:
        x = data[i+1].point.x;
        y = data[i+1].point.y;
	need_current = TRUE;
        i++;
        break;
      default:
        g_assert_not_reached ();
    }
  }
#undef ADD_POINT
}

void
swfdec_path_merge (cairo_path_t *dest, const cairo_path_t *start, 
    const cairo_path_t *end, double ratio)
{
  int i;
  cairo_path_data_t *ddata, *sdata, *edata;
  double inv = 1.0 - ratio;

  g_assert (start->num_data == end->num_data);

  swfdec_path_reset (dest);
  swfdec_path_ensure_size (dest, start->num_data);
  dest->num_data = start->num_data;
  ddata = dest->data;
  sdata = start->data;
  edata = end->data;
  for (i = 0; i < dest->num_data; i++) {
    g_assert (sdata[i].header.type == edata[i].header.type);
    ddata[i] = sdata[i];
    switch (sdata[i].header.type) {
      case CAIRO_PATH_CURVE_TO:
	ddata[i+1].point.x = sdata[i+1].point.x * inv + edata[i+1].point.x * ratio;
	ddata[i+1].point.y = sdata[i+1].point.y * inv + edata[i+1].point.y * ratio;
	ddata[i+2].point.x = sdata[i+2].point.x * inv + edata[i+2].point.x * ratio;
	ddata[i+2].point.y = sdata[i+2].point.y * inv + edata[i+2].point.y * ratio;
	i += 2;
      case CAIRO_PATH_MOVE_TO:
      case CAIRO_PATH_LINE_TO:
	ddata[i+1].point.x = sdata[i+1].point.x * inv + edata[i+1].point.x * ratio;
	ddata[i+1].point.y = sdata[i+1].point.y * inv + edata[i+1].point.y * ratio;
	i++;
      case CAIRO_PATH_CLOSE_PATH:
	break;
      default:
	g_assert_not_reached ();
    }
  }
}

