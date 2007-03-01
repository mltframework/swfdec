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

#ifndef _SWFDEC_SCRIPTABLE_H_
#define _SWFDEC_SCRIPTABLE_H_

#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_types.h>
#include <libswfdec/js/jspubtd.h>

G_BEGIN_DECLS

//typedef struct _SwfdecScriptable SwfdecScriptable;
typedef struct _SwfdecScriptableClass SwfdecScriptableClass;

#define SWFDEC_TYPE_SCRIPTABLE                    (swfdec_scriptable_get_type())
#define SWFDEC_IS_SCRIPTABLE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SCRIPTABLE))
#define SWFDEC_IS_SCRIPTABLE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SCRIPTABLE))
#define SWFDEC_SCRIPTABLE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SCRIPTABLE, SwfdecScriptable))
#define SWFDEC_SCRIPTABLE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SCRIPTABLE, SwfdecScriptableClass))
#define SWFDEC_SCRIPTABLE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_SCRIPTABLE, SwfdecScriptableClass))

struct _SwfdecScriptable {
  GObject		object;

  JSContext *		jscx;		/* context for jsobj */
  JSObject *		jsobj;		/* JS object belonging to us or NULL if none */
};

struct _SwfdecScriptableClass {
  GObjectClass		object_class;

  const JSClass *	jsclass;	/* class used by objects of this type (filled by subclasses) */
  /* the default should be good enough most of the time */
  JSObject *		(* create_js_object)		(SwfdecScriptable *	scriptable);
};

GType			swfdec_scriptable_get_type	(void);

void			swfdec_scriptable_finalize	(JSContext *		cx,
							 JSObject *		obj);

JSObject *		swfdec_scriptable_get_object	(SwfdecScriptable *	scriptable);
gpointer		swfdec_scriptable_from_jsval	(JSContext *		cx,
							 jsval			val,
							 GType			type);
gpointer		swfdec_scriptable_from_object	(JSContext *		cx,
							 JSObject *		object,
							 GType			type);

void			swfdec_scriptable_set_variables	(SwfdecScriptable *	script,
							 const char *		variables);


G_END_DECLS
#endif
