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

#ifndef _SWFDEC_AS_DEBUGGER_H_
#define _SWFDEC_AS_DEBUGGER_H_

#include <libswfdec/swfdec_as_object.h>
#include <libswfdec/swfdec_as_types.h>
#include <libswfdec/swfdec_script.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsDebuggerClass SwfdecAsDebuggerClass;

#define SWFDEC_TYPE_AS_DEBUGGER                    (swfdec_as_debugger_get_type())
#define SWFDEC_IS_AS_DEBUGGER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_DEBUGGER))
#define SWFDEC_IS_AS_DEBUGGER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_DEBUGGER))
#define SWFDEC_AS_DEBUGGER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_DEBUGGER, SwfdecAsDebugger))
#define SWFDEC_AS_DEBUGGER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_DEBUGGER, SwfdecAsDebuggerClass))
#define SWFDEC_AS_DEBUGGER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_DEBUGGER, SwfdecAsDebuggerClass))

struct _SwfdecAsDebugger {
  /*< private >*/
  GObject		object;
};

struct _SwfdecAsDebuggerClass {
  GObjectClass		object_class;

  /* called before executing a bytecode */
  void			(* step)	(SwfdecAsDebugger *	debugger,
					 SwfdecAsContext *	context);
  /* called after adding a frame from the function stack */
  void			(* start_frame)	(SwfdecAsDebugger *	debugger,
					 SwfdecAsContext *	context,
					 SwfdecAsFrame *	frame);
  /* called after removing a frame from the function stack */
  void			(* finish_frame)(SwfdecAsDebugger *	debugger,
					 SwfdecAsContext *	context,
					 SwfdecAsFrame *	frame,
					 SwfdecAsValue *	return_value);
};

GType		swfdec_as_debugger_get_type	(void);


G_END_DECLS
#endif
