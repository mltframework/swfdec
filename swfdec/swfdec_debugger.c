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
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_interpret.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_movie.h"
#include "swfdec_player_internal.h"

enum {
  SCRIPT_ADDED,
  SCRIPT_REMOVED,
  BREAKPOINT,
  BREAKPOINT_ADDED,
  BREAKPOINT_REMOVED,
  MOVIE_ADDED,
  MOVIE_REMOVED,
  LAST_SIGNAL
};

G_DEFINE_TYPE (SwfdecDebugger, swfdec_debugger, SWFDEC_TYPE_PLAYER)
static guint signals[LAST_SIGNAL] = { 0, };

/*** SwfdecDebuggerScript ***/

typedef struct {
  guint			version;	/* version of parsed file */
  SwfdecConstantPool *	constant_pool;	/* current constant pool */
  GArray *		commands;	/* SwfdecDebuggerCommands parsed so far */
} ScriptParser;

static char *
swfdec_debugger_print_push (ScriptParser *parser, const guint8 *data, guint len)
{
  gboolean first = TRUE;
  SwfdecBits bits;
  GString *string = g_string_new ("Push");

  swfdec_bits_init_data (&bits, data, len);
  while (swfdec_bits_left (&bits)) {
    guint type = swfdec_bits_get_u8 (&bits);
    if (first)
      g_string_append (string, " ");
    else
      g_string_append (string, ", ");
    first = FALSE;
    switch (type) {
      case 0: /* string */
	{
	  char *s = swfdec_bits_get_string (&bits, parser->version);
	  if (!s)
	    goto error;
	  g_string_append_c (string, '"');
	  g_string_append (string, s);
	  g_string_append_c (string, '"');
	  g_free (s);
	  break;
	}
      case 1: /* float */
	g_string_append_printf (string, "%g", swfdec_bits_get_float (&bits));
	break;
      case 2: /* null */
	g_string_append (string, "null");
	break;
      case 3: /* undefined */
	g_string_append (string, "undefined");
	break;
      case 4: /* register */
	g_string_append_printf (string, "Register %u", swfdec_bits_get_u8 (&bits));
	break;
      case 5: /* boolean */
	g_string_append (string, swfdec_bits_get_u8 (&bits) ? "True" : "False");
	break;
      case 6: /* double */
	g_string_append_printf (string, "%g", swfdec_bits_get_double (&bits));
	break;
      case 7: /* 32bit int */
	g_string_append_printf (string, "%d", swfdec_bits_get_u32 (&bits));
	break;
      case 8: /* 8bit ConstantPool address */
      case 9: /* 16bit ConstantPool address */
	{
	  guint id;
	  const char *s;

	  if (!parser->constant_pool) {
	    SWFDEC_ERROR ("no constant pool");
	    goto error;
	  }
	  id = type == 8 ? swfdec_bits_get_u8 (&bits) : swfdec_bits_get_u16 (&bits);
	  if (id >= swfdec_constant_pool_size (parser->constant_pool)) {
	    SWFDEC_ERROR ("constant pool size too small");
	    goto error;
	  }
	  s = swfdec_constant_pool_get (parser->constant_pool, id);
	  g_string_append_c (string, '"');
	  g_string_append (string, s);
	  g_string_append_c (string, '"');
	}
	break;
      default:
	SWFDEC_ERROR ("Push: type %u not implemented", type);
	goto error;
    }
  }
  return g_string_free (string, FALSE);

error:
  g_string_free (string, TRUE);
  return g_strdup ("erroneous action Push");
}

/* NB: constant pool actions are special in that they are called at init time */
static gboolean
swfdec_debugger_add_command (gconstpointer bytecode, guint action, 
    const guint8 *data, guint len, gpointer parserp)
{
  ScriptParser *parser = parserp;
  SwfdecDebuggerCommand command;

  command.code = bytecode;
  if (action == SWFDEC_AS_ACTION_PUSH) {
    command.description = swfdec_debugger_print_push (parser, data, len);
  } else {
    command.description = swfdec_script_print_action (action, data, len);
  }
  g_assert (command.description != NULL);
  g_array_append_val (parser->commands, command);
  if (action == SWFDEC_AS_ACTION_CONSTANT_POOL) {
    if (parser->constant_pool)
      swfdec_constant_pool_free (parser->constant_pool);
    parser->constant_pool = swfdec_constant_pool_new_from_action (data, len, parser->version);
  }
  return TRUE;
}

