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

typedef gboolean (* SwfdecAsVariableForeach) (SwfdecAsObject *object, 
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data);

struct _SwfdecAsObject {
  /*< private >*/
  SwfdecAsObject *	next;		/* GC management */
  SwfdecAsContext *	context;	/* the context that manages the object */
  gboolean		array:1;	/* TRUE if object is an array */
  gboolean		super:1;	/* TRUE if object is a super object */
  gboolean		movie:1;	/* TRUE if object is really a MovieClip */
  SwfdecAsObject *	prototype;	/* prototype object (referred to as __proto__) */
  guint			prototype_flags; /* propflags for the prototype object */
  GHashTable *		properties;	/* string->SwfdecAsVariable mapping or NULL when not in GC */
  GHashTable *		watches;	/* string->WatchData mapping or NULL when not watching anything */
  GSList *		interfaces;	/* list of interfaces this object implements */
  SwfdecAsRelay	*	relay;		/* object we relay data to */
};


SwfdecAsObject *swfdec_as_object_new		(SwfdecAsContext *    	context,
						 ...) G_GNUC_NULL_TERMINATED;
SwfdecAsObject *swfdec_as_object_new_empty    	(SwfdecAsContext *    	context);
SwfdecAsObject * swfdec_as_object_set_constructor_by_name 
						(SwfdecAsObject *	object,
						 const char *		name,
						 ...) G_GNUC_NULL_TERMINATED;
SwfdecAsObject * swfdec_as_object_set_constructor_by_namev 
						(SwfdecAsObject *	object,
						 const char *		name,
						 va_list		args);
void		swfdec_as_object_create		(SwfdecAsFunction *	fun,
						 guint			n_args,
						 const SwfdecAsValue *	args,
						 SwfdecAsValue *	return_value);
void		swfdec_as_object_set_constructor(SwfdecAsObject *	object,
						 SwfdecAsObject *	construct);
SwfdecAsObject *swfdec_as_object_resolve	(SwfdecAsObject *	object);
void		swfdec_as_object_mark		(SwfdecAsObject *	object);

void		swfdec_as_object_set_relay	(SwfdecAsObject *	object,
						 SwfdecAsRelay *	relay);

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
						 SwfdecAsNative		native);
SwfdecAsFunction *swfdec_as_object_add_constructor
						(SwfdecAsObject *	object,
						 const char *		name,
						 GType			construct_type,
						 SwfdecAsNative		native,
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
