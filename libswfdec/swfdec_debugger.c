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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include "swfdec_debugger.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_player_internal.h"
#include "js/jsdbgapi.h"

/*** SwfdecDebuggerScript ***/

static SwfdecDebuggerScript *
swfdec_debugger_script_new (JSScript *script, const char *name, 
    SwfdecDebuggerCommand *commands, guint n_commands)
{
  SwfdecDebuggerScript *ret;

  ret = g_new0 (SwfdecDebuggerScript, 1);
  ret->script = script;
  ret->name = g_strdup (name);
  ret->commands = commands;
  ret->n_commands = n_commands;

  return ret;
}

static void
swfdec_debugger_script_free (SwfdecDebuggerScript *script)
{
  g_assert (script);
  g_free (script->commands);
  g_free (script);
}

/*** SwfdecDebugger ***/

enum {
  SCRIPT_ADDED,
  SCRIPT_REMOVED,
  BREAKPOINT,
  BREAKPOINT_ADDED,
  BREAKPOINT_REMOVED,
  LAST_SIGNAL
};

G_DEFINE_TYPE (SwfdecDebugger, swfdec_debugger, SWFDEC_TYPE_PLAYER)
guint signals [LAST_SIGNAL] = { 0, };

static void
swfdec_debugger_dispose (GObject *object)
{
  SwfdecDebugger *debugger = SWFDEC_DEBUGGER (object);

  g_assert (g_hash_table_size (debugger->scripts) == 0);
  g_hash_table_destroy (debugger->scripts);
  if (debugger->breakpoints)
    g_array_free (debugger->breakpoints, TRUE);

  G_OBJECT_CLASS (swfdec_debugger_parent_class)->dispose (object);
}

static void
swfdec_debugger_class_init (SwfdecDebuggerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_debugger_dispose;

  signals[SCRIPT_ADDED] = g_signal_new ("script-added", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
      G_TYPE_NONE, 1, G_TYPE_POINTER);
  signals[SCRIPT_REMOVED] = g_signal_new ("script-removed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__POINTER,
      G_TYPE_NONE, 1, G_TYPE_POINTER);
  signals[BREAKPOINT] = g_signal_new ("breakpoint", 
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
      g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);
  signals[BREAKPOINT_ADDED] = g_signal_new ("breakpoint-added", 
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
      g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);
  signals[BREAKPOINT_REMOVED] = g_signal_new ("breakpoint-removed", 
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
      g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);
}

static void
swfdec_debugger_init (SwfdecDebugger *debugger)
{
  debugger->scripts = g_hash_table_new_full (g_direct_hash, g_direct_equal, 
      NULL, (GDestroyNotify) swfdec_debugger_script_free);
}

/**
 * swfdec_debugger_new:
 *
 * Creates a #SwfdecPlayer that can be debugged.
 *
 * Returns: a new #SwfdecDebugger
 **/
SwfdecPlayer *
swfdec_debugger_new (void)
{
  SwfdecPlayer *player;

  swfdec_init ();
  player = g_object_new (SWFDEC_TYPE_DEBUGGER, NULL);
  return player;
}

void
swfdec_debugger_add_script (SwfdecDebugger *debugger, JSScript *jscript, const char *name,
    SwfdecDebuggerCommand *commands, guint n_commands)
{
  SwfdecDebuggerScript *dscript = swfdec_debugger_script_new (jscript, name, commands, n_commands);

  g_hash_table_insert (debugger->scripts, jscript, dscript);
  g_signal_emit (debugger, signals[SCRIPT_ADDED], 0, dscript);
}

void
swfdec_debugger_remove_script (SwfdecDebugger *debugger, JSScript *script)
{
  SwfdecDebuggerScript *dscript = g_hash_table_lookup (debugger->scripts, script);

  g_assert (dscript);
  g_signal_emit (debugger, signals[SCRIPT_REMOVED], 0, dscript);
  g_hash_table_remove (debugger->scripts, script);
}

typedef struct {
  GFunc func;
  gpointer data;
} ForeachData;

