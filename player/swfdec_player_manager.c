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

#include <stdlib.h>
#include <string.h>
#include <libswfdec/swfdec_debugger.h>
#include <libswfdec/swfdec_js.h>
#include <libswfdec/js/jsdbgapi.h>
#include <libswfdec/js/jsinterp.h>
#include "swfdec_player_manager.h"
#include "swfdec_source.h"

enum {
  PROP_0,
  PROP_PLAYING,
  PROP_SPEED,
  PROP_INTERRUPTED
};

enum {
  MESSAGE,
  LAST_SIGNAL
};

G_DEFINE_TYPE (SwfdecPlayerManager, swfdec_player_manager, G_TYPE_OBJECT)
guint signals[LAST_SIGNAL];

static void
swfdec_player_manager_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecPlayerManager *manager = SWFDEC_PLAYER_MANAGER (object);
  
  switch (param_id) {
    case PROP_PLAYING:
      g_value_set_boolean (value, swfdec_player_manager_get_playing (manager));
      break;
    case PROP_SPEED:
      g_value_set_double (value, swfdec_player_manager_get_speed (manager));
      break;
    case PROP_INTERRUPTED:
      g_value_set_boolean (value, swfdec_player_manager_get_interrupted (manager));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_player_manager_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecPlayerManager *manager = SWFDEC_PLAYER_MANAGER (object);

  switch (param_id) {
    case PROP_PLAYING:
      swfdec_player_manager_set_playing (manager, g_value_get_boolean (value));
      break;
    case PROP_SPEED:
      swfdec_player_manager_set_speed (manager, g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_player_manager_dispose (GObject *object)
{
  SwfdecPlayerManager *manager = SWFDEC_PLAYER_MANAGER (object);

  swfdec_player_manager_set_playing (manager, FALSE);
  g_object_unref (manager->player);

  G_OBJECT_CLASS (swfdec_player_manager_parent_class)->dispose (object);
}

static void
swfdec_player_manager_class_init (SwfdecPlayerManagerClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);

  object_class->dispose = swfdec_player_manager_dispose;
  object_class->set_property = swfdec_player_manager_set_property;
  object_class->get_property = swfdec_player_manager_get_property;

  g_object_class_install_property (object_class, PROP_SPEED,
      g_param_spec_double ("speed", "speed", "playback speed of movie",
	  G_MINDOUBLE, 16.0, 1.0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PLAYING,
      g_param_spec_boolean ("playing", "playing", "if the movie is played back",
	  FALSE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_INTERRUPTED,
      g_param_spec_boolean ("interrupted", "interrupted", "TRUE if we're handling a breakpoint",
	  FALSE, G_PARAM_READABLE));

  signals[MESSAGE] = g_signal_new ("message", G_TYPE_FROM_CLASS (g_class),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__UINT_POINTER, /* FIXME */
      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);
}

static void
swfdec_player_manager_init (SwfdecPlayerManager *manager)
{
  manager->speed = 1.0;
}

static void breakpoint_hit_cb (SwfdecDebugger *debugger, guint id, SwfdecPlayerManager *manager);

static void
swfdec_player_manager_set_player (SwfdecPlayerManager *manager, SwfdecPlayer *player)
{
  if (manager->player == player)
    return;

  if (manager->player) {
    g_signal_handlers_disconnect_by_func (manager->player, breakpoint_hit_cb, manager);
    g_object_unref (manager->player);
  }
  manager->player = player;
  if (player) {
    g_object_ref (player);
    g_signal_connect (player, "breakpoint", G_CALLBACK (breakpoint_hit_cb), manager);
  }
}

SwfdecPlayerManager *
swfdec_player_manager_new (SwfdecPlayer *player)
{
  SwfdecPlayerManager *manager;
  
  g_return_val_if_fail (player == NULL || SWFDEC_IS_PLAYER (player), NULL);

  manager = g_object_new (SWFDEC_TYPE_PLAYER_MANAGER, 0);
  swfdec_player_manager_set_player (manager, player);

  return manager;
}

void
swfdec_player_manager_set_speed (SwfdecPlayerManager *manager, double speed)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (speed > 0.0);

  if (manager->speed == speed)
    return;
  if (manager->source) {
    swfdec_player_manager_set_playing (manager, FALSE);
    manager->speed = speed;
    swfdec_player_manager_set_playing (manager, TRUE);
  } else {
    manager->speed = speed;
  }
  g_object_notify (G_OBJECT (manager), "speed");
}

double		
swfdec_player_manager_get_speed (SwfdecPlayerManager *manager)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), 1.0);

  return manager->speed;
}