static SwfdecDebuggerScript *
swfdec_debugger_script_new (SwfdecScript *script)
{
  ScriptParser parser;
  SwfdecDebuggerScript *ret;

  ret = g_new0 (SwfdecDebuggerScript, 1);
  ret->script = script;
  swfdec_script_ref (script);
  parser.commands = g_array_new (TRUE, FALSE, sizeof (SwfdecDebuggerCommand));
  parser.version = script->version;
  if (script->constant_pool) {
    parser.constant_pool = swfdec_constant_pool_new_from_action (
	script->constant_pool->data, script->constant_pool->length, parser.version);
  } else {
    parser.constant_pool = NULL;
  }
  swfdec_script_foreach (script, swfdec_debugger_add_command, &parser);
  ret->n_commands = parser.commands->len;
  ret->commands = (SwfdecDebuggerCommand *) g_array_free (parser.commands, FALSE);
  if (parser.constant_pool)
    swfdec_constant_pool_free (parser.constant_pool);

  return ret;
}

static void
swfdec_debugger_script_free (SwfdecDebuggerScript *script)
{
  guint i;

  g_assert (script);
  swfdec_script_unref (script->script);
  for (i = 0; i < script->n_commands; i++) {
    g_free (script->commands[i].description);
  }
  g_free (script->commands);
  g_free (script);
}

/*** BREAKPOINTS ***/

typedef enum {
  BREAKPOINT_NONE,
  BREAKPOINT_PC
} BreakpointType;

typedef struct {
  BreakpointType	type;
  const guint8 *	pc;
} Breakpoint;

static void
swfdec_debugger_update_interrupting (SwfdecDebugger *debugger)
{
  guint i;
  
  debugger->has_breakpoints = debugger->stepping;

  for (i = 0; i < debugger->breakpoints->len && !debugger->has_breakpoints; i++) {
    Breakpoint *br = &g_array_index (debugger->breakpoints, Breakpoint, i);

    if (br->type != BREAKPOINT_NONE)
      debugger->has_breakpoints = TRUE;
  }
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

  for (i = 0; i < debugger->breakpoints->len; i++) {
    br = &g_array_index (debugger->breakpoints, Breakpoint, i);
    if (br->type == BREAKPOINT_NONE)
      break;
    br = NULL;
  }

  if (br == NULL) {
    g_array_set_size (debugger->breakpoints, debugger->breakpoints->len + 1);
    br = &g_array_index (debugger->breakpoints, Breakpoint, 
	debugger->breakpoints->len - 1);
  }
  br->type = BREAKPOINT_PC;
  br->pc = script->commands[line].code;
  swfdec_debugger_update_interrupting (debugger);
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
  if (br->type == BREAKPOINT_NONE)
    return;

  g_signal_emit (debugger, signals[BREAKPOINT_REMOVED], 0, id);
  br->type = BREAKPOINT_NONE;
  swfdec_debugger_update_interrupting (debugger);
}

static gboolean
swfdec_debugger_get_from_as (SwfdecDebugger *debugger, SwfdecScript *script, 
    const guint8 *pc, SwfdecDebuggerScript **script_out, guint *line_out)
{
  guint line;
  SwfdecDebuggerScript *dscript = swfdec_debugger_get_script (debugger, script);

  if (dscript == NULL)
    return FALSE;
  for (line = 0; line < dscript->n_commands; line++) {
    if (pc == dscript->commands[line].code) {
      if (script_out)
	*script_out = dscript;
      if (line_out)
	*line_out = line;
      return TRUE;
    }
    if (pc < (guint8 *) dscript->commands[line].code)
      return FALSE;
  }
  return FALSE;
}

