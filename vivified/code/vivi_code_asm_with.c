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

#include "vivi_code_asm_with.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_with_resolve (ViviCodeEmitter *emitter, SwfdecBuffer *buffer,
    gsize offset, gpointer data, GError **error)
{
  ViviCodeAsmWith *with = VIVI_CODE_ASM_WITH (data);
  gssize label_offset;
  gsize diff;

  label_offset = vivi_code_emitter_get_label_offset (emitter, with->label);
  if (label_offset < 0) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_MISSING_LABEL,
	"no label \"%s\"", vivi_code_label_get_name (with->label));
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
	"with body too big");
    return FALSE;
  }
  buffer->data[offset - 1] = diff >> 8;
  buffer->data[offset - 2] = diff;
  return TRUE;
}


static gboolean
vivi_code_asm_with_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);

  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_WITH);
  swfdec_bots_put_u16 (emit, 2);
  swfdec_bots_put_u16 (emit, 0); /* with to be resolved later */
  vivi_code_emitter_add_later (emitter, vivi_code_asm_with_resolve, code);

  return TRUE;
}

static void
vivi_code_asm_with_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_with_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmWith, vivi_code_asm_with, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_with_asm_init))

static void
vivi_code_asm_with_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAsmWith *with = VIVI_CODE_ASM_WITH (token);

  vivi_code_printer_print (printer, "with ");
  vivi_code_printer_print (printer, vivi_code_label_get_name (with->label));
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_with_dispose (GObject *object)
{
  ViviCodeAsmWith *with = VIVI_CODE_ASM_WITH (object);

  g_object_unref (with->label);

  G_OBJECT_CLASS (vivi_code_asm_with_parent_class)->dispose (object);
}

static void
vivi_code_asm_with_class_init (ViviCodeAsmWithClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  object_class->dispose = vivi_code_asm_with_dispose;

  token_class->print = vivi_code_asm_with_print;

  code_class->bytecode = SWFDEC_AS_ACTION_WITH;
}

static void
vivi_code_asm_with_init (ViviCodeAsmWith *with)
{
}

ViviCodeAsm *
vivi_code_asm_with_new (ViviCodeLabel *label)
{
  ViviCodeAsmWith *with;

  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), NULL);

  with = g_object_new (VIVI_TYPE_CODE_ASM_WITH, NULL);
  with->label = g_object_ref (label);

  return VIVI_CODE_ASM (with);
}

ViviCodeLabel *
vivi_code_asm_with_get_label (ViviCodeAsmWith *with)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_WITH (with), NULL);

  return with->label;
}

