/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_DEBUGGER_H_
#define _SWFDEC_DEBUGGER_H_

#include <glib-object.h>
#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_types.h>
#include <libswfdec/js/jsapi.h>

G_BEGIN_DECLS

//typedef struct _SwfdecDebugger SwfdecDebugger;
typedef struct _SwfdecDebuggerClass SwfdecDebuggerClass;
typedef struct _SwfdecDebuggerScript SwfdecDebuggerScript;

#define SWFDEC_TYPE_DEBUGGER                    (swfdec_debugger_get_type())
#define SWFDEC_IS_DEBUGGER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DEBUGGER))
#define SWFDEC_IS_DEBUGGER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DEBUGGER))
#define SWFDEC_DEBUGGER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DEBUGGER, SwfdecDebugger))
#define SWFDEC_DEBUGGER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DEBUGGER, SwfdecDebuggerClass))
#define SWFDEC_DEBUGGER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_DEBUGGER, SwfdecDebuggerClass))

struct _SwfdecDebuggerScript {
  JSScript *		script;
  GArray *		commands;
};

struct _SwfdecDebugger {
  GObject		object;

  GHashTable *		scripts;	/* JSScript => SwfdecDebuggerScript mapping */
};

struct _SwfdecDebuggerClass {
  GObjectClass		object_class;
};

GType			swfdec_debugger_get_type        (void);
SwfdecDebugger *	swfdec_debugger_get		(SwfdecPlayer *	player);


G_END_DECLS
#endif
