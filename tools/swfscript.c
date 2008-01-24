/* Swfedit
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

#include <gtk/gtk.h>
#include "swfdec/swfdec_script_internal.h"
#include "swfdec_out.h"
#include "swfedit_file.h"

/* the stuff we look for */
guint *add_trace = NULL;

typedef gboolean ( *SwfeditTokenForeachFunc) (SwfeditToken *token, guint idx, 
    const char *name, SwfeditTokenType type, gconstpointer value, gpointer data);

static gboolean
swfedit_token_foreach (SwfeditToken *token, SwfeditTokenForeachFunc func, 
    gpointer data)
{
  SwfeditTokenEntry *entry;
  guint i;

  g_return_val_if_fail (SWFEDIT_IS_TOKEN (token), FALSE);
  g_return_val_if_fail (func != NULL, FALSE);

  for (i = 0; i < token->tokens->len; i++) {
    entry = &g_array_index (token->tokens, SwfeditTokenEntry, i);
    if (!func (token, i, entry->name, entry->type, entry->value, data))
      return FALSE;
    if (entry->type == SWFEDIT_TOKEN_OBJECT) {
      if (!swfedit_token_foreach (entry->value, func, data))
	return FALSE;
    }
  }
  return TRUE;
}

typedef struct {
  guint			offset;			/* offset in bytes from start of script */
  guint			new_offset;		/* new offset in bytes from start of script */
  guint			new_actions;		/* number of actions in new script */
} Action;

typedef struct {
  SwfdecScript *	script;			/* the original script */
  SwfdecOut *		out;			/* output for new script or NULL when buffer is set */
  SwfdecBuffer *	buffer;			/* buffer containing new script or NULL while constructing */
  GArray *		actions;		/* all actions in the script */
} State;

static gboolean
action_in_array (guint *array, guint action)
{
  if (array == NULL)
    return FALSE;
  while (*array != 0) {
    if (*array == action)
      return TRUE;
    array++;
  }
  return FALSE;
}

static guint
lookup_offset (GArray *array, guint offset)
{
  guint i;
  for (i = 0; i < array->len; i++) {
    Action *action = &g_array_index (array, Action, i);
    if (action->offset == offset)
      return action->new_offset;
  }
  g_assert_not_reached ();
  return 0;
}

static gboolean
fixup_jumps_foreach (gconstpointer bytecode, guint action,
        const guint8 *data, guint len, gpointer user_data)
{
  State *state = user_data;
    
  if (action == 0x99 || action == 0x9d) {
    guint offset = (guint8 *) bytecode - state->script->buffer->data;
    guint jump_offset = offset + 5 + GINT16_FROM_LE (*((gint16 *) data));
    offset = lookup_offset (state->actions, offset);
    jump_offset = lookup_offset (state->actions, jump_offset);
    *((gint16 *) &state->buffer->data[offset + 3]) = 
      GINT16_TO_LE (jump_offset - offset - 5);
  }
  if (action == 0x8a || action == 0x8d) {
    Action *cur = NULL; /* silence gcc */
    guint id = action == 0x8a ? 2 : 0;
    guint i, count;
    guint offset = (guint8 *) bytecode - state->script->buffer->data;
    for (i = 0; i < state->actions->len; i++) {
      cur = &g_array_index (state->actions, Action, i);
      if (cur->offset == offset) {
	offset = cur->new_offset;
	break;
      }
    }
    g_assert (i < state->actions->len);
    i = data[id];
    count = cur->new_actions - 1; /* FIXME: only works as long as we append actions */
    while (i > 0) {
      cur++;
      count += cur->new_actions;
      i--;
    }
    g_assert (count < 256);
    state->buffer->data[offset + 3 + id] = count;
  }
  return TRUE;
}

static gboolean
modify_script_foreach (gconstpointer bytecode, guint action,
        const guint8 *data, guint len, gpointer user_data)
{
  Action next;
  State *state = user_data;
    
  next.offset = (guint8 *) bytecode - state->script->buffer->data;
  next.new_offset = swfdec_out_get_bits (state->out) / 8;
  next.new_actions = 1;
  swfdec_out_put_u8 (state->out, action);
  if (action & 0x80) {
    swfdec_out_put_u16 (state->out, len);
    swfdec_out_put_data (state->out, data, len);
  }
  if (action_in_array (add_trace, action)) {
    swfdec_out_put_u8 (state->out, 0x4c); /* PushDuplicate */
    swfdec_out_put_u8 (state->out, 0x26); /* Trace */
    next.new_actions += 2;
  }
  g_array_append_val (state->actions, next);
  return TRUE;
}

