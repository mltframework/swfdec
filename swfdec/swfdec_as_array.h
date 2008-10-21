/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_AS_ARRAY_H_
#define _SWFDEC_AS_ARRAY_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS


SwfdecAsObject *swfdec_as_array_new		(SwfdecAsContext *	context);

#define		swfdec_as_array_push(array,value) \
  swfdec_as_array_append_with_flags ((array), 1, (value), 0)
#define		swfdec_as_array_push_with_flags(array,value,flags) \
  swfdec_as_array_append_with_flags ((array), 1, (value), (flags))
#define		swfdec_as_array_append(array,n,values) \
  swfdec_as_array_append_with_flags ((array), (n), (values), 0)
void		swfdec_as_array_append_with_flags (SwfdecAsObject *	array,
						 guint			n,
						 const SwfdecAsValue *	values,
						 SwfdecAsVariableFlag	flags);
void		swfdec_as_array_insert		(SwfdecAsObject *	array,
						 gint32			idx,
						 SwfdecAsValue *	value);
#define		swfdec_as_array_insert(array,idx,value) \
  swfdec_as_array_insert_with_flags ((array), (idx), (value), 0)
void		swfdec_as_array_insert_with_flags (SwfdecAsObject *	array,
						 gint32			idx,
						 const SwfdecAsValue *	value,
						 SwfdecAsVariableFlag	flags);
gint32		swfdec_as_array_get_length	(SwfdecAsObject *	array);
void		swfdec_as_array_set_length	(SwfdecAsObject *	array,
						 gint32			length);
void		swfdec_as_array_get_value	(SwfdecAsObject *	array,
						 gint32			idx,
						 SwfdecAsValue *	value);
void		swfdec_as_array_set_value	(SwfdecAsObject *	array,
						 gint32			idx,
						 SwfdecAsValue *	value);
void		swfdec_as_array_remove		(SwfdecAsObject *	array,
						 gint32			idx);


G_END_DECLS
#endif
