/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_COLOR_MATRIX_FILTER_H_
#define _SWFDEC_COLOR_MATRIX_FILTER_H_

#include <swfdec/swfdec_filter.h>
#include <swfdec/swfdec_convolution_matrix.h>

G_BEGIN_DECLS


typedef struct _SwfdecColorMatrixFilter SwfdecColorMatrixFilter;
typedef struct _SwfdecColorMatrixFilterClass SwfdecColorMatrixFilterClass;

#define SWFDEC_TYPE_COLOR_MATRIX_FILTER                    (swfdec_color_matrix_filter_get_type())
#define SWFDEC_IS_COLOR_MATRIX_FILTER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_COLOR_MATRIX_FILTER))
#define SWFDEC_IS_COLOR_MATRIX_FILTER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_COLOR_MATRIX_FILTER))
#define SWFDEC_COLOR_MATRIX_FILTER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_COLOR_MATRIX_FILTER, SwfdecColorMatrixFilter))
#define SWFDEC_COLOR_MATRIX_FILTER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_COLOR_MATRIX_FILTER, SwfdecColorMatrixFilterClass))
#define SWFDEC_COLOR_MATRIX_FILTER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_COLOR_MATRIX_FILTER, SwfdecColorMatrixFilterClass))

struct _SwfdecColorMatrixFilter {
  SwfdecFilter		filter;

  double		matrix[20];	/* color matrix to apply */
};

struct _SwfdecColorMatrixFilterClass {
  SwfdecFilterClass	filter_class;
};

GType			swfdec_color_matrix_filter_get_type	(void);


G_END_DECLS
#endif
