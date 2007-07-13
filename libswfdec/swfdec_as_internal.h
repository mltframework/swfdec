/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_AS_INTERNAL_H_
#define _SWFDEC_AS_INTERNAL_H_

#include <libswfdec/swfdec_as_types.h>

G_BEGIN_DECLS

/* This header contains all the non-exported symbols that can't go into 
 * exported headers 
 */

/* swfdec_as_array.c */
void	      	swfdec_as_array_init_context	(SwfdecAsContext *	context,
					      	 guint			version);

/* swfdec_as_function.c */
void		swfdec_as_function_init_context (SwfdecAsContext *	context,
						 guint			version);

/* swfdec_as_object.c */
void		swfdec_as_object_init_context	(SwfdecAsContext *	context,
					      	 guint			version);

G_END_DECLS
#endif
