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
 * SwfdecAsDebuggerClass
 * @step: This function is called everytime just before a bytecode is executed by 
 *        the script engine. So it's very powerful, but can also slow down the
 *        script engine a lot.
 * @frame_change: Called whenever the current frame changes. This happens either
 *                when calling a function or when a function call returns. The
 *                function call is called for any type of frame, even calls to
 *                native functions.
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

