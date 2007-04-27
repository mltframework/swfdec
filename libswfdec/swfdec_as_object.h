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

#ifndef _SWFDEC_AS_OBJECT_H_
#define _SWFDEC_AS_OBJECT_H_

#include <glib-object.h>
#include <libswfdec/swfdec_as_types.h>
#include <libswfdec/swfdec_types.h>

G_BEGIN_DECLS

/* NB: matches ASSetPropFlags */
typedef enum {
  /* ActionScript flags go here */
  SWFDEC_AS_VARIABLE_DONT_ENUM = (1 << 0),
  SWFDEC_AS_VARIABLE_PERMANENT = (1 << 1),
  SWFDEC_AS_VARIABLE_READONLY = (1 << 2),
  /* internal flags go here */
  SWFDEC_AS_VARIABLE_NATIVE = (1 << 3),
  SWFDEC_AS_VARIABLE_TEMPORARY = (1 << 4)
} SwfdecAsVariableFlag;

typedef struct _SwfdecAsObjectClass SwfdecAsObjectClass;
typedef struct _SwfdecAsVariable SwfdecAsVariable;
typedef void (* SwfdecAsVariableSetter) (SwfdecAsObject *object, const SwfdecAsValue *value);
typedef void (* SwfdecAsVariableGetter) (SwfdecAsObject *object, SwfdecAsValue *value);
typedef gboolean (* SwfdecAsVariableForeach) (SwfdecAsObject *object, 
    const char *variable, SwfdecAsVariable *value, gpointer data);

struct _SwfdecAsVariable {
  guint			flags;		/* SwfdecAsVariableFlag values */
  union {
    SwfdecAsValue     	value;		/* value of property */
    struct {
      SwfdecAsVariableSetter set;	/* setter for native property */
      SwfdecAsVariableGetter get;	/* getter for native property */
    } funcs;
  }			value;
};
#define swfdec_as_variable_mark(var) G_STMT_START{ \
  SwfdecAsVariable *__var = (SwfdecAsVariable *) (var); \
  if (!(__var->flags & SWFDEC_AS_VARIABLE_NATIVE)) \
    swfdec_as_value_mark (&__var->value.value); \
}G_STMT_END

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
  SwfdecAsObject *	prototype;	/* prototype object (referred to as __proto__) */
  GHashTable *		properties;	/* string->SwfdecAsVariable mapping or NULL when not in GC */
  guint8		flags;		/* GC flags */
  gsize			size;		/* size reserved in GC */
};

struct _SwfdecAsObjectClass {
  GObjectClass		object_class;

  /* mark everything that should survive during GC */
  void			(* mark)		(SwfdecAsObject *	object);
  /* object was added to the context */
  void			(* add)			(SwfdecAsObject *	object);
  /* get the place that holds a variable or return NULL */
  SwfdecAsVariable *	(* get)			(SwfdecAsObject *       object,
						 const char *		variable,
						 gboolean		create);
  /* delete the variable - it does exists */
  void			(* delete)		(SwfdecAsObject *       object,
						 const char *		variable);
  /* call with every variable until func returns FALSE */
  gboolean		(* foreach)		(SwfdecAsObject *	object,
						 SwfdecAsVariableForeach func,
						 gpointer		data);
};

GType		swfdec_as_object_get_type	(void);

SwfdecAsObject *swfdec_as_object_new		(SwfdecAsContext *    	context);
SwfdecAsObject *swfdec_as_object_create		(SwfdecAsFunction *	construct,
						 guint			n_args,
						 SwfdecAsValue *	args);

void		swfdec_as_object_add		(SwfdecAsObject *     	object,
						 SwfdecAsContext *    	context,
						 gsize			size);
void		swfdec_as_object_collect	(SwfdecAsObject *     	object);
void		swfdec_as_object_root		(SwfdecAsObject *     	object);
void		swfdec_as_object_unroot	  	(SwfdecAsObject *     	object);

/* I'd like to name these [gs]et_property, but binding authors will complain
 * about overlap with g_object_[gs]et_property then */
void		swfdec_as_object_set_variable	(SwfdecAsObject *	object,
						 const char *		variable,
						 const SwfdecAsValue *	value);
void		swfdec_as_object_get_variable	(SwfdecAsObject *	object,
						 const char *		variable,
						 SwfdecAsValue *	value);
void		swfdec_as_object_delete_variable(SwfdecAsObject *	object,
						 const char *		variable);
SwfdecAsObject *swfdec_as_object_find_variable	(SwfdecAsObject *	object,
						 const char *		variable);
void		swfdec_as_object_set_variable_flags
						(SwfdecAsObject *       object,
						 const char *		variable,
						 SwfdecAsVariableFlag	flags);
void		swfdec_as_object_unset_variable_flags
						(SwfdecAsObject *       object,
						 const char *		variable,
						 SwfdecAsVariableFlag	flags);
gboolean	swfdec_as_object_foreach	(SwfdecAsObject *       object,
						 SwfdecAsVariableForeach func,
						 gpointer		data);

SwfdecAsFunction *swfdec_as_object_add_function	(SwfdecAsObject *	object,
						 const char *		name,
						 GType			type,
						 SwfdecAsNative		native,
						 guint			min_args);
void		swfdec_as_object_add_variable	(SwfdecAsObject *	object,
						 const char *		name,
						 SwfdecAsVariableSetter	set,
						 SwfdecAsVariableGetter get);

void		swfdec_as_object_run		(SwfdecAsObject *       object,
						 SwfdecScript *		script);
gboolean	swfdec_as_object_has_function	(SwfdecAsObject *       object,
						 const char *		name);
void		swfdec_as_object_call		(SwfdecAsObject *       object,
						 const char *		name,
						 guint			argc,
						 SwfdecAsValue *	argv,
						 SwfdecAsValue *	return_value);
						 
void		swfdec_as_object_init_context	(SwfdecAsContext *	context,
					      	 guint			version);

void		swfdec_as_variable_set		(SwfdecAsObject *	object,
						 SwfdecAsVariable *	var,
						 const SwfdecAsValue *	value);
void		swfdec_as_variable_get		(SwfdecAsObject *	object,
						 SwfdecAsVariable *	var,
						 SwfdecAsValue *	value);


G_END_DECLS
#endif