gboolean
swfdec_debugger_get_current (SwfdecDebugger *debugger,
    SwfdecDebuggerScript **dscript, guint *line)
{
  SwfdecAsFrame *frame;

  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), FALSE);

  frame = SWFDEC_AS_CONTEXT (debugger)->frame;
  if (frame == NULL)
    return FALSE;
  if (frame->script == NULL) /* native function */
    return FALSE;
  return swfdec_debugger_get_from_as (debugger, frame->script, frame->pc, dscript, line);
}

typedef struct {
  const guint8 *pc;
  SwfdecScript *current;
} BreakpointFinder;

static void
swfdec_debugger_find_script (gpointer key, gpointer value, gpointer data)
{
  BreakpointFinder *find = data;
  SwfdecScript *script = key;

  if (script->buffer->data > find->pc ||
      find->pc >= (script->buffer->data + script->buffer->length))
    return;
  if (find->current == NULL ||
      find->current->buffer->length > script->buffer->length)
    find->current = script;
}

gboolean
swfdec_debugger_get_breakpoint (SwfdecDebugger *debugger, guint id, 
    SwfdecDebuggerScript **script, guint *line)
{
  Breakpoint *br;
  BreakpointFinder find = { NULL, NULL };

  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), FALSE);
  g_return_val_if_fail (id > 0, FALSE);

  if (debugger->breakpoints == NULL)
    return FALSE;
  if (id > debugger->breakpoints->len)
    return FALSE;
  br = &g_array_index (debugger->breakpoints, Breakpoint, id - 1);
  if (br->type == BREAKPOINT_NONE)
    return FALSE;

  find.pc = br->pc;
  g_hash_table_foreach (debugger->scripts, swfdec_debugger_find_script, &find);
  if (find.current == NULL)
    return FALSE;
  return swfdec_debugger_get_from_as (debugger, find.current, find.pc, script, line);
}

guint
swfdec_debugger_get_n_breakpoints (SwfdecDebugger *debugger)
{
  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), 0);

  if (debugger->breakpoints == NULL)
    return 0;

  return debugger->breakpoints->len;
}

/*** SwfdecDebugger ***/

static void
swfdec_debugger_dispose (GObject *object)
{
  SwfdecDebugger *debugger = SWFDEC_DEBUGGER (object);

  swfdec_debugger_set_stepping (debugger, FALSE);
  g_assert (g_hash_table_size (debugger->scripts) == 0);
  g_hash_table_destroy (debugger->scripts);
  if (debugger->breakpoints)
    g_array_free (debugger->breakpoints, TRUE);

  G_OBJECT_CLASS (swfdec_debugger_parent_class)->dispose (object);
}

#if 0
static void
swfdec_debugger_do_breakpoint (SwfdecDebugger *debugger, guint id)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (debugger);

  GList *walk;
  for (walk = player->roots; walk; walk = walk->next) {
    swfdec_movie_update (walk->data);
  }
  g_object_thaw_notify (G_OBJECT (debugger));
  if (!swfdec_rect_is_empty (&player->invalid)) {
    double x, y, width, height;
    x = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.x0);
    y = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.y0);
    width = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.x1 - player->invalid.x0);
    height = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.y1 - player->invalid.y0);
    g_signal_emit_by_name (player, "invalidate", x, y, width, height);
    swfdec_rect_init_empty (&player->invalid);
  }

  g_signal_emit (debugger, signals[BREAKPOINT], 0, id);

  g_object_freeze_notify (G_OBJECT (debugger));
}

