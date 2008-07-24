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
#include "vivi_wrap.h"

G_DEFINE_TYPE (ViviBreakpoint, vivi_breakpoint, SWFDEC_TYPE_AS_OBJECT)

static gboolean
vivi_breakpoint_step (ViviDebugger *debugger, ViviBreakpoint *breakpoint)
{
  SwfdecAsObject *obj = SWFDEC_AS_OBJECT (breakpoint);
  SwfdecAsValue retval;

  swfdec_as_object_call (obj, swfdec_as_context_get_string (swfdec_gc_object_get_context (obj), "onCommand"), 0, NULL, &retval);
  return swfdec_as_value_to_boolean (swfdec_gc_object_get_context (obj), &retval);
}

static gboolean
vivi_breakpoint_enter_frame (ViviDebugger *debugger, SwfdecAsFrame *frame, ViviBreakpoint *breakpoint)
{
  SwfdecAsObject *obj = SWFDEC_AS_OBJECT (breakpoint);
  SwfdecAsValue val;
  SwfdecAsValue retval;

  SWFDEC_AS_VALUE_SET_OBJECT (&val, vivi_wrap_object (VIVI_APPLICATION (swfdec_gc_object_get_context (obj)), SWFDEC_AS_OBJECT (frame)));
  swfdec_as_object_call (obj, swfdec_as_context_get_string (swfdec_gc_object_get_context (obj), "onEnterFrame"), 1, &val, &retval);
  return swfdec_as_value_to_boolean (swfdec_gc_object_get_context (obj), &retval);
}

static gboolean
vivi_breakpoint_leave_frame (ViviDebugger *debugger, SwfdecAsFrame *frame, const SwfdecAsValue *ret, ViviBreakpoint *breakpoint)
{
  SwfdecAsObject *obj = SWFDEC_AS_OBJECT (breakpoint);
  SwfdecAsValue vals[2];
  SwfdecAsValue retval;

  SWFDEC_AS_VALUE_SET_OBJECT (&vals[0], vivi_wrap_object (VIVI_APPLICATION (swfdec_gc_object_get_context (obj)), SWFDEC_AS_OBJECT (frame)));
  vivi_wrap_value (VIVI_APPLICATION (swfdec_gc_object_get_context (obj)), &vals[1], ret);
  swfdec_as_object_call (obj, swfdec_as_context_get_string (swfdec_gc_object_get_context (obj), "onLeaveFrame"), 2, vals, &retval);
  return swfdec_as_value_to_boolean (swfdec_gc_object_get_context (obj), &retval);
}

static gboolean
vivi_breakpoint_set_variable (ViviDebugger *debugger, SwfdecAsObject *object, 
    const char *variable, const SwfdecAsValue *value, ViviBreakpoint *breakpoint)
{
  SwfdecAsObject *obj = SWFDEC_AS_OBJECT (breakpoint);
  SwfdecAsValue vals[3];
  SwfdecAsValue retval;

  SWFDEC_AS_VALUE_SET_OBJECT (&vals[0], vivi_wrap_object (VIVI_APPLICATION (swfdec_gc_object_get_context (obj)), object));
  SWFDEC_AS_VALUE_SET_STRING (&vals[1], swfdec_as_context_get_string (swfdec_gc_object_get_context (obj), variable));
  vivi_wrap_value (VIVI_APPLICATION (swfdec_gc_object_get_context (obj)), &vals[2], value);
  swfdec_as_object_call (obj, swfdec_as_context_get_string (swfdec_gc_object_get_context (obj), "onSetVariable"), 3, vals, &retval);
  return swfdec_as_value_to_boolean (swfdec_gc_object_get_context (obj), &retval);
}

