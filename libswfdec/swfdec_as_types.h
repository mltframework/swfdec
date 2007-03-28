/* SwfdecAs
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

#ifndef _SWFDEC_AS_TYPES_H_
#define _SWFDEC_AS_TYPES_H_

#include <glib.h>

G_BEGIN_DECLS

/* fundamental types */
#define SWFDEC_TYPE_AS_UNDEFINED (0)
#define SWFDEC_TYPE_AS_BOOLEAN	(1)
/* #define SWFDEC_AS_INT	(2)  not defined yet, might be useful as an optimization */
#define SWFDEC_TYPE_AS_NUMBER	(3)
#define SWFDEC_TYPE_AS_STRING	(4)
#define SWFDEC_TYPE_AS_NULL	(5)
#define SWFDEC_TYPE_AS_ASOBJECT	(6)

typedef guint8 SwfdecAsType;
typedef struct _SwfdecAsContext SwfdecAsContext;
typedef struct _SwfdecAsObject SwfdecAsObject;
typedef struct _SwfdecAsValue SwfdecAsValue;

struct _SwfdecAsValue {
  SwfdecAsType		type;
  union {
    gboolean		boolean;
    double		number;
    const char *	string;
    SwfdecAsObject *	object;
  } value;
};

#define SWFDEC_AS_IS_VALUE(val) ((val)->type <= SWFDEC_TYPE_AS_OBJECT)

#define SWFDEC_AS_VALUE_IS_UNDEFINED(val) ((val)->type == SWFDEC_TYPE_AS_UNDEFINED)
#define SWFDEC_AS_VALUE_SET_UNDEFINED(val) (val)->type = SWFDEC_TYPE_AS_UNDEFINED

#define SWFDEC_AS_VALUE_IS_BOOLEAN(val) ((val)->type == SWFDEC_TYPE_AS_BOOLEAN)
#define SWFDEC_AS_VALUE_GET_BOOLEAN(val) (g_assert ((val)->type == SWFDEC_TYPE_AS_BOOLEAN), (val)->value.boolean)
#define SWFDEC_AS_VALUE_SET_BOOLEAN(val,b) G_STMT_START { \
  (val)->type = SWFDEC_TYPE_AS_BOOLEAN; \
  (val)->value.boolean = b; \
} G_STMT_END

#define SWFDEC_AS_VALUE_IS_NUMBER(val) ((val)->type == SWFDEC_TYPE_AS_NUMBER)
#define SWFDEC_AS_VALUE_GET_NUMBER(val) (g_assert ((val)->type == SWFDEC_TYPE_AS_NUMBER), (val)->value.number)
#define SWFDEC_AS_VALUE_SET_NUMBER(val,d) G_STMT_START { \
  (val)->type = SWFDEC_TYPE_AS_NUMBER; \
  (val)->value.number = d; \
} G_STMT_END

#define SWFDEC_AS_VALUE_IS_STRING(val) ((val)->type == SWFDEC_TYPE_AS_STRING)
#define SWFDEC_AS_VALUE_GET_STRING(val) (g_assert ((val)->type == SWFDEC_TYPE_AS_STRING), (val)->value.string)
#define SWFDEC_AS_VALUE_SET_STRING(val,s) G_STMT_START { \
  (val)->type = SWFDEC_TYPE_AS_STRING; \
  (val)->value.string = s; \
} G_STMT_END

#define SWFDEC_AS_VALUE_IS_NULL(val) ((val)->type == SWFDEC_TYPE_AS_NULL)
#define SWFDEC_AS_VALUE_SET_NULL(val) (val)->type = SWFDEC_TYPE_AS_NULL

#define SWFDEC_AS_VALUE_IS_OBJECT(val) ((val)->type == SWFDEC_TYPE_AS_ASOBJECT)
#define SWFDEC_AS_VALUE_GET_OBJECT(val) (g_assert ((val)->type == SWFDEC_TYPE_AS_ASOBJECT), (val)->value.object)
#define SWFDEC_AS_VALUE_SET_OBJECT(val,o) G_STMT_START { \
  (val)->type = SWFDEC_TYPE_AS_ASOBJECT; \
  (val)->value.object = o; \
} G_STMT_END


/* List of static strings that are required all the time */
extern const char *swfdec_as_strings[];
#define SWFDEC_AS_EMPTY_STRING (swfdec_as_strings[0] + 1)
#define SWFDEC_AS_PROTO_STRING (swfdec_as_strings[1] + 1)


const char *	swfdec_as_value_to_string	(SwfdecAsContext *	context,
						 const SwfdecAsValue *	value);


G_END_DECLS
#endif
