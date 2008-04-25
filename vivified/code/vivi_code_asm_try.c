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

#include <string.h>

#include <swfdec/swfdec_as_interpret.h>
#include <swfdec/swfdec_bits.h>

#include "vivi_code_asm_try.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_try_resolve (ViviCodeEmitter *emitter, SwfdecBuffer *buffer,
    gsize offset, gpointer data, GError **error)
{
  ViviCodeAsmTry *try_ = VIVI_CODE_ASM_TRY (data);
  gssize catch_start_offset, finally_start_offset, end_offset;
  gsize write_offset, diff;

  write_offset = offset;

  if (try_->use_register) {
    offset += 1;
  } else {
    offset += strlen (try_->variable_name) + 1;
  }

  catch_start_offset =
    vivi_code_emitter_get_label_offset (emitter, try_->catch_start);
  if (catch_start_offset < 0) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_MISSING_LABEL,
	"no label \"%s\"", vivi_code_label_get_name (try_->catch_start));
    return FALSE;
  }

  finally_start_offset =
    vivi_code_emitter_get_label_offset (emitter, try_->finally_start);
  if (finally_start_offset < 0) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_MISSING_LABEL,
	"no label \"%s\"", vivi_code_label_get_name (try_->finally_start));
    return FALSE;
  }

  end_offset = vivi_code_emitter_get_label_offset (emitter, try_->end);
  if (end_offset < 0) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_MISSING_LABEL,
	"no label \"%s\"", vivi_code_label_get_name (try_->end));
    return FALSE;
  }

  if ((gsize)catch_start_offset < offset) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"catch start before try");
    return FALSE;
  }

  if (finally_start_offset < catch_start_offset) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"finally start before catch start");
    return FALSE;
  }

  if (end_offset < finally_start_offset) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"end before finally start");
    return FALSE;
  }

  // try size
  diff = catch_start_offset - offset;
  if (diff > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"catch start too far away");
    return FALSE;
  }

  buffer->data[write_offset - 5] = diff >> 8;
  buffer->data[write_offset - 6] = diff;

  // catch size
  diff = finally_start_offset - catch_start_offset;
  if (diff > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"finally start too far away");
    return FALSE;
  }

  buffer->data[write_offset - 3] = diff >> 8;
  buffer->data[write_offset - 4] = diff;

  // finally size
  diff = end_offset - finally_start_offset;
  if (diff > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"end too far away");
    return FALSE;
  }

  buffer->data[write_offset - 1] = diff >> 8;
  buffer->data[write_offset - 2] = diff;

  return TRUE;
}

static gboolean
vivi_code_asm_try_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter,
    GError **error)
{
  ViviCodeAsmTry *try_ = VIVI_CODE_ASM_TRY (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);
  guint flags;

  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_TRY);
  swfdec_bots_put_u16 (emit,
      8 + (try_->use_register ? 0 : strlen (try_->variable_name)));

  flags = try_->reserved_flags << 3;
  if (try_->has_catch) {
    flags |= (1 << 0);
  } else {
    flags &= ~(1 << 0);
  }
  if (try_->has_finally) {
    flags |= (1 << 1);
  } else {
    flags &= ~(1 << 1);
  }
  if (try_->use_register) {
    flags |= (1 << 2);
  } else {
    flags &= ~(1 << 2);
  }
  swfdec_bots_put_u8 (emit, flags);

  swfdec_bots_put_u16 (emit, 0); /* try size */
  swfdec_bots_put_u16 (emit, 0); /* catch size */
  swfdec_bots_put_u16 (emit, 0); /* finally size */
  vivi_code_emitter_add_later (emitter, vivi_code_asm_try_resolve, code);

  if (try_->use_register) {
    swfdec_bots_put_u8 (emit, try_->register_number);
  } else {
    swfdec_bots_put_string (emit, try_->variable_name);
  }

  return TRUE;
}

static void
vivi_code_asm_try_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_try_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmTry, vivi_code_asm_try, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_try_asm_init))

