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

#include "vivi_code_asm_jump.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_jump_resolve (ViviCodeEmitter *emitter, SwfdecBuffer *buffer,
    gsize offset, gpointer data, GError **error)
{
  ViviCodeAsmJump *jump = VIVI_CODE_ASM_JUMP (data);
  gssize label_offset, diff;

  label_offset = vivi_code_emitter_get_label_offset (emitter, jump->label);
  if (label_offset < 0) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_MISSING_LABEL,
	"no label \"%s\"", vivi_code_label_get_name (jump->label));
    return FALSE;
  }
  diff = label_offset - offset;
  if (diff > G_MAXINT16 || diff < G_MININT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"branch target too far away");
    return FALSE;
  }
  buffer->data[offset - 1] = diff >> 8;
  buffer->data[offset - 2] = diff;
  return TRUE;
}


static gboolean
vivi_code_asm_jump_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);

  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_JUMP);
  swfdec_bots_put_u16 (emit, 2);
  swfdec_bots_put_u16 (emit, 0); /* jump to be resolved later */
  vivi_code_emitter_add_later (emitter, vivi_code_asm_jump_resolve, code);

  return TRUE;
}

static void
vivi_code_asm_jump_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_jump_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmJump, vivi_code_asm_jump, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_jump_asm_init))

static void
vivi_code_asm_jump_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAsmJump *jump = VIVI_CODE_ASM_JUMP (token);

  vivi_code_printer_print (printer, "jump ");
  vivi_code_printer_print (printer, vivi_code_label_get_name (jump->label));
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_jump_dispose (GObject *object)
{
  ViviCodeAsmJump *jump = VIVI_CODE_ASM_JUMP (object);

  g_object_unref (jump->label);

  G_OBJECT_CLASS (vivi_code_asm_jump_parent_class)->dispose (object);
}

static void
vivi_code_asm_jump_class_init (ViviCodeAsmJumpClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  object_class->dispose = vivi_code_asm_jump_dispose;

  token_class->print = vivi_code_asm_jump_print;

  code_class->bytecode = SWFDEC_AS_ACTION_JUMP;
}

static void
vivi_code_asm_jump_init (ViviCodeAsmJump *jump)
{
}

ViviCodeAsm *
vivi_code_asm_jump_new (ViviCodeLabel *label)
{
  ViviCodeAsmJump *jump;

  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), NULL);

  jump = g_object_new (VIVI_TYPE_CODE_ASM_JUMP, NULL);
  jump->label = g_object_ref (label);

  return VIVI_CODE_ASM (jump);
}

ViviCodeLabel *
vivi_code_asm_jump_get_label (ViviCodeAsmJump *jump)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_JUMP (jump), NULL);

  return jump->label;
}

