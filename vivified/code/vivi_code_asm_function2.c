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
#include <swfdec/swfdec_script_internal.h>

#include "vivi_code_asm_function2.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_function2_resolve (ViviCodeEmitter *emitter, SwfdecBuffer *buffer,
    gsize offset, gpointer data, GError **error)
{
  ViviCodeAsmFunction2 *fun = VIVI_CODE_ASM_FUNCTION2 (data);
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
vivi_code_asm_function2_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  ViviCodeAsmFunction2 *fun = VIVI_CODE_ASM_FUNCTION2 (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);
  SwfdecBots *bots;
  guint i, len;

  bots = swfdec_bots_open ();
  swfdec_bots_put_string (bots, fun->name ? fun->name : "");
  swfdec_bots_put_u16 (bots, fun->args->len);
  swfdec_bots_put_u8 (bots, fun->n_registers);
  swfdec_bots_put_u16 (bots, fun->flags);
  for (i = 0; i < fun->args->len; i++) {
    SwfdecScriptArgument *arg = &g_array_index (fun->args, SwfdecScriptArgument, i);
    swfdec_bots_put_u8 (bots, arg->preload);
    swfdec_bots_put_string (bots, arg->name);
  }
  swfdec_bots_put_u16 (bots, 0); /* length of body, to be resolved later */
  
  len = swfdec_bots_get_bytes (bots);
  if (len > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"DeclareFunction2 action too big");
    swfdec_bots_free (bots);
    return FALSE;
  }

  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_DEFINE_FUNCTION2);
  swfdec_bots_put_u16 (emit, len);
  swfdec_bots_put_bots (emit, bots);
  vivi_code_emitter_add_later (emitter, vivi_code_asm_function2_resolve, fun);
  swfdec_bots_free (bots);

  return TRUE;
}

static void
vivi_code_asm_function2_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_function2_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmFunction2, vivi_code_asm_function2, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_function2_asm_init))

/* FIXME: export for compiler */
static const char *flag_names[16] = {
  "preload_this",
  "suppress_this",
  "preload_args",
  "suppress_args",
  "preload_super",
  "suppress_super",
  "preload_root",
  "preload_parent",
  "preload_global",
  "reserved1",
  "reserved2",
  "reserved3",
  "reserved4",
  "reserved5",
  "reserved6",
  "reserved7"
};

static void
vivi_code_asm_function2_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAsmFunction2 *fun = VIVI_CODE_ASM_FUNCTION2 (token);
  guint i;

  vivi_code_printer_print (printer, "function2 ");
  if (fun->name) {
    char *s = vivi_code_escape_string (fun->name);
    vivi_code_printer_print (printer, s);
    vivi_code_printer_print (printer, " ");
    g_free (s);
  }
  if (fun->n_registers) {
    char *s = g_strdup_printf ("%u", fun->n_registers);
    vivi_code_printer_print (printer, s);
    vivi_code_printer_print (printer, " ");
    g_free (s);
  }
  for (i = 0; i < G_N_ELEMENTS (flag_names); i++) {
    if (fun->flags & (1 << i)) {
      vivi_code_printer_print (printer, flag_names[i]);
      vivi_code_printer_print (printer, " ");
    }
  }
  for (i = 0; i < fun->args->len; i++) {
    SwfdecScriptArgument *arg = &g_array_index (fun->args, SwfdecScriptArgument, i);
    if (i == 0)
      vivi_code_printer_print (printer, "(");
    else
      vivi_code_printer_print (printer, ", ");
    /* FIXME: escape this? */
    vivi_code_printer_print (printer, arg->name);
    if (arg->preload) {
      char *s = g_strdup_printf ("%u", arg->preload);
      vivi_code_printer_print (printer, " ");
      vivi_code_printer_print (printer, s);
      g_free (s);
    }
  }
  if (i > 0)
    vivi_code_printer_print (printer, ") ");
  vivi_code_printer_print (printer, vivi_code_label_get_name (fun->label));
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_function2_dispose (GObject *object)
{
  ViviCodeAsmFunction2 *fun = VIVI_CODE_ASM_FUNCTION2 (object);
  guint i;

  for (i = 0; i < fun->args->len; i++) {
    g_free (g_array_index (fun->args, SwfdecScriptArgument, i).name);
  }
  g_array_free (fun->args, TRUE);
  g_free (fun->name);
  g_object_unref (fun->label);

  G_OBJECT_CLASS (vivi_code_asm_function2_parent_class)->dispose (object);
}