static const struct {
  const char *	event;
  const char *	signal;
  GCallback	handler;
} events[] = {
  { NULL, NULL, NULL }, /* invalid */
  { "onCommand", "step", G_CALLBACK (vivi_breakpoint_step) },
  { "onEnterFrame", "enter-frame", G_CALLBACK (vivi_breakpoint_enter_frame) },
  { "onLeaveFrame", "leave-frame", G_CALLBACK (vivi_breakpoint_leave_frame) },
  { "onSetVariable", "set-variable", G_CALLBACK (vivi_breakpoint_set_variable) }
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
vivi_breakpoint_add (ViviBreakpoint *breakpoint, guint i)
{
  ViviDebugger *debugger = VIVI_APPLICATION (swfdec_gc_object_get_context (breakpoint))->debugger;

  g_assert (i != 0);
  if (breakpoint->active) {
    breakpoint->handlers[i] = g_signal_connect (debugger, events[i].signal,
	events[i].handler, breakpoint);
  } else {
    breakpoint->handlers[i] = 1;
  }
}

static void
vivi_breakpoint_remove (ViviBreakpoint *breakpoint, guint i)
{
  ViviDebugger *debugger = VIVI_APPLICATION (swfdec_gc_object_get_context (breakpoint))->debugger;

  g_assert (i != 0);
  if (breakpoint->active)
    g_signal_handler_disconnect (debugger, breakpoint->handlers[i]);
  breakpoint->handlers[i] = 0;
}

static void
vivi_breakpoint_set (SwfdecAsObject *object, const char *variable, const SwfdecAsValue *val, guint flags)
{
  guint i;

  i = vivi_breakpoint_find_event (variable);
  if (i) {
    ViviBreakpoint *breakpoint = VIVI_BREAKPOINT (object);
    if (SWFDEC_AS_VALUE_IS_OBJECT (val) &&
	SWFDEC_IS_AS_FUNCTION (SWFDEC_AS_VALUE_GET_OBJECT (val))) {
      if (!breakpoint->handlers[i])
	vivi_breakpoint_add (breakpoint, i);
    } else {
      if (breakpoint->handlers[i])
	vivi_breakpoint_remove (breakpoint, i);
    }
  }
  SWFDEC_AS_OBJECT_CLASS (vivi_breakpoint_parent_class)->set (object, variable, val, flags);
}

static SwfdecAsDeleteReturn
vivi_breakpoint_delete (SwfdecAsObject *object, const char *variable)
{
  ViviBreakpoint *breakpoint = VIVI_BREAKPOINT (object);
  guint i;
  SwfdecAsDeleteReturn ret;

  ret = SWFDEC_AS_OBJECT_CLASS (vivi_breakpoint_parent_class)->del (object, variable);

  if (ret == SWFDEC_AS_DELETE_DELETED) {
    i = vivi_breakpoint_find_event (variable);
    if (i && breakpoint->handlers[i])
      vivi_breakpoint_remove (breakpoint, i);
  }

  return ret;
}

static void
vivi_breakpoint_dispose (GObject *object)
{
  ViviBreakpoint *breakpoint = VIVI_BREAKPOINT (object);

  vivi_breakpoint_set_active (breakpoint, FALSE);

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
  breakpoint->active = TRUE;
}

void
vivi_breakpoint_set_active (ViviBreakpoint *breakpoint, gboolean active)
{
  guint i;

  g_return_if_fail (VIVI_IS_BREAKPOINT (breakpoint));

  g_print ("active = %d", active);
  if (breakpoint->active == active)
    return;
  breakpoint->active = !breakpoint->active;
  for (i = 1; i < G_N_ELEMENTS (events); i++) {
    if (breakpoint->handlers[i] == 0)
      continue;
    /* FIXME: this is hacky */
    breakpoint->active = !breakpoint->active;
    vivi_breakpoint_remove (breakpoint, i);
    breakpoint->active = !breakpoint->active;
    vivi_breakpoint_add (breakpoint, i);
  }
}

/*** AS CODE ***/

VIVI_FUNCTION ("breakpoint_active_get", vivi_breakpoint_as_get_active)
void
vivi_breakpoint_as_get_active (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  if (!VIVI_IS_BREAKPOINT (this))
    return;

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, VIVI_BREAKPOINT (this)->active);
}

VIVI_FUNCTION ("breakpoint_active_set", vivi_breakpoint_as_set_active)
void
vivi_breakpoint_as_set_active (SwfdecAsContext *cx, SwfdecAsObject *this,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  if (!VIVI_IS_BREAKPOINT (this) ||
      argc == 0)
    return;
  vivi_breakpoint_set_active (VIVI_BREAKPOINT (this), swfdec_as_value_to_boolean (cx, &argv[0]));
}