gboolean
swfdec_player_manager_get_interrupted (SwfdecPlayerManager *manager)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), FALSE);

  return manager->interrupt_loop != NULL;
}

static void
swfdec_player_manager_update_playing (SwfdecPlayerManager *manager)
{
  gboolean should_have_source = manager->playing && 
    !swfdec_player_manager_get_interrupted (manager);

  if (should_have_source && manager->source == NULL) {
    manager->source = swfdec_iterate_source_new (manager->player);
    g_source_attach (manager->source, NULL);
  } else if (!should_have_source && manager->source != NULL) {
    g_source_destroy (manager->source);
    g_source_unref (manager->source);
    manager->source = NULL;
  }
}

void
swfdec_player_manager_set_playing (SwfdecPlayerManager *manager, gboolean playing)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));

  if (manager->playing == playing)
    return;
  manager->playing = playing;
  swfdec_player_manager_update_playing (manager);
  g_object_notify (G_OBJECT (manager), "playing");
}

gboolean
swfdec_player_manager_get_playing (SwfdecPlayerManager *manager)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager), FALSE);

  return manager->playing;
}

void
swfdec_player_manager_iterate (SwfdecPlayerManager *manager)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (!swfdec_player_manager_get_interrupted (manager));

  swfdec_player_manager_set_playing (manager, FALSE);
  swfdec_player_iterate (manager->player);
}

void
swfdec_player_manager_continue (SwfdecPlayerManager *manager)
{
  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (swfdec_player_manager_get_interrupted (manager));

  g_main_loop_quit (manager->interrupt_loop);
}

/*** command handling ***/

typedef enum {
  SWFDEC_MESSAGE_INPUT,
  SWFDEC_MESSAGE_OUTPUT,
  SWFDEC_MESSAGE_ERROR
} SwfdecMessageType;

static void
swfdec_player_manager_send_message (SwfdecPlayerManager *manager,
    SwfdecMessageType type, char *format, ...) G_GNUC_PRINTF (3, 4);
static void
swfdec_player_manager_send_message (SwfdecPlayerManager *manager,
    SwfdecMessageType type, char *format, ...)
{
  va_list args;
  char *msg;

  va_start (args, format);
  msg = g_strdup_vprintf (format, args);
  va_end (args);
  g_signal_emit (manager, signals[MESSAGE], 0, (guint) type, msg);
  g_free (msg);
}
#define swfdec_player_manager_output(manager, ...) \
  swfdec_player_manager_send_message (manager, SWFDEC_MESSAGE_OUTPUT, __VA_ARGS__)
#define swfdec_player_manager_error(manager, ...) \
  swfdec_player_manager_send_message (manager, SWFDEC_MESSAGE_ERROR, __VA_ARGS__)

static void
breakpoint_hit_cb (SwfdecDebugger *debugger, guint id, SwfdecPlayerManager *manager)
{
  g_object_ref (manager);
  manager->interrupt_loop = g_main_loop_new (NULL, FALSE);
  swfdec_player_manager_update_playing (manager);
  g_object_notify (G_OBJECT (manager), "interrupted");
  swfdec_player_manager_output (manager, "Breakpoint %u", id);
  g_main_loop_run (manager->interrupt_loop);
  g_main_loop_unref (manager->interrupt_loop);
  manager->interrupt_loop = NULL;
  g_object_notify (G_OBJECT (manager), "interrupted");
  swfdec_player_manager_update_playing (manager);
  g_object_unref (manager);
}

static void
command_print (SwfdecPlayerManager *manager, const char *arg)
{
  jsval rval;

  if (swfdec_js_run (manager->player, arg, &rval)) {
    const char *s;
    s = swfdec_js_to_string (manager->player->jscx, rval);
    if (s)
      swfdec_player_manager_output (manager, "%s", s);
    else
      swfdec_player_manager_error (manager, "Invalid return value");
  } else {
    swfdec_player_manager_error (manager, "Invalid command");
  }
}

static void
command_play (SwfdecPlayerManager *manager, const char *arg)
{
  swfdec_player_manager_set_playing (manager, TRUE);
}