static void
vivi_code_asm_try_print (ViviCodeToken *token, ViviCodePrinter *printer)
{
  ViviCodeAsmTry *try_ = VIVI_CODE_ASM_TRY (token);
  guint i;

  vivi_code_printer_print (printer, "try ");

  if (try_->has_catch)
    vivi_code_printer_print (printer, "has_catch ");

  if (try_->has_finally)
    vivi_code_printer_print (printer, "has_finally ");

  if (try_->use_register)
    vivi_code_printer_print (printer, "use_register ");

  for (i = 0; i < 5; i++) {
    if (try_->reserved_flags & (1 << i)) {
      char *str = g_strdup_printf ("reserverd%i ", i + 1);
      vivi_code_printer_print (printer, str);
      g_free (str);
    }
  }

  if (try_->use_register) {
    char *str = g_strdup_printf ("%i", try_->register_number);
    vivi_code_printer_print (printer, str);
    g_free (str);
  } else {
    vivi_code_printer_print (printer, try_->variable_name);
  }
  vivi_code_printer_print (printer, " ");

  vivi_code_printer_print (printer,
      vivi_code_label_get_name (try_->catch_start));
  vivi_code_printer_print (printer, " ");

  vivi_code_printer_print (printer,
      vivi_code_label_get_name (try_->finally_start));
  vivi_code_printer_print (printer, " ");

  vivi_code_printer_print (printer, vivi_code_label_get_name (try_->end));

  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_try_dispose (GObject *object)
{
  ViviCodeAsmTry *try_ = VIVI_CODE_ASM_TRY (object);

  if (!try_->use_register)
    g_free (try_->variable_name);

  g_object_unref (try_->catch_start);
  g_object_unref (try_->finally_start);
  g_object_unref (try_->end);

  G_OBJECT_CLASS (vivi_code_asm_try_parent_class)->dispose (object);
}

static void
vivi_code_asm_try_class_init (ViviCodeAsmTryClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  object_class->dispose = vivi_code_asm_try_dispose;

  token_class->print = vivi_code_asm_try_print;

  code_class->bytecode = SWFDEC_AS_ACTION_TRY;
}

static void
vivi_code_asm_try_init (ViviCodeAsmTry *try_)
{
}

static ViviCodeAsmTry *
vivi_code_asm_try_new_internal (gboolean has_catch, gboolean has_finally,
    ViviCodeLabel *catch_start, ViviCodeLabel *finally_start,
    ViviCodeLabel *end)
{
  ViviCodeAsmTry *ret;

  g_return_val_if_fail (VIVI_IS_CODE_LABEL (catch_start), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_LABEL (finally_start), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_LABEL (end), NULL);

  ret = g_object_new (VIVI_TYPE_CODE_ASM_TRY, NULL);
  ret->catch_start = g_object_ref (catch_start);
  ret->finally_start = g_object_ref (finally_start);
  ret->end = g_object_ref (end);

  ret->has_catch = has_catch;
  ret->has_finally = has_finally;
  ret->reserved_flags = 0;

  return ret;
}

ViviCodeAsm *
vivi_code_asm_try_new_variable (const char *name, gboolean has_catch,
   gboolean has_finally, ViviCodeLabel *catch_start,
   ViviCodeLabel *finally_start, ViviCodeLabel *end)
{
  ViviCodeAsmTry *try_;

  g_return_val_if_fail (name != NULL, NULL);

  try_ = vivi_code_asm_try_new_internal (has_catch, has_finally, catch_start,
      finally_start, end);
  try_->use_register = FALSE;
  try_->variable_name = g_strdup (name);

  return VIVI_CODE_ASM (try_);
}

ViviCodeAsm *
vivi_code_asm_try_new_register (guint register_number, gboolean has_catch,
    gboolean has_finally, ViviCodeLabel *catch_start,
    ViviCodeLabel *finally_start, ViviCodeLabel *end)
{
  ViviCodeAsmTry *try_;

  g_return_val_if_fail (register_number < 256, NULL);

  try_ = vivi_code_asm_try_new_internal (has_catch, has_finally, catch_start,
      finally_start, end);
  try_->use_register = TRUE;
  try_->register_number = register_number;

  return VIVI_CODE_ASM (try_);
}

void
vivi_code_asm_try_set_reserved_flags (ViviCodeAsmTry *try_, guint flags)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_TRY (try_));
  g_return_if_fail (flags < (1 << 5));

  try_->reserved_flags = flags;
}

void
vivi_code_asm_try_set_variable_name (ViviCodeAsmTry *try_, const char *name)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_TRY (try_));
  g_return_if_fail (name != NULL);

  if (!try_->use_register) {
    g_free (try_->variable_name);
  } else {
    try_->use_register = FALSE;
  }

  try_->variable_name = g_strdup (name);
}
