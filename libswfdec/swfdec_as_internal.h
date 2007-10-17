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

#include <libswfdec/swfdec_as_object.h>
#include <libswfdec/swfdec_as_types.h>
#include <libswfdec/swfdec_security.h>

G_BEGIN_DECLS

/* This header contains all the non-exported symbols that can't go into 
 * exported headers 
 */
#define SWFDEC_AS_NATIVE(x, y, func) SWFDEC_AS_CONSTRUCTOR (x, y, func, NULL)
#define SWFDEC_AS_CONSTRUCTOR(x, y, func, type) void func (SwfdecAsContext *cx, \
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret);


void		swfdec_as_function_set_constructor (SwfdecAsFunction *	fun);
void		swfdec_as_function_set_security	(SwfdecAsFunction *	fun,
						 SwfdecSecurity *	sec);
void		swfdec_as_function_init_context (SwfdecAsContext *	context,
						 guint			version);

/* swfdec_as_context.c */
void		swfdec_as_context_run_init_script (SwfdecAsContext *	context,
						 const guint8 *		data,
						 gsize			length,
						 guint			version);

/* swfdec_as_object.c */
typedef SwfdecAsVariableForeach SwfdecAsVariableForeachRemove;
typedef const char *(* SwfdecAsVariableForeachRename) (SwfdecAsObject *object, 
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data);

void		swfdec_as_object_collect	(SwfdecAsObject *     	object);
guint		swfdec_as_object_foreach_remove	(SwfdecAsObject *       object,
						 SwfdecAsVariableForeach func,
						 gpointer		data);
void		swfdec_as_object_foreach_rename	(SwfdecAsObject *       object,
						 SwfdecAsVariableForeachRename func,
						 gpointer		data);

void		swfdec_as_object_init_context	(SwfdecAsContext *	context,
					      	 guint			version);
void		swfdec_as_object_decode		(SwfdecAsObject *	obj,
						 const char *		str);
SwfdecAsObject * swfdec_as_object_prototype_for_version (SwfdecAsObject *object,
						 guint			version,
						 gboolean		check7);
void		swfdec_as_object_run_with_security 
						(SwfdecAsObject *	object,
						 SwfdecScript *		script,
						 SwfdecSecurity *	sec);


G_END_DECLS
#endif
