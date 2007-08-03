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

#include "swfdec_interaction.h"

static const GScannerConfig scanner_config = {
  (char *) ",; \t\n",
  (char *) G_CSET_a_2_z G_CSET_A_2_Z,
  (char *) G_CSET_a_2_z G_CSET_A_2_Z,
  (char *) "#\n",
  FALSE,
  FALSE, TRUE, FALSE, TRUE, TRUE, FALSE,
  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,
  TRUE, TRUE, TRUE, FALSE, FALSE,
  FALSE, TRUE, FALSE, FALSE,
  0
};

void
swfdec_interaction_free (SwfdecInteraction *inter)
{
  g_return_if_fail (inter != NULL);

  g_array_free (inter->commands, TRUE);
  g_free (inter);
}

void
swfdec_interaction_reset (SwfdecInteraction *inter)
{
  g_return_if_fail (inter != NULL);

  inter->mouse_x = 0;
  inter->mouse_y = 0;
  inter->mouse_button = 0;
  inter->cur_idx = 0;
  inter->time_elapsed = 0;
}

static void
swfdec_interaction_scanner_message (GScanner *scanner, gchar *message, gboolean error)
{
  if (!error)
    g_printerr ("warning: %s\n", message);
  g_set_error (scanner->user_data, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s", message);
}

static void
swfdec_command_append_mouse (SwfdecInteraction *inter, int x, int y, int button)
{
  SwfdecCommand command;

  command.command = SWFDEC_COMMAND_MOVE;
  command.args.mouse.x = x;
  command.args.mouse.y = y;
  command.args.mouse.button = button;
  inter->mouse_x = x;
  inter->mouse_y = y;
  inter->mouse_button = button;
  g_array_append_val (inter->commands, command);
}

static void
swfdec_command_append_key (SwfdecInteraction *inter, guint key, SwfdecCommandType type)
{
  SwfdecCommand command;

  command.command = type;
  command.args.key = key;
  g_array_append_val (inter->commands, command);
}

static void
swfdec_command_append_wait (SwfdecInteraction *inter, int msecs)
{
  SwfdecCommand command;

  command.command = SWFDEC_COMMAND_WAIT;
  command.args.time = msecs;
  g_array_append_val (inter->commands, command);
}

SwfdecInteraction *
swfdec_interaction_new (const char *data, guint length, GError **error)
{
  GScanner *scanner;
  GTokenType token;
  SwfdecInteraction *inter;
  int i, j;
  
  g_return_val_if_fail (data != NULL || length == 0, NULL);

  /* setup scanner */
  scanner = g_scanner_new (&scanner_config);
  scanner->user_data = error;
  scanner->msg_handler = swfdec_interaction_scanner_message;
  g_scanner_scope_add_symbol (scanner, 0, "wait", GINT_TO_POINTER (SWFDEC_COMMAND_WAIT));
  g_scanner_scope_add_symbol (scanner, 0, "move", GINT_TO_POINTER (SWFDEC_COMMAND_MOVE));
  g_scanner_scope_add_symbol (scanner, 0, "down", GINT_TO_POINTER (SWFDEC_COMMAND_DOWN));
  g_scanner_scope_add_symbol (scanner, 0, "up", GINT_TO_POINTER (SWFDEC_COMMAND_UP));
  g_scanner_scope_add_symbol (scanner, 0, "press", GINT_TO_POINTER (SWFDEC_COMMAND_PRESS));
  g_scanner_scope_add_symbol (scanner, 0, "release", GINT_TO_POINTER (SWFDEC_COMMAND_RELEASE));
  g_scanner_input_text (scanner, data, length);

  /* setup inter */
  inter = g_new0 (SwfdecInteraction, 1);
  inter->commands = g_array_new (FALSE, FALSE, sizeof (SwfdecCommand));

  while ((token = g_scanner_get_next_token (scanner)) != G_TOKEN_EOF) {
    switch (token) {
      case SWFDEC_COMMAND_WAIT:
	token = g_scanner_get_next_token (scanner);
	if (token != G_TOKEN_INT) {
	  g_scanner_unexp_token (scanner, G_TOKEN_INT, NULL, NULL, NULL, NULL, TRUE);
	  goto error;
	}
	i = scanner->value.v_int;
	swfdec_command_append_wait (inter, i);
	break;
      case SWFDEC_COMMAND_MOVE:
	token = g_scanner_get_next_token (scanner);
	if (token != G_TOKEN_INT) {
	  g_scanner_unexp_token (scanner, G_TOKEN_INT, NULL, NULL, NULL, NULL, TRUE);
	  goto error;
	}
	i = scanner->value.v_int;
	token = g_scanner_get_next_token (scanner);
	if (token != G_TOKEN_INT) {
	  g_scanner_unexp_token (scanner, G_TOKEN_INT, NULL, NULL, NULL, NULL, TRUE);
	  goto error;
	}
	j = scanner->value.v_int;
	swfdec_command_append_mouse (inter, i, j, inter->mouse_button);
	break;
      case SWFDEC_COMMAND_DOWN:
	swfdec_command_append_mouse (inter, inter->mouse_x, inter->mouse_y, 1);
	break;
      case SWFDEC_COMMAND_UP:
	swfdec_command_append_mouse (inter, inter->mouse_x, inter->mouse_y, 0);
	break;
      case SWFDEC_COMMAND_PRESS:
	token = g_scanner_get_next_token (scanner);
	if (token != G_TOKEN_INT) {
	  g_scanner_unexp_token (scanner, G_TOKEN_INT, NULL, NULL, NULL, NULL, TRUE);
	  goto error;
	}
	swfdec_command_append_key (inter, scanner->value.v_int, SWFDEC_COMMAND_PRESS);
	break;
      case SWFDEC_COMMAND_RELEASE:
	token = g_scanner_get_next_token (scanner);
	if (token != G_TOKEN_INT) {
	  g_scanner_unexp_token (scanner, G_TOKEN_INT, NULL, NULL, NULL, NULL, TRUE);
	  goto error;
	}
	swfdec_command_append_key (inter, scanner->value.v_int, SWFDEC_COMMAND_RELEASE);
	break;
      default:
	g_scanner_unexp_token (scanner, SWFDEC_COMMAND_WAIT, NULL, NULL, NULL, NULL, TRUE);
	goto error;
    }
  }
  swfdec_interaction_reset (inter);
  g_scanner_destroy (scanner);
  return inter;

error:
  swfdec_interaction_free (inter);
  g_scanner_destroy (scanner);
  return NULL;
}

SwfdecInteraction *
swfdec_interaction_new_from_file (const char *filename, GError **error)
{
  char *contents;
  gsize length;
  SwfdecInteraction *ret;

  g_return_val_if_fail (filename != NULL, NULL);

  if (!g_file_get_contents (filename, &contents, &length, error))
    return NULL;

  ret = swfdec_interaction_new (contents, length, error);
  g_free (contents);
  return ret;
}

/* returns time until next event in msecs or -1 if none */
int
swfdec_interaction_get_next_event (SwfdecInteraction *inter)
{
  SwfdecCommand *command;

  g_return_val_if_fail (inter != NULL, -1);

  if (inter->cur_idx >= inter->commands->len)
    return -1;
  command = &g_array_index (inter->commands, SwfdecCommand, inter->cur_idx);
  if (command->command != SWFDEC_COMMAND_WAIT)
    return 0;
  g_assert (command->args.time > inter->time_elapsed);
  return command->args.time - inter->time_elapsed;
}

void
swfdec_interaction_advance (SwfdecInteraction *inter, SwfdecPlayer *player, guint msecs)
{
  SwfdecCommand *command;

  g_return_if_fail (inter != NULL);

  inter->time_elapsed += msecs;
  while (inter->cur_idx < inter->commands->len) {
    command = &g_array_index (inter->commands, SwfdecCommand, inter->cur_idx);
    switch (command->command) {
      case SWFDEC_COMMAND_WAIT:
	if (inter->time_elapsed < command->args.time)
	  return;
	inter->time_elapsed -= command->args.time;
	break;
      case SWFDEC_COMMAND_MOVE:
	swfdec_player_handle_mouse (player, command->args.mouse.x, 
	    command->args.mouse.y, command->args.mouse.button);
	break;
      case SWFDEC_COMMAND_PRESS:
	swfdec_player_key_press (player, command->args.key);
	break;
      case SWFDEC_COMMAND_RELEASE:
	swfdec_player_key_release (player, command->args.key);
	break;
      case SWFDEC_COMMAND_UP:
      case SWFDEC_COMMAND_DOWN:
	/* these 2 get synthetisized into SWFDEC_COMMAND_MOVE */
      default:
	g_assert_not_reached ();
	return;
    }
    inter->cur_idx++;
  }
}

guint
swfdec_interaction_get_duration (SwfdecInteraction *inter)
{
  guint i, duration;

  g_return_val_if_fail (inter != NULL, 0);

  duration = 0;
  for (i = 0; i < inter->commands->len; i++) {
    SwfdecCommand *command = &g_array_index (inter->commands, SwfdecCommand, i);
    if (command->command == SWFDEC_COMMAND_WAIT)
      duration += command->args.time;
  }
  return duration;
}

