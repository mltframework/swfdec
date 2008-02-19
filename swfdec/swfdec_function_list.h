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

#ifndef __SWFDEC_FUNCTION_LIST_H__
#define __SWFDEC_FUNCTION_LIST_H__

#include <glib.h>

G_BEGIN_DECLS


typedef struct _SwfdecFunctionList SwfdecFunctionList;
struct _SwfdecFunctionList {
  GList *	list;
};

void		  	swfdec_function_list_clear	(SwfdecFunctionList *	list);

void			swfdec_function_list_add	(SwfdecFunctionList *	list,
							 GFunc			func,
							 gpointer		data,
							 GDestroyNotify 	destroy);
void			swfdec_function_list_remove	(SwfdecFunctionList *	list,
							 gpointer		data);

void			swfdec_function_list_execute	(SwfdecFunctionList *	list,
							 gpointer		data);
void			swfdec_function_list_execute_and_clear
							(SwfdecFunctionList *   list,
							 gpointer		data);

G_END_DECLS

#endif
