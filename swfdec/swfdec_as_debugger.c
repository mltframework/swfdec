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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>

#include "swfdec_as_debugger.h"
#include "swfdec_as_context.h"

G_DEFINE_TYPE (SwfdecAsDebugger, swfdec_as_debugger, G_TYPE_OBJECT)

/**
 * SECTION:SwfdecAsDebugger
 * @title: SwfdecAsDebugger
 * @short_description: the debugger object
 * @see also: SwfdecAsContext
 *
 * The debugger object is a special object that can be set on a #SwfdecAsContext
 * upon creation. If that is done, the debugger can then be used to inspect the 
 * running Actionscript application.
 */

/**
 * SwfdecAsDebugger
 *
 * This is the type of the debugger object.
 */

/**
 * SwfdecAsDebuggerClass:
 * @add: Called whenever an object is added to the garbage collection engine 
 *       using swfdec_as_object_add ()
 * @remove: Called whenever an object is about to be collected by the garbage 
 *          collector.
 * @step: This function is called everytime just before a bytecode is executed 
 *        by the script engine. So it's very powerful, but can also slow down 
 *        the script engine a lot.
 * @enter_frame: Called after a new #SwfdecAsFrame has been initialized and 
 *               pushed to the top of the execution stack.
 * @leave_frame: Called just after a #SwfdecAsFrame has been removed from the
 *		  execution stack. The return value has not been forwarded to
 *		  the parent when this function is called.
 * @set_variable: Called whenever swfdec_as_object_set_variable() is called, 
 *                before actually setting the variable. This function is also
 *                called when variables are set by internal code, not just when
 *                interpreting scripts. It also doesn't matter if setting the 
 *                variable will succeed. 
 *
 * The class object for the debugger. You need to override these functions to 
 * get useful functionality for the debugger.
 */

static void
swfdec_as_debugger_class_init (SwfdecAsDebuggerClass *klass)
{
}

static void
swfdec_as_debugger_init (SwfdecAsDebugger *debugger)
{
}

