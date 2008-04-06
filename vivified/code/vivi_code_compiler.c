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

  if (compiler->data)
    swfdec_buffer_unref (swfdec_out_close (compiler->data));

  for (iter = compiler->actions; iter != NULL; iter = iter->next) {
    swfdec_buffer_unref (
	swfdec_out_close (((ViviCodeCompilerAction *)iter->data)->data));
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
  compiler->data = swfdec_out_open ();
}

ViviCodeCompiler *
vivi_code_compiler_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_COMPILER, NULL);
}

SwfdecBuffer *
vivi_code_compiler_get_data (ViviCodeCompiler *compiler)
{
  SwfdecBuffer *buffer;

  g_return_val_if_fail (VIVI_IS_CODE_COMPILER (compiler), NULL);
  g_return_val_if_fail (compiler->action == NULL, NULL);

  buffer = swfdec_out_close (compiler->data);
  compiler->data = swfdec_out_open ();

  return buffer;
}

void
vivi_code_compiler_compile_token (ViviCodeCompiler *compiler,
    ViviCodeToken *token)
{
  ViviCodeTokenClass *klass;
  ViviCodeCompilerAction *action;

  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (VIVI_IS_CODE_TOKEN (token));

  action = compiler->action;

  klass = VIVI_CODE_TOKEN_GET_CLASS (token);
  g_return_if_fail (klass->compile);
  klass->compile (token, compiler);

  g_assert (compiler->action == action);
}

void
vivi_code_compiler_compile_value (ViviCodeCompiler *compiler,
    ViviCodeValue *value)
{
  vivi_code_compiler_compile_token (compiler, VIVI_CODE_TOKEN (value));
}


void
vivi_code_compiler_add_action (ViviCodeCompiler *compiler,
    SwfdecAsAction action)
{
  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));

  if (compiler->action != NULL)
    compiler->actions = g_slist_prepend (compiler->actions, compiler->action);

  compiler->action = g_new0 (ViviCodeCompilerAction, 1);
  compiler->action->id = action;
  compiler->action->data = swfdec_out_open ();

  g_print ("ADD ACTION %i\n", action);
}

void
vivi_code_compiler_end_action (ViviCodeCompiler *compiler)
{
  SwfdecBuffer *buffer;

  g_return_if_fail (VIVI_IS_CODE_COMPILER (compiler));
  g_return_if_fail (compiler->action != NULL);

  g_print ("END ACTION %i\n", compiler->action->id);

  buffer = swfdec_out_close (compiler->action->data);

  g_assert (compiler->action->id & 0x80 || buffer->length == 0);

  swfdec_out_put_u8 (compiler->data, compiler->action->id);
  if (compiler->action->id & 0x80) {
    swfdec_out_put_u16 (compiler->data, buffer->length);
    swfdec_out_put_buffer (compiler->data, buffer);
  }
  swfdec_buffer_unref (buffer);

  g_free (compiler->action);

  if (compiler->actions != NULL) {
    compiler->action = compiler->actions->data;
    compiler->actions =
      g_slist_delete_link (compiler->actions, compiler->actions);
  } else {
    compiler->action = NULL;
  }
}

void vivi_code_compiler_write_u8 (ViviCodeCompiler *compiler, guint value)
{
  g_print ("write_u8 %i\n", value);

  swfdec_out_put_u8 (compiler->action->data, value);
}

void
vivi_code_compiler_write_double (ViviCodeCompiler *compiler, double value)
{
  g_print ("write_double %g\n", value);
}

void
vivi_code_compiler_write_string (ViviCodeCompiler *compiler, const char *value)
{
  g_print ("write_string %s\n", value);

  swfdec_out_put_string (compiler->action->data, value);
}
