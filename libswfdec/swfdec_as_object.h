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
  SWFDEC_AS_VARIABLE_NATIVE = (1 << 3)
} SwfdecAsVariableFlag;

typedef struct _SwfdecAsObjectClass SwfdecAsObjectClass;
typedef gboolean (* SwfdecAsVariableForeach) (SwfdecAsObject *object, 
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data);
typedef SwfdecAsVariableForeach SwfdecAsVariableForeachRemove;
typedef const char *(* SwfdecAsVariableForeachRename) (SwfdecAsObject *object, 
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data);

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
  /* get the value and flags for a variables */
  gboolean	      	(* get)			(SwfdecAsObject *       object,
						 const char *		variable,
						 SwfdecAsValue *	val,
						 guint *      		flags);
  /* set the variable - and return it (or NULL on error) */
  void			(* set)			(SwfdecAsObject *	object,
						 const char *		variable,
						 const SwfdecAsValue *	val);
  /* set flags of a variable */
  void			(* set_flags)	      	(SwfdecAsObject *	object,
						 const char *		variable,
						 guint			flags,
						 guint			mask);
  /* delete the variable - return TRUE if it exists */
  gboolean		(* delete)		(SwfdecAsObject *       object,
						 const char *		variable);
  /* call with every variable until func returns FALSE */
  gboolean		(* foreach)		(SwfdecAsObject *	object,
						 SwfdecAsVariableForeach func,
						 gpointer		data);
  char *		(* debug)		(SwfdecAsObject *	object);
};

GType		swfdec_as_object_get_type	(void);

SwfdecAsObject *swfdec_as_object_new		(SwfdecAsContext *    	context);
SwfdecAsObject *swfdec_as_object_new_empty    	(SwfdecAsContext *    	context);
void		swfdec_as_object_create		(SwfdecAsFunction *	construct,
						 guint			n_args,
						 const SwfdecAsValue *	args,
						 gboolean		scripted);
void		swfdec_as_object_set_constructor(SwfdecAsObject *	object,
						 SwfdecAsObject *	construct,
						 gboolean		scripted);
char *		swfdec_as_object_get_debug	(SwfdecAsObject *	object);

void		swfdec_as_object_add		(SwfdecAsObject *     	object,
						 SwfdecAsContext *    	context,
						 gsize			size);
void		swfdec_as_object_collect	(SwfdecAsObject *     	object);

/* I'd like to name these [gs]et_property, but binding authors will complain
 * about overlap with g_object_[gs]et_property then */
void		swfdec_as_object_set_variable	(SwfdecAsObject *	object,
						 const char *		variable,
						 const SwfdecAsValue *	value);
#define swfdec_as_object_get_variable(object, variable, value) \
  swfdec_as_object_get_variable_and_flags (object, variable, value, NULL, NULL)
gboolean	swfdec_as_object_get_variable_and_flags
						(SwfdecAsObject *	object,
						 const char *		variable,
						 SwfdecAsValue *	value,
						 guint *		flags,
						 SwfdecAsObject **	pobject);
gboolean	swfdec_as_object_delete_variable(SwfdecAsObject *	object,
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
guint		swfdec_as_object_foreach_remove	(SwfdecAsObject *       object,
						 SwfdecAsVariableForeach func,
						 gpointer		data);
void		swfdec_as_object_foreach_rename	(SwfdecAsObject *       object,
						 SwfdecAsVariableForeachRename func,
						 gpointer		data);

SwfdecAsFunction *swfdec_as_object_add_function	(SwfdecAsObject *	object,
						 const char *		name,
						 GType			type,
						 SwfdecAsNative		native,
						 guint			min_args);

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


G_END_DECLS
#endif