static void
do_foreach (gpointer key, gpointer value, gpointer data)
{
  ForeachData *fdata = data;

  fdata->func (value, fdata->data);
}

void
swfdec_debugger_foreach_script (SwfdecDebugger *debugger, GFunc func, gpointer data)
{
  ForeachData fdata = { func, data };
  g_hash_table_foreach (debugger->scripts, do_foreach, &fdata);
}

/*** BREAKPOINTS ***/

typedef struct {
  SwfdecDebuggerScript *	script;
  guint				line;
} Breakpoint;

static JSTrapStatus
swfdec_debugger_handle_breakpoint (JSContext *cx, JSScript *script, jsbytecode *pc,
    jsval *rval, void *closure)
{
  SwfdecDebugger *debugger = JS_GetContextPrivate (cx);

  g_signal_emit (debugger, signals[BREAKPOINT], 0, GPOINTER_TO_UINT (closure));
  return JSTRAP_CONTINUE;
}

guint
swfdec_debugger_set_breakpoint (SwfdecDebugger *debugger, 
    SwfdecDebuggerScript *script, guint line)
{
  guint i;
  Breakpoint *br = NULL;

  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), 0);
  g_return_val_if_fail (script != NULL, 0);
  g_return_val_if_fail (line < script->n_commands, 0);

  if (debugger->breakpoints == NULL)
    debugger->breakpoints = g_array_new (FALSE, FALSE, sizeof (Breakpoint));
  if (script->commands[line].breakpoint != 0)
    return script->commands[line].breakpoint;

  for (i = 0; i < debugger->breakpoints->len; i++) {
    br = &g_array_index (debugger->breakpoints, Breakpoint, i);
    if (br->script == NULL)
      break;
    br = NULL;
  }
  if (!JS_SetTrap (SWFDEC_PLAYER (debugger)->jscx, script->script,
	  script->commands[line].code, swfdec_debugger_handle_breakpoint, 
	  GUINT_TO_POINTER (i + 1)))
    return 0;

  if (br == NULL) {
    g_array_set_size (debugger->breakpoints, debugger->breakpoints->len + 1);
    br = &g_array_index (debugger->breakpoints, Breakpoint, 
	debugger->breakpoints->len - 1);
  }
  br->script = script;
  br->line = line;
  script->commands[line].breakpoint = i + 1;
  g_signal_emit (debugger, signals[BREAKPOINT_ADDED], 0, i + 1);
  return i + 1;
}

void
swfdec_debugger_unset_breakpoint (SwfdecDebugger *debugger, guint id)
{
  Breakpoint *br;

  g_return_if_fail (SWFDEC_IS_DEBUGGER (debugger));
  g_return_if_fail (id > 0);

  if (debugger->breakpoints == NULL)
    return;
  if (id > debugger->breakpoints->len)
    return;
  br = &g_array_index (debugger->breakpoints, Breakpoint, id - 1);
  if (br->script == NULL)
    return;

  JS_ClearTrap (SWFDEC_PLAYER (debugger)->jscx, br->script->script,
      br->script->commands[br->line].code, NULL, NULL);
  g_signal_emit (debugger, signals[BREAKPOINT_REMOVED], 0, id);
  br->script->commands[br->line].breakpoint = 0;
  br->script = NULL;
}

gboolean
swfdec_debugger_get_breakpoint (SwfdecDebugger *debugger, guint id, 
    SwfdecDebuggerScript **script, guint *line)
{
  Breakpoint *br;

  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), FALSE);
  g_return_val_if_fail (id > 0, FALSE);

  if (debugger->breakpoints == NULL)
    return FALSE;
  if (id > debugger->breakpoints->len)
    return FALSE;
  br = &g_array_index (debugger->breakpoints, Breakpoint, id - 1);
  if (br->script == NULL)
    return FALSE;

  if (script)
    *script = br->script;
  if (line)
    *line = br->line;
  return TRUE;
}

guint
swfdec_debugger_get_n_breakpoints (SwfdecDebugger *debugger)
{
  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), 0);

  if (debugger->breakpoints == NULL)
    return 0;

  return debugger->breakpoints->len;
}

