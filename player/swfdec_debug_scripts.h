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

#ifndef _SWFDEC_DEBUG_SCRIPTS_H_
#define _SWFDEC_DEBUG_SCRIPTS_H_

#include <gtk/gtk.h>
#include <swfdec/swfdec_debugger.h>

G_BEGIN_DECLS

typedef struct _SwfdecDebugScripts SwfdecDebugScripts;
typedef struct _SwfdecDebugScriptsClass SwfdecDebugScriptsClass;

#define SWFDEC_TYPE_DEBUG_SCRIPTS                    (swfdec_debug_scripts_get_type())
#define SWFDEC_IS_DEBUG_SCRIPTS(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DEBUG_SCRIPTS))
#define SWFDEC_IS_DEBUG_SCRIPTS_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DEBUG_SCRIPTS))
#define SWFDEC_DEBUG_SCRIPTS(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DEBUG_SCRIPTS, SwfdecDebugScripts))
#define SWFDEC_DEBUG_SCRIPTS_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DEBUG_SCRIPTS, SwfdecDebugScriptsClass))

struct _SwfdecDebugScripts
{
  GtkTreeView		treeview;

  SwfdecDebugger *	debugger;
};

struct _SwfdecDebugScriptsClass
{
  GtkTreeViewClass	treeview_class;
};

GType		swfdec_debug_scripts_get_type		(void);

GtkWidget *	swfdec_debug_scripts_new		(SwfdecDebugger *	debugger);


G_END_DECLS
#endif
