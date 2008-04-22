/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include <swfdec/swfdec_as_interpret.h>
#include <swfdec/swfdec_bits.h>

#include "vivi_code_asm_function.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_function_resolve (ViviCodeEmitter *emitter, SwfdecBuffer *buffer,
    gsize offset, gpointer data, GError **error)
{
  ViviCodeAsmFunction *fun = VIVI_CODE_ASM_FUNCTION (data);
  gssize label_offset;
  gsize diff;

  label_offset = vivi_code_emitter_get_label_offset (emitter, fun->label);
  if (label_offset < 0) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_MISSING_LABEL,
	"no label \"%s\"", vivi_code_label_get_name (fun->label));
    return FALSE;
  }
  if ((gsize) label_offset < offset) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_INVALID_LABEL,
	"cannot jump backwards");
    return FALSE;
  }
  diff = label_offset - offset;
  if (diff > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"function body too big");
    return FALSE;
  }
  buffer->data[offset - 1] = diff >> 8;
  buffer->data[offset - 2] = diff;
  return TRUE;
}

static gboolean
vivi_code_asm_function_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  ViviCodeAsmFunction *fun = VIVI_CODE_ASM_FUNCTION (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);
  SwfdecBots *bots;
  guint i, len;

  bots = swfdec_bots_open ();
  swfdec_bots_put_string (bots, fun->name ? fun->name : "");
  len = fun->args ? g_strv_length (fun->args) : 0;
  g_assert (len <= G_MAXUINT16);
  swfdec_bots_put_u16 (bots, len);
  for (i = 0; i < len; i++) {
    swfdec_bots_put_string (bots, fun->args[i]);
  }
  swfdec_bots_put_u16 (bots, 0); /* function to be resolved later */
  
  len = swfdec_bots_get_bytes (bots);
  if (len > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"DeclareFunction action too big");
    swfdec_bots_free (bots);
    return FALSE;
  }

  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_DEFINE_FUNCTION);
  swfdec_bots_put_u16 (emit, len);
  swfdec_bots_put_bots (emit, bots);
  vivi_code_emitter_add_later (emitter, vivi_code_asm_function_resolve, fun);
  swfdec_bots_free (bots);

  return TRUE;
}

static void
vivi_code_asm_function_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_function_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmFunction, vivi_code_asm_function, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_function_asm_init))

static void
vivi_code_asm_function_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAsmFunction *function = VIVI_CODE_ASM_FUNCTION (token);
  guint i;

  vivi_code_printer_print (printer, "function ");
  if (function->name) {
    char *s = vivi_code_escape_string (function->name);
    vivi_code_printer_print (printer, s);
    vivi_code_printer_print (printer, " ");
    g_free (s);
  }
  if (function->args) {
    for (i = 0; function->args[i]; i++) {
      if (i == 0)
	vivi_code_printer_print (printer, "(");
      else
	vivi_code_printer_print (printer, ", ");
      /* FIXME: escape this? */
      vivi_code_printer_print (printer, function->args[i]);
    }
    if (i > 0)
      vivi_code_printer_print (printer, ") ");
  }
  vivi_code_printer_print (printer, vivi_code_label_get_name (function->label));
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_function_dispose (GObject *object)
{
  ViviCodeAsmFunction *function = VIVI_CODE_ASM_FUNCTION (object);

  g_strfreev (function->args);
  g_free (function->name);
  g_object_unref (function->label);

  G_OBJECT_CLASS (vivi_code_asm_function_parent_class)->dispose (object);
}

static void
vivi_code_asm_function_class_init (ViviCodeAsmFunctionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  object_class->dispose = vivi_code_asm_function_dispose;

  token_class->print = vivi_code_asm_function_print;

  code_class->bytecode = SWFDEC_AS_ACTION_DEFINE_FUNCTION;
}

static void
vivi_code_asm_function_init (ViviCodeAsmFunction *function)
{
}

ViviCodeAsm *
vivi_code_asm_function_new (ViviCodeLabel *label, const char *name,
    char *const *arguments)
{
  ViviCodeAsmFunction *function;

  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), NULL);

  function = g_object_new (VIVI_TYPE_CODE_ASM_FUNCTION, NULL);
  function->label = g_object_ref (label);
  function->name = g_strdup (name);
  function->args = g_strdupv ((char **) arguments);

  return VIVI_CODE_ASM (function);
}

ViviCodeLabel *
vivi_code_asm_function_get_label (ViviCodeAsmFunction *function)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION (function), NULL);

  return function->label;
}

const char *
vivi_code_asm_function_get_name (ViviCodeAsmFunction *function)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION (function), NULL);

  return function->name;
}

char * const *
vivi_code_asm_function_get_arguments (ViviCodeAsmFunction *function)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION (function), NULL);

  return function->args;
}
