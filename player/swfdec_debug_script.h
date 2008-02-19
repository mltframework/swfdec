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

#ifndef _SWFDEC_DEBUG_SCRIPT_H_
#define _SWFDEC_DEBUG_SCRIPT_H_

#include <gtk/gtk.h>
#include <swfdec/swfdec_debugger.h>

G_BEGIN_DECLS

typedef struct _SwfdecDebugScript SwfdecDebugScript;
typedef struct _SwfdecDebugScriptClass SwfdecDebugScriptClass;

#define SWFDEC_TYPE_DEBUG_SCRIPT                    (swfdec_debug_script_get_type())
#define SWFDEC_IS_DEBUG_SCRIPT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DEBUG_SCRIPT))
#define SWFDEC_IS_DEBUG_SCRIPT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DEBUG_SCRIPT))
#define SWFDEC_DEBUG_SCRIPT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DEBUG_SCRIPT, SwfdecDebugScript))
#define SWFDEC_DEBUG_SCRIPT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DEBUG_SCRIPT, SwfdecDebugScriptClass))

struct _SwfdecDebugScript
{
  GtkTreeView		treeview;

  SwfdecDebugger *	debugger;
  SwfdecDebuggerScript *script;
};

struct _SwfdecDebugScriptClass
{
  GtkTreeViewClass	treeview_class;
};

GType		swfdec_debug_script_get_type		(void);

GtkWidget *	swfdec_debug_script_new			(SwfdecDebugger *	debugger);

void		swfdec_debug_script_set_script		(SwfdecDebugScript *	script,
							 SwfdecDebuggerScript *	dscript);

G_END_DECLS
#endif
