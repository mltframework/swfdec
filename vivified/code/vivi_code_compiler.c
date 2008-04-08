/* Vivified
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "vivi_code_compiler.h"

G_DEFINE_TYPE (ViviCodeCompiler, vivi_code_compiler, G_TYPE_OBJECT)

static void
vivi_code_compiler_dispose (GObject *object)
{
  ViviCodeCompiler *compiler = VIVI_CODE_COMPILER (object);
  GSList *iter;

  for (iter = compiler->actions; iter != NULL; iter = iter->next) {
    if (((ViviCodeCompilerAction *)iter->data)->data != NULL) {
      swfdec_buffer_unref (
	  swfdec_bots_close (((ViviCodeCompilerAction *)iter->data)->data));
    }
  }

  g_slist_free (compiler->actions);

  G_OBJECT_CLASS (vivi_code_compiler_parent_class)->dispose (object);
}

static void
vivi_code_compiler_class_init (ViviCodeCompilerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_code_compiler_dispose;
}

static void
vivi_code_compiler_init (ViviCodeCompiler *compiler)
{
}

ViviCodeCompiler *
vivi_code_compiler_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_COMPILER, NULL);
}

void
vivi_code_compiler_compile_token (ViviCodeCompiler *compiler,
    ViviCodeToken *token)
{
  ViviCodeTokenClass *klass;
  ViviCodeCompilerAction *action;
  GSList *current;

  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (VIVI_IS_CODE_TOKEN (token));

  current = compiler->current;
  action = compiler->action;

  klass = VIVI_CODE_TOKEN_GET_CLASS (token);
  g_return_if_fail (klass->compile);
  klass->compile (token, compiler);

  g_assert (compiler->current == current);
  g_assert (compiler->action == action);
}

void
vivi_code_compiler_compile_value (ViviCodeCompiler *compiler,
    ViviCodeValue *value)
{
  vivi_code_compiler_compile_token (compiler, VIVI_CODE_TOKEN (value));
}

static gsize
vivi_code_compiler_action_get_size (ViviCodeCompilerAction *action)
{
  g_return_val_if_fail (action != NULL, 0);

  if (action->id & 0x80) {
    return 3 +
      (action->data != NULL ? swfdec_bots_get_bytes (action->data) : 0);
  } else {
    g_assert (action->data == NULL ||
	swfdec_bots_get_bytes (action->data) == 0);
    return 1;
  }
}

static void
vivi_code_compiler_action_write_data (ViviCodeCompilerAction *action,
    SwfdecBots *data)
{
  g_return_if_fail (action != NULL);
  g_return_if_fail (data != NULL);

  swfdec_bots_put_u8 (data, action->id);
  if (action->id & 0x80) {
    if (action->data == NULL) {
      swfdec_bots_put_u16 (data, 0);
    } else {
      swfdec_bots_put_u16 (data, swfdec_bots_get_bytes (action->data));
      swfdec_bots_put_bots (data, action->data);
    }
  } else {
    g_assert (action->data == NULL ||
	swfdec_bots_get_bytes (action->data) == 0);
  }
}

SwfdecBuffer *
vivi_code_compiler_get_data (ViviCodeCompiler *compiler)
{
  SwfdecBots *data;
  GSList *iter;

  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), NULL);
  g_return_val_if_fail (compiler->current == NULL, NULL);

  data = swfdec_bots_open ();
  compiler->actions = g_slist_reverse (compiler->actions);
  for (iter = compiler->actions; iter != NULL; iter = iter->next) {
    vivi_code_compiler_action_write_data (iter->data, data);
  }
  compiler->actions = g_slist_reverse (compiler->actions);
  return swfdec_bots_close (data);
}

gsize
vivi_code_compiler_tail_size (ViviCodeCompiler *compiler)
{
  GSList *iter;
  gsize size;

  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), 0);

  size = 0;
  for (iter = compiler->actions; iter != compiler->current; iter = iter->next)
  {
    size += vivi_code_compiler_action_get_size (iter->data);
  }
  return size;
}

void
vivi_code_compiler_begin_action (ViviCodeCompiler *compiler,
    SwfdecAsAction action)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));

  compiler->action = g_new0 (ViviCodeCompilerAction, 1);
  compiler->action->id = action;
  compiler->action->data = swfdec_bots_open ();
  compiler->action->parent = compiler->current;

  compiler->actions = g_slist_prepend (compiler->actions, compiler->action);
  compiler->current = compiler->actions;
}

void
vivi_code_compiler_end_action (ViviCodeCompiler *compiler)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (compiler->current != NULL);

  compiler->current = compiler->action->parent;
  if (compiler->current != NULL) {
    compiler->action = compiler->current->data;
  } else {
    compiler->action = NULL;
  }
}

void
vivi_code_compiler_write_empty_action (ViviCodeCompiler *compiler,
    SwfdecAsAction id)
{
  ViviCodeCompilerAction *action;

  action = g_new0 (ViviCodeCompilerAction, 1);
  action->id = id;
  action->data = NULL;
  action->parent = NULL;

  compiler->actions = g_slist_prepend (compiler->actions, action);
}

void vivi_code_compiler_write_u8 (ViviCodeCompiler *compiler, guint value)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (compiler->action != NULL);

  swfdec_bots_put_u8 (compiler->action->data, value);
}

void vivi_code_compiler_write_u16 (ViviCodeCompiler *compiler, guint value)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (compiler->action != NULL);

  swfdec_bots_put_u16 (compiler->action->data, value);
}

void vivi_code_compiler_write_s16 (ViviCodeCompiler *compiler, guint value)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (compiler->action != NULL);

  swfdec_bots_put_s16 (compiler->action->data, value);
}

void
vivi_code_compiler_write_double (ViviCodeCompiler *compiler, double value)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (compiler->action != NULL);

  swfdec_bots_put_double (compiler->action->data, value);
}

void
vivi_code_compiler_write_string (ViviCodeCompiler *compiler, const char *value)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (compiler->action != NULL);

  swfdec_bots_put_string (compiler->action->data, value);
}
