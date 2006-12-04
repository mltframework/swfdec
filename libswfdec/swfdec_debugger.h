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
#include <libswfdec/swfdec_player_internal.h>
#include <libswfdec/swfdec_types.h>
#include <libswfdec/js/jsapi.h>

G_BEGIN_DECLS

//typedef struct _SwfdecDebugger SwfdecDebugger;
typedef struct _SwfdecDebuggerClass SwfdecDebuggerClass;
typedef struct _SwfdecDebuggerScript SwfdecDebuggerScript;
typedef struct _SwfdecDebuggerCommand SwfdecDebuggerCommand;

#define SWFDEC_TYPE_DEBUGGER                    (swfdec_debugger_get_type())
#define SWFDEC_IS_DEBUGGER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DEBUGGER))
#define SWFDEC_IS_DEBUGGER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DEBUGGER))
#define SWFDEC_DEBUGGER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DEBUGGER, SwfdecDebugger))
#define SWFDEC_DEBUGGER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DEBUGGER, SwfdecDebuggerClass))
#define SWFDEC_DEBUGGER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_DEBUGGER, SwfdecDebuggerClass))

struct _SwfdecDebuggerCommand {
  gpointer		code;		/* pointer to start bytecode in JScript */
  guint			breakpoint;	/* id of breakpoint pointing here */
  char *		description;	/* string describing the action */
};

struct _SwfdecDebuggerScript {
  JSScript *		script;		/* the script */
  char *		name;		/* descriptive name of script */
  SwfdecDebuggerCommand *commands;	/* commands executed in the script (NULL-terminated) */
  guint			n_commands;	/* number of commands */
};

struct _SwfdecDebugger {
  SwfdecPlayer		player;

  GHashTable *		scripts;	/* JSScript => SwfdecDebuggerScript mapping */
  GArray *		breakpoints;	/* all breakpoints */
};

struct _SwfdecDebuggerClass {
  SwfdecPlayerClass   	player_class;
};

GType			swfdec_debugger_get_type        (void);
SwfdecPlayer *		swfdec_debugger_new		(void);

guint			swfdec_debugger_set_breakpoint	(SwfdecDebugger *	debugger,
							 SwfdecDebuggerScript *	script,
							 guint			line);
void			swfdec_debugger_unset_breakpoint
							(SwfdecDebugger *	debugger,
							 guint			id);
gboolean		swfdec_debugger_get_breakpoint	(SwfdecDebugger *	debugger,
							 guint			id,
							 SwfdecDebuggerScript **script,
							 guint *      		line);
guint			swfdec_debugger_get_n_breakpoints
							(SwfdecDebugger *	debugger);

void			swfdec_debugger_add_script	(SwfdecDebugger *	debugger,
							 JSScript *		script,
							 const char *		name,
							 SwfdecDebuggerCommand *commands,
							 guint			n_commands);
void			swfdec_debugger_remove_script	(SwfdecDebugger *	debugger,
							 JSScript *		script);
void			swfdec_debugger_foreach_script	(SwfdecDebugger *	debugger,
							 GFunc			func,
							 gpointer		data);


G_END_DECLS
#endif
