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

#ifndef _SWFDEC_AS_OBJECT_H_
#define _SWFDEC_AS_OBJECT_H_

#include <glib-object.h>
#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_gc_object.h>

G_BEGIN_DECLS

/* NB: matches ASSetPropFlags */
typedef enum {
  SWFDEC_AS_VARIABLE_HIDDEN = (1 << 0),
  SWFDEC_AS_VARIABLE_PERMANENT = (1 << 1),
  SWFDEC_AS_VARIABLE_CONSTANT = (1 << 2),

  SWFDEC_AS_VARIABLE_VERSION_6_UP = (1 << 7),
  SWFDEC_AS_VARIABLE_VERSION_NOT_6 = (1 << 8),
  SWFDEC_AS_VARIABLE_VERSION_7_UP = (1 << 10),
  SWFDEC_AS_VARIABLE_VERSION_8_UP = (1 << 12),
  SWFDEC_AS_VARIABLE_VERSION_9_UP = (1 << 13),
} SwfdecAsVariableFlag;

typedef enum {
  SWFDEC_AS_DELETE_NOT_FOUND = 0,
  SWFDEC_AS_DELETE_DELETED,
  SWFDEC_AS_DELETE_NOT_DELETED
} SwfdecAsDeleteReturn;

typedef struct _SwfdecAsObjectClass SwfdecAsObjectClass;
typedef gboolean (* SwfdecAsVariableForeach) (SwfdecAsObject *object, 
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data);

#define SWFDEC_TYPE_AS_OBJECT                    (swfdec_as_object_get_type())
#define SWFDEC_IS_AS_OBJECT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_OBJECT))
#define SWFDEC_IS_AS_OBJECT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_OBJECT))
#define SWFDEC_AS_OBJECT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_OBJECT, SwfdecAsObject))
#define SWFDEC_AS_OBJECT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_OBJECT, SwfdecAsObjectClass))
#define SWFDEC_AS_OBJECT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_OBJECT, SwfdecAsObjectClass))

struct _SwfdecAsObject {
  /*< protected >*/
  SwfdecGcObject      	object;
  /*< private >*/
  SwfdecAsObject *	prototype;	/* prototype object (referred to as __proto__) */
  guint			prototype_flags; /* propflags for the prototype object */
  GHashTable *		properties;	/* string->SwfdecAsVariable mapping or NULL when not in GC */
  GHashTable *		watches;	/* string->WatchData mapping or NULL when not watching anything */
  GSList *		interfaces;	/* list of interfaces this object implements */
};

struct _SwfdecAsObjectClass {
  SwfdecGcObjectClass	object_class;

  /* get the value and flags for a variables */
  gboolean	      	(* get)			(SwfdecAsObject *       object,
						 SwfdecAsObject *	orig,
						 const char *		variable,
						 SwfdecAsValue *	val,
						 guint *      		flags);
  /* set the variable - and return it (or NULL on error) */
  void			(* set)			(SwfdecAsObject *	object,
						 const char *		variable,
						 const SwfdecAsValue *	val,
						 guint			default_flags);
  /* set flags of a variable */
  void			(* set_flags)	      	(SwfdecAsObject *	object,
						 const char *		variable,
						 guint			flags,
						 guint			mask);
  /* delete the variable - return TRUE if it exists */
  SwfdecAsDeleteReturn	(* del)			(SwfdecAsObject *       object,
						 const char *		variable);
  /* call with every variable until func returns FALSE */
  gboolean		(* foreach)		(SwfdecAsObject *	object,
						 SwfdecAsVariableForeach func,
						 gpointer		data);
  /* get the real object referenced by this object (useful for internal objects) */
  SwfdecAsObject *	(* resolve)		(SwfdecAsObject *	object);
  /* get a debug string representation for this object */
  char *		(* debug)		(SwfdecAsObject *	object);
};

GType		swfdec_as_object_get_type	(void);

SwfdecAsObject *swfdec_as_object_new		(SwfdecAsContext *    	context);
SwfdecAsObject *swfdec_as_object_new_empty    	(SwfdecAsContext *    	context);
void		swfdec_as_object_create		(SwfdecAsFunction *	fun,
						 guint			n_args,
						 const SwfdecAsValue *	args,
						 SwfdecAsValue *	return_value);
void		swfdec_as_object_set_constructor(SwfdecAsObject *	object,
						 SwfdecAsObject *	construct);
SwfdecAsObject *swfdec_as_object_resolve	(SwfdecAsObject *	object);
char *		swfdec_as_object_get_debug	(SwfdecAsObject *	object);

/* I'd like to name these [gs]et_property, but binding authors will complain
 * about overlap with g_object_[gs]et_property then */
#define swfdec_as_object_set_variable(object, variable, value) \
  swfdec_as_object_set_variable_and_flags (object, variable, value, 0)
void		swfdec_as_object_set_variable_and_flags
						(SwfdecAsObject *	object,
						 const char *		variable,
						 const SwfdecAsValue *	value,
						 guint			default_flags);
void		swfdec_as_object_add_variable	(SwfdecAsObject *	object,
						 const char *		variable, 
						 SwfdecAsFunction *	get,
						 SwfdecAsFunction *	set,
						 SwfdecAsVariableFlag	default_flags);
#define swfdec_as_object_get_variable(object, variable, value) \
  swfdec_as_object_get_variable_and_flags (object, variable, value, NULL, NULL)
gboolean	swfdec_as_object_get_variable_and_flags
						(SwfdecAsObject *	object,
						 const char *		variable,
						 SwfdecAsValue *	value,
						 guint *		flags,
						 SwfdecAsObject **	pobject);
SwfdecAsObject *swfdec_as_object_has_variable	(SwfdecAsObject *	object,
						 const char *		variable);
SwfdecAsDeleteReturn
		swfdec_as_object_delete_variable(SwfdecAsObject *	object,
						 const char *		variable);
void		swfdec_as_object_delete_all_variables
						(SwfdecAsObject *	object);
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
SwfdecAsFunction *swfdec_as_object_add_constructor
						(SwfdecAsObject *	object,
						 const char *		name,
						 GType			type,
						 GType			construct_type,
						 SwfdecAsNative		native,
						 guint			min_args,
						 SwfdecAsObject *	prototype);

gboolean	swfdec_as_object_call		(SwfdecAsObject *       object,
						 const char *		name,
						 guint			argc,
						 SwfdecAsValue *	argv,
						 SwfdecAsValue *	return_value);
void		swfdec_as_object_run		(SwfdecAsObject *       object,
						 SwfdecScript *		script);


G_END_DECLS
#endif
