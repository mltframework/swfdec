/* Vivified
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

#include "vivi_breakpoint.h"
#include "vivi_application.h"
#include "vivi_function.h"

G_DEFINE_TYPE (ViviBreakpoint, vivi_breakpoint, SWFDEC_TYPE_AS_OBJECT)

static gboolean
vivi_breakpoint_step (ViviDebugger *debugger, ViviBreakpoint *breakpoint)
{
  SwfdecAsObject *obj = SWFDEC_AS_OBJECT (breakpoint);
  SwfdecAsValue val;

  swfdec_as_object_call (obj, swfdec_as_context_get_string (obj->context, "onStep"), 0, NULL, &val);
  return swfdec_as_value_to_boolean (obj->context, &val);
}

static const struct {
  const char *	event;
  const char *	signal;
  GCallback	handler;
} events[] = {
  { NULL, NULL, NULL }, /* invalid */
  { "onStep", "step", G_CALLBACK (vivi_breakpoint_step) }
};

static guint
vivi_breakpoint_find_event (const char *name)
{
  guint i;

  for (i = 1; i < G_N_ELEMENTS (events); i++) {
    if (g_str_equal (events[i].event, name))
      return i;
  }
  return 0;
}

static void
vivi_breakpoint_set (SwfdecAsObject *object, const char *variable, const SwfdecAsValue *val)
{
  guint i;

  i = vivi_breakpoint_find_event (variable);
  if (i) {
    ViviBreakpoint *breakpoint = VIVI_BREAKPOINT (object);
    ViviDebugger *debugger = VIVI_APPLICATION (object->context)->debugger;
    if (SWFDEC_AS_VALUE_IS_OBJECT (val) &&
	SWFDEC_IS_AS_FUNCTION (SWFDEC_AS_VALUE_GET_OBJECT (val))) {
      if (!breakpoint->handlers[i])
	breakpoint->handlers[i] = g_signal_connect (debugger, events[i].signal,
	    events[i].handler, object);
    } else {
      if (breakpoint->handlers[i]) {
	g_signal_handler_disconnect (debugger, breakpoint->handlers[i]);
	breakpoint->handlers[i] = 0;
      }
    }
  }
  SWFDEC_AS_OBJECT_CLASS (vivi_breakpoint_parent_class)->set (object, variable, val);
}

static gboolean
vivi_breakpoint_delete (SwfdecAsObject *object, const char *variable)
{
  ViviBreakpoint *breakpoint = VIVI_BREAKPOINT (object);
  guint i;

  i = vivi_breakpoint_find_event (variable);
  if (i && breakpoint->handlers[i]) {
    ViviDebugger *debugger = VIVI_APPLICATION (object->context)->debugger;
    g_signal_handler_disconnect (debugger, breakpoint->handlers[i]);
    breakpoint->handlers[i] = 0;
  }

  return SWFDEC_AS_OBJECT_CLASS (vivi_breakpoint_parent_class)->del (object, variable);
}

static void
vivi_breakpoint_dispose (GObject *object)
{
  ViviBreakpoint *breakpoint = VIVI_BREAKPOINT (object);
  ViviDebugger *debugger = VIVI_APPLICATION (SWFDEC_AS_OBJECT (breakpoint)->context)->debugger;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (events); i++) {
    if (breakpoint->handlers[i])
      g_signal_handler_disconnect (debugger, breakpoint->handlers[i]);
  }
  G_OBJECT_CLASS (vivi_breakpoint_parent_class)->dispose (object);
}

static void
vivi_breakpoint_class_init (ViviBreakpointClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *as_object_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = vivi_breakpoint_dispose;

  as_object_class->set = vivi_breakpoint_set;
  as_object_class->del = vivi_breakpoint_delete;
}

static void
vivi_breakpoint_init (ViviBreakpoint *breakpoint)
{
}

/*** AS CODE ***/

