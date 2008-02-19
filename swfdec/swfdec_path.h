/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_PATH_H_
#define _SWFDEC_PATH_H_

#include <cairo.h>
#include <swfdec/swfdec_rect.h>

G_BEGIN_DECLS

/* FIXME: Shouldn't this code be in cairo somewhere? */

void		swfdec_path_init		(cairo_path_t *		path);
void		swfdec_path_reset		(cairo_path_t *		path);

#define swfdec_path_require_size(path, steps) \
  swfdec_path_ensure_size ((path), (path)->num_data + steps)
void		swfdec_path_ensure_size		(cairo_path_t *		path,
						 int			size);
void		swfdec_path_move_to		(cairo_path_t *		path,
						 double			x,
						 double			y);
void		swfdec_path_line_to		(cairo_path_t *		path,
						 double			x,
						 double			y);
void		swfdec_path_curve_to		(cairo_path_t *		path,
						 double			start_x,
						 double			start_y,
						 double			control_x,
						 double			control_y,
						 double			end_x,
						 double			end_y);
void		swfdec_path_append		(cairo_path_t *		path,
						 const cairo_path_t *	append);
void		swfdec_path_append_reverse	(cairo_path_t *		path,
						 const cairo_path_t *	append,
						 double			x,
						 double			y);

void		swfdec_path_get_extents		(const cairo_path_t *	path,
						 SwfdecRect *		extents);

void		swfdec_path_merge		(cairo_path_t *		dest,
						 const cairo_path_t *	start, 
						 const cairo_path_t *	end,
						 double			ratio);


G_END_DECLS
#endif
