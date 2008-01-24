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

#ifndef _SWFDEC_AS_STRING_H_
#define _SWFDEC_AS_STRING_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsString SwfdecAsString;
typedef struct _SwfdecAsStringClass SwfdecAsStringClass;

#define SWFDEC_TYPE_AS_STRING                    (swfdec_as_string_get_type())
#define SWFDEC_IS_AS_STRING(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_STRING))
#define SWFDEC_IS_AS_STRING_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_STRING))
#define SWFDEC_AS_STRING(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_STRING, SwfdecAsString))
#define SWFDEC_AS_STRING_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_STRING, SwfdecAsStringClass))
#define SWFDEC_AS_STRING_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_STRING, SwfdecAsStringClass))

struct _SwfdecAsString {
  SwfdecAsObject	object;

  const char *		string;		/* string represented by this string object */
};

struct _SwfdecAsStringClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_as_string_get_type	(void);

const char *	swfdec_as_str_sub		(SwfdecAsContext *	cx,
						 const char *		str,
						 guint			offset,
						 guint			len);

char *		swfdec_as_string_escape		(SwfdecAsContext *	context,
						 const char *		string);
char *		swfdec_as_string_unescape	(SwfdecAsContext *	context,
						 const char *		string);


G_END_DECLS
#endif
