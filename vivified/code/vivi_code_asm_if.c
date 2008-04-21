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

#include "vivi_code_asm_if.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_if_resolve (ViviCodeEmitter *emitter, SwfdecBuffer *buffer,
    gpointer data, GError **error)
{
  /* FIXME: write */
  g_return_val_if_reached (FALSE);
}

static gboolean
vivi_code_asm_if_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);

  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_IF);
  swfdec_bots_put_u16 (emit, 2);
  swfdec_bots_put_u16 (emit, 0); /* if to be resolved later */
  vivi_code_emitter_add_later (emitter, vivi_code_asm_if_resolve, code);

  return TRUE;
}

static void
vivi_code_asm_if_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_if_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmIf, vivi_code_asm_if, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_if_asm_init))

static void
vivi_code_asm_if_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAsmIf *iff = VIVI_CODE_ASM_IF (token);

  vivi_code_printer_print (printer, "if ");
  vivi_code_printer_print (printer, vivi_code_label_get_name (iff->label));
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_if_dispose (GObject *object)
{
  ViviCodeAsmIf *iff = VIVI_CODE_ASM_IF (object);

  g_object_unref (iff->label);

  G_OBJECT_CLASS (vivi_code_asm_if_parent_class)->dispose (object);
}

static void
vivi_code_asm_if_class_init (ViviCodeAsmIfClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  object_class->dispose = vivi_code_asm_if_dispose;

  token_class->print = vivi_code_asm_if_print;

  code_class->bytecode = SWFDEC_AS_ACTION_IF;
}

static void
vivi_code_asm_if_init (ViviCodeAsmIf *iff)
{
}

ViviCodeAsm *
vivi_code_asm_if_new (ViviCodeLabel *label)
{
  ViviCodeAsmIf *ret;

  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), NULL);

  ret = g_object_new (VIVI_TYPE_CODE_ASM_IF, NULL);
  ret->label = g_object_ref (label);

  return VIVI_CODE_ASM (ret);
}

ViviCodeLabel *
vivi_code_asm_if_get_label (ViviCodeAsmIf *iff)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_IF (iff), NULL);

  return iff->label;
}