static void
command_stop (SwfdecPlayerManager *manager, const char *arg)
{
  swfdec_player_manager_set_playing (manager, FALSE);
}

static void
command_iterate (SwfdecPlayerManager *manager, const char *arg)
{
  swfdec_player_manager_iterate (manager);
}

static void
command_continue (SwfdecPlayerManager *manager, const char *arg)
{
  if (swfdec_player_manager_get_interrupted (manager))
    swfdec_player_manager_continue (manager);
  else
    swfdec_player_manager_error (manager, "Not interrupted, cannot continue");
}

static void
command_breakpoints (SwfdecPlayerManager *manager, const char *arg)
{
  guint i, n, line;
  SwfdecDebugger *debugger;
  SwfdecDebuggerScript *script;

  debugger = SWFDEC_DEBUGGER (manager->player);
  n = swfdec_debugger_get_n_breakpoints (debugger);
  for (i = 1; i <= n; i++) {
    if (swfdec_debugger_get_breakpoint (debugger, i, &script, &line)) {
      swfdec_player_manager_output (manager, "%u: %s line %u: %s",
	  i, script->name, line, script->commands[line].description);
    }
  }
}

static void
command_delete (SwfdecPlayerManager *manager, const char *arg)
{
  char *end;
  guint id;

  id = strtoul (arg, &end, 10);
  if (id == 0 || *end != '\0') {
    swfdec_player_manager_error (manager, "no breakpoint '%s'", arg);
    return;
  }
  swfdec_debugger_unset_breakpoint (SWFDEC_DEBUGGER (manager->player), id);
}

static void
command_stack (SwfdecPlayerManager *manager, const char *arg)
{
  JSStackFrame *frame = NULL;
  guint i, min, max;

  if (!swfdec_player_manager_get_interrupted (manager)) {
    swfdec_player_manager_error (manager, "Not interrupted");
    return;
  }
  JS_FrameIterator (manager->player->jscx, &frame);
  min = 1;
  max = frame->sp - frame->spbase;
  for (i = min; i <= max; i++) {
    const char *s = swfdec_js_to_string (manager->player->jscx, frame->sp[-i]);
    swfdec_player_manager_output (manager, "%2u: %s", i, s);
  }
}

static void command_help (SwfdecPlayerManager *manager, const char *arg);
/* NB: the first word in the command string is used, partial matches are ok */
struct {
  const char *	name;
  void		(* func)	(SwfdecPlayerManager *manager, const char *arg);
  const char *	description;
} commands[] = {
  { "help",	command_help,		"print all available commands and a quick description" },
  { "print",	command_print,	  	"evaluate the argument as a JavaScript script" },
  { "play",	command_play,		"play the movie" },
  { "stop",	command_stop,	 	"stop the movie" },
  { "iterate",	command_iterate,	"iterate the movie once" },
  { "breakpoints", command_breakpoints,	"show all breakpoints" },
  { "delete",	command_delete,		"delete a breakpoint" },
  { "continue",	command_continue,	"continue when stopped inside a breakpoint" },
  { "stack",	command_stack,		"print the first arguments on the stack" },
};

static void
command_help (SwfdecPlayerManager *manager, const char *arg)
{
  guint i;
  swfdec_player_manager_output (manager, "The following commands are available:");
  for (i = 0; i < G_N_ELEMENTS (commands); i++) {
    swfdec_player_manager_output (manager, "%s: %s", commands[i].name, commands[i].description);
  }
}

void
swfdec_player_manager_execute (SwfdecPlayerManager *manager, const char *command)
{
  guint i;
  const char *space;

  g_return_if_fail (SWFDEC_IS_PLAYER_MANAGER (manager));
  g_return_if_fail (command != NULL);

  swfdec_player_manager_send_message (manager, SWFDEC_MESSAGE_INPUT, "%s", command);
  space = strchr (command, ' ');
  if (space == NULL)
    space = command + strlen (command);
  for (i = 0; i < G_N_ELEMENTS(commands); i++) {
    if (g_ascii_strncasecmp (commands[i].name, command, space - command) == 0) {
      commands[i].func (manager, *space == '\0' ? NULL : space + 1);
      return;
    }
  }
  swfdec_player_manager_error (manager, "No such command '%*s'", space - command, command);
}

