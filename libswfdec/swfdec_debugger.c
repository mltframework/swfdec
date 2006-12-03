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
  LAST_SIGNAL
};

G_DEFINE_TYPE (SwfdecDebugger, swfdec_debugger, G_TYPE_OBJECT)
guint signals [LAST_SIGNAL] = { 0, };

static void
swfdec_debugger_dispose (GObject *object)
{
  SwfdecDebugger *debugger = SWFDEC_DEBUGGER (object);

  g_assert (g_hash_table_size (debugger->scripts) == 0);
  g_hash_table_destroy (debugger->scripts);

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
}

static void
swfdec_debugger_init (SwfdecDebugger *debugger)
{
  debugger->scripts = g_hash_table_new_full (g_direct_hash, g_direct_equal, 
      NULL, (GDestroyNotify) swfdec_debugger_script_free);
}

static void
swfdec_player_attach_debugger (SwfdecPlayer *player)
{
  SwfdecDebugger *debugger;
  
  g_assert (player->roots == NULL);
  g_assert (player->debugger == NULL);

  debugger = g_object_new (SWFDEC_TYPE_DEBUGGER, NULL);
  SWFDEC_INFO ("attaching debugger %p to player %p", debugger, player);
  player->debugger = debugger;
  g_object_weak_ref (G_OBJECT (player), (GWeakNotify) g_object_unref, debugger);
}

/**
 * swfdec_debugger_get:
 * @player: a #SwfdecPlayer
 *
 * Gets the debugger associated with @player. A debugger will only be 
 * associated with @player, if it was created via 
 * swfdec_player_new_with_debugger().
 *
 * Returns: the debugger associated with @player or NULL if none
 **/
SwfdecDebugger *
swfdec_debugger_get (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  return player->debugger;
}

SwfdecPlayer *
swfdec_player_new_with_debugger (void)
{
  SwfdecPlayer *player = swfdec_player_new ();

  swfdec_player_attach_debugger (player);
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