static gboolean
modify_file (SwfeditToken *token, guint idx, const char *name, 
    SwfeditTokenType type, gconstpointer value, gpointer data)
{
  Action end;
  SwfdecScript *script;
  State state;

  if (type != SWFEDIT_TOKEN_SCRIPT)
    return TRUE;

  state.script = (SwfdecScript *) value;
  state.out = swfdec_out_open ();
  state.buffer = NULL;
  state.actions = g_array_new (FALSE, FALSE, sizeof (Action));
  swfdec_script_foreach (state.script, modify_script_foreach, &state);
  /* compute end offset */
  end.offset = g_array_index (state.actions, Action, state.actions->len - 1).offset;
  if (state.script->buffer->data[end.offset] & 0x80) {
    end.offset += GUINT16_FROM_LE (*((guint16* ) &state.script->buffer->data[end.offset + 1])) + 3;
  } else {
    end.offset++;
  }
  end.new_offset = swfdec_out_get_bits (state.out) / 8;
  end.new_actions = 0;
  g_array_append_val (state.actions, end);
#if 0
  {
    guint i;
    for (i = 0; i < state.actions->len; i++) {
      Action *action = &g_array_index (state.actions, Action, i);
      g_print ("%u  %u => %u  (%u actions)\n", i, action->offset, 
	  action->new_offset, action->new_actions);
    }
  }
#endif
  /* maybe append 0 byte */
  if (end.offset + 1 == state.script->buffer->length) {
    swfdec_out_put_u8 (state.out, 0);
  } else {
    g_assert (end.offset == state.script->buffer->length);
  }
  state.buffer = swfdec_out_close (state.out);
  state.out = NULL;
  swfdec_script_foreach (state.script, fixup_jumps_foreach, &state);
  g_array_free (state.actions, TRUE);
#if 0
  g_print ("got a new script in %u bytes - old script was %u bytes\n", 
      state.buffer->length, state.script->buffer->length);
#endif
  script = swfdec_script_new (state.buffer, state.script->name, state.script->version);
  g_assert (script);
  swfedit_token_set (token, idx, script);

  return TRUE;
}

static guint *
string_to_action_list (const char *list)
{
  char **actions = g_strsplit (list, ",", -1);
  guint *ret;
  guint i, len;

  len = g_strv_length (actions);
  ret = g_new (guint, len + 1);
  ret[len] = 0;
  for (i = 0; i < len; i++) {
    ret[i] = swfdec_action_get_from_name (actions[i]);
    if (ret[i] == 0) {
      g_printerr ("No such action \"%s\"\n", actions[i]);
      g_free (actions);
      g_free (ret);
      return NULL;
    }
  }
  g_free (actions);
  return ret;
}

int
main (int argc, char **argv)
{
  SwfeditFile *file;
  GError *error = NULL;
  char *add_trace_s = NULL;
  GOptionEntry options[] = {
    { "add-trace", 't', 0, G_OPTION_ARG_STRING, &add_trace_s, "list of actions to trace", "ACTION, ACTION" },
    { NULL }
  };
  GOptionContext *ctx;

  ctx = g_option_context_new ("");
  g_option_context_add_main_entries (ctx, options, "options");
  g_option_context_add_group (ctx, gtk_get_option_group (TRUE));
  g_option_context_parse (ctx, &argc, &argv, &error);
  g_option_context_free (ctx);
  if (error) {
    g_printerr ("error parsing arguments: %s\n", error->message);
    g_error_free (error);
    return 1;
  }

  if (argc < 2) {
    g_printerr ("Usage: %s FILENAME [OUTPUT-FILENAME]\n", argv[0]);
    return 1;
  }
  if (add_trace_s) {
    add_trace = string_to_action_list (add_trace_s);
    g_free (add_trace_s);
    if (add_trace == NULL)
      return 1;
  }
  file = swfedit_file_new (argv[1], &error);
  if (file == NULL) {
    g_printerr ("error opening file %s: %s\n", argv[1], error->message);
    g_error_free (error);
    return 1;
  }
  if (!swfedit_token_foreach (SWFEDIT_TOKEN (file), modify_file, NULL)) {
    g_printerr ("modifying file %s failed.\n", argv[1]);
    g_object_unref (file);
    return 1;
  }
  g_free (file->filename);
  if (argc > 2) {
    file->filename = g_strdup (argv[2]);
  } else {
    file->filename = g_strdup_printf ("%s.out.swf", argv[1]);
  }
  if (!swfedit_file_save (file, &error)) {
    g_printerr ("Error saving file: %s\n", error->message);
    g_error_free (error);
  }
  g_print ("saved modified file to %s\n", file->filename);
  g_object_unref (file);
  return 0;
}

