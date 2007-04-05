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

#ifndef _SWFDEC_AS_OBJECT_H_
#define _SWFDEC_AS_OBJECT_H_

#include <glib-object.h>
#include <libswfdec/swfdec_as_types.h>
#include <libswfdec/swfdec_types.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_AS_VARIABLE_PERMANENT = (1 << 0),
  SWFDEC_AS_VARIABLE_DONT_ENUM = (1 << 1),
  SWFDEC_AS_VARIABLE_READONLY = (1 << 2)
} SwfdecAsVariableFlag;

typedef struct _SwfdecAsObjectClass SwfdecAsObjectClass;

#define SWFDEC_TYPE_AS_OBJECT                    (swfdec_as_object_get_type())
#define SWFDEC_IS_AS_OBJECT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_OBJECT))
#define SWFDEC_IS_AS_OBJECT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_OBJECT))
#define SWFDEC_AS_OBJECT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_OBJECT, SwfdecAsObject))
#define SWFDEC_AS_OBJECT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_OBJECT, SwfdecAsObjectClass))
#define SWFDEC_AS_OBJECT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_OBJECT, SwfdecAsObjectClass))

#define SWFDEC_AS_OBJECT_HAS_CONTEXT(obj)	 (SWFDEC_AS_OBJECT (obj)->properties != NULL)

struct _SwfdecAsObject {
  GObject		object;

  SwfdecAsContext *	context;	/* context used */
  SwfdecAsObject *	prototype;	/* prototype object (referred to as __prototype__ */
  GHashTable *		properties;	/* properties hash table or NULL when not in GC */
  guint8		flags;		/* GC flags */
  gsize			size;		/* size reserved in GC */
};

struct _SwfdecAsObjectClass {
  GObjectClass		object_class;

  void			(* mark)		(SwfdecAsObject *	object);
};

GType		swfdec_as_object_get_type	(void);

SwfdecAsObject *swfdec_as_object_new		(SwfdecAsContext *    	context);

void		swfdec_as_object_add		(SwfdecAsObject *     	object,
						 SwfdecAsContext *    	context,
						 gsize			size);
void		swfdec_as_object_collect	(SwfdecAsObject *     	object);
void		swfdec_as_object_root		(SwfdecAsObject *     	object);
void		swfdec_as_object_unroot	  	(SwfdecAsObject *     	object);

/* I'd like to name these [gs]et_property, but binding authors will complain
 * about overlap with g_object_[gs]et_property then */
void		swfdec_as_object_set_variable	(SwfdecAsObject *	object,
						 const SwfdecAsValue *	variable,
						 const SwfdecAsValue *	value);
void		swfdec_as_object_get_variable	(SwfdecAsObject *	object,
						 const SwfdecAsValue *	variable,
						 SwfdecAsValue *	value);
void		swfdec_as_object_delete_variable(SwfdecAsObject *	object,
						 const SwfdecAsValue *	variable);
SwfdecAsObject *swfdec_as_object_find_variable	(SwfdecAsObject *	object,
						 const SwfdecAsValue *	variable);

/* shortcuts, you probably don't want to bind them */
#define swfdec_as_object_set(object, name, value) G_STMT_START { \
  SwfdecAsValue __variable; \
  SWFDEC_AS_VALUE_SET_STRING (&__variable, (name)); \
  swfdec_as_object_set_variable ((object), &__variable, (value)); \
}G_STMT_END
#define swfdec_as_object_get(object, name, value) G_STMT_START { \
  SwfdecAsValue __variable; \
  SWFDEC_AS_VALUE_SET_STRING (&__variable, (name)); \
  swfdec_as_object_get_variable ((object), &__variable, (value)); \
}G_STMT_END

void		swfdec_as_object_run		(SwfdecAsObject *       object,
						 SwfdecScript *		script);
gboolean	swfdec_as_object_has_function	(SwfdecAsObject *       object,
						 const char *		name);
void		swfdec_as_object_call		(SwfdecAsObject *       object,
						 const char *		name,
						 guint			argc,
						 SwfdecAsValue *	argv);

G_END_DECLS
#endif
