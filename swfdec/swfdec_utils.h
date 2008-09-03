/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_UTILS_H_
#define _SWFDEC_UTILS_H_

#include <cairo.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

gboolean	swfdec_str_case_equal		(gconstpointer		v1,
						 gconstpointer		v2);
guint		swfdec_str_case_hash		(gconstpointer		v);

int		swfdec_strcmp			(guint			version,
						 const char *		s1,
						 const char *		s2);
int		swfdec_strncmp			(guint			version,
						 const char *		s1,
						 const char *		s2,
						 guint			n);

gboolean	swfdec_matrix_from_as_object	(cairo_matrix_t *	matrix,
						 SwfdecAsObject *	object);


G_END_DECLS
#endif