static void
vivi_code_asm_function2_class_init (ViviCodeAsmFunction2Class *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  object_class->dispose = vivi_code_asm_function2_dispose;

  token_class->print = vivi_code_asm_function2_print;

  code_class->bytecode = SWFDEC_AS_ACTION_DEFINE_FUNCTION2;
}

static void
vivi_code_asm_function2_init (ViviCodeAsmFunction2 *fun)
{
  fun->args = g_array_new (FALSE, FALSE, sizeof (SwfdecScriptArgument));
}

ViviCodeAsm *
vivi_code_asm_function2_new (ViviCodeLabel *label, const char *name,
    guint n_registers, guint flags)
{
  ViviCodeAsmFunction2 *fun;

  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), NULL);
  g_return_val_if_fail (n_registers < 256, NULL);
  g_return_val_if_fail (flags <= G_MAXUINT16, NULL);

  fun= g_object_new (VIVI_TYPE_CODE_ASM_FUNCTION2, NULL);
  fun->label = g_object_ref (label);
  fun->name = g_strdup (name);
  fun->n_registers = n_registers;
  fun->flags = flags;

  return VIVI_CODE_ASM (fun);
}

ViviCodeLabel *
vivi_code_asm_function2_get_label (ViviCodeAsmFunction2 *fun)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION2 (fun), NULL);

  return fun->label;
}

void
vivi_code_asm_function2_set_label (ViviCodeAsmFunction2 *fun, ViviCodeLabel *label)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_FUNCTION2 (fun));
  g_return_if_fail (VIVI_IS_CODE_LABEL (label));

  g_object_ref (label);
  g_object_unref (fun->label);
  fun->label = label;
}

const char *
vivi_code_asm_function2_get_name (ViviCodeAsmFunction2 *fun)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION2 (fun), NULL);

  return fun->name;
}

guint
vivi_code_asm_function2_get_flags (ViviCodeAsmFunction2 *fun)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION2 (fun), 0);

  return fun->flags;
}

guint
vivi_code_asm_function2_get_n_registers (ViviCodeAsmFunction2 *fun)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION2 (fun), 0);

  return fun->n_registers;
}

guint
vivi_code_asm_function2_get_n_arguments	(ViviCodeAsmFunction2 *fun)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION2 (fun), 0);

  return fun->args->len;
}

const char *
vivi_code_asm_function2_get_argument_name (ViviCodeAsmFunction2 *fun, guint i)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION2 (fun), NULL);
  g_return_val_if_fail (i < fun->args->len, NULL);

  return g_array_index (fun->args, SwfdecScriptArgument, i).name;
}

guint
vivi_code_asm_function2_get_argument_preload (ViviCodeAsmFunction2 *fun, guint i)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_FUNCTION2 (fun), 0);
  g_return_val_if_fail (i < fun->args->len, 0);

  return g_array_index (fun->args, SwfdecScriptArgument, i).preload;
}

void
vivi_code_asm_function2_add_argument (ViviCodeAsmFunction2 *fun, 
    const char *name, guint preload)
{
  SwfdecScriptArgument arg;

  g_return_if_fail (VIVI_IS_CODE_ASM_FUNCTION2 (fun));
  g_return_if_fail (fun->args->len < G_MAXUINT16);
  g_return_if_fail (name != NULL);

  arg.preload = preload;
  arg.name = g_strdup (name);
  g_array_append_val (fun->args, arg);
}