static void
swfdec_debugger_step (SwfdecAsContext *context)
{
  SwfdecDebugger *debugger = SWFDEC_DEBUGGER (context);

  if (context->state != SWFDEC_AS_CONTEXT_RUNNING)
    return;
  if (!debugger->has_breakpoints)
    return;

  if (debugger->stepping) {
    swfdec_debugger_do_breakpoint (debugger, 0);
  } else {
    guint i;
    Breakpoint *br;
    const guint8 *pc = context->frame->pc;

    for (i = 0; i < debugger->breakpoints->len; i++) {
      br = &g_array_index (debugger->breakpoints, Breakpoint, i);
      if (br->type == BREAKPOINT_NONE)
	continue;
      if (br->pc == pc) {
	swfdec_debugger_do_breakpoint (debugger, i + 1);
	return;
      }
    }
  }
}
#endif

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
  signals[MOVIE_ADDED] = g_signal_new ("movie-added", 
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, G_TYPE_OBJECT);
  signals[MOVIE_REMOVED] = g_signal_new ("movie-removed", 
      G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST, 0, NULL, NULL, 
      g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, G_TYPE_OBJECT);
}

static void
swfdec_debugger_init (SwfdecDebugger *debugger)
{
  debugger->scripts = g_hash_table_new_full (g_direct_hash, g_direct_equal, 
      NULL, (GDestroyNotify) swfdec_debugger_script_free);
  debugger->breakpoints = g_array_new (FALSE, FALSE, sizeof (Breakpoint));
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
swfdec_debugger_add_script (SwfdecDebugger *debugger, SwfdecScript *script)
{
  SwfdecDebuggerScript *dscript = swfdec_debugger_script_new (script);

  g_hash_table_insert (debugger->scripts, script, dscript);
  g_signal_emit (debugger, signals[SCRIPT_ADDED], 0, dscript);
}

SwfdecDebuggerScript *
swfdec_debugger_get_script (SwfdecDebugger *debugger, SwfdecScript *script)
{
  SwfdecDebuggerScript *dscript = g_hash_table_lookup (debugger->scripts, script);

  return dscript;
}

void
swfdec_debugger_remove_script (SwfdecDebugger *debugger, SwfdecScript *script)
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

void
swfdec_debugger_set_stepping (SwfdecDebugger *debugger, gboolean stepping)
{
  g_return_if_fail (SWFDEC_IS_DEBUGGER (debugger));

  if (debugger->stepping == stepping)
    return;
  debugger->stepping = stepping;
}

gboolean
swfdec_debugger_get_stepping (SwfdecDebugger *debugger)
{
  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), FALSE);

  return debugger->stepping;
}

const char *
swfdec_debugger_run (SwfdecDebugger *debugger, const char *command)
{
  SwfdecPlayer *player;
  GList *walk;

  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), NULL);
  g_return_val_if_fail (command != NULL, NULL);
  
  player = SWFDEC_PLAYER (debugger);
  g_object_freeze_notify (G_OBJECT (debugger));


  SWFDEC_ERROR ("ooops");


  for (walk = player->roots; walk; walk = walk->next) {
    swfdec_movie_update (walk->data);
  }
  if (!swfdec_rect_is_empty (&player->invalid)) {
    double x, y, width, height;
    x = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.x0);
    y = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.y0);
    width = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.x1 - player->invalid.x0);
    height = SWFDEC_TWIPS_TO_DOUBLE (player->invalid.y1 - player->invalid.y0);
    g_signal_emit_by_name (player, "invalidate", x, y, width, height);
    swfdec_rect_init_empty (&player->invalid);
  }
  g_object_thaw_notify (G_OBJECT (debugger));

  return command;
}

guint
swfdec_debugger_script_has_breakpoint (SwfdecDebugger *debugger, 
    SwfdecDebuggerScript *script, guint line)
{
  guint i;
  const guint8 *pc;

  g_return_val_if_fail (SWFDEC_IS_DEBUGGER (debugger), FALSE);
  g_return_val_if_fail (script != NULL, FALSE);
  g_return_val_if_fail (line < script->n_commands, FALSE);

  pc = script->commands[line].code;
  for (i = 0; i < debugger->breakpoints->len; i++) {
    Breakpoint *br = &g_array_index (debugger->breakpoints, Breakpoint, i);

    if (br->type == BREAKPOINT_PC && pc == br->pc)
      return i + 1;
  }
  return 0;
}
