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
#include <swfdec/swfdec_bots.h>

#include "vivi_code_asm_store.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_store_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  ViviCodeAsmStore *store = VIVI_CODE_ASM_STORE (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);

  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_STORE_REGISTER);
  swfdec_bots_put_u16 (emit, 1);
  swfdec_bots_put_u8 (emit, store->reg);

  return TRUE;
}

static void
vivi_code_asm_store_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_store_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmStore, vivi_code_asm_store, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_store_asm_init))

static void
vivi_code_asm_store_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAsmStore *store = VIVI_CODE_ASM_STORE (token);
  char *s;

  s = g_strdup_printf ("store %u", store->reg);
  vivi_code_printer_print (printer, s);
  g_free (s);
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_store_class_init (ViviCodeAsmStoreClass *klass)
{
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  token_class->print = vivi_code_asm_store_print;

  code_class->bytecode = SWFDEC_AS_ACTION_STORE_REGISTER;
}

static void
vivi_code_asm_store_init (ViviCodeAsmStore *store)
{
}

ViviCodeAsm *
vivi_code_asm_store_new (guint reg)
{
  ViviCodeAsmStore *ret;

  g_return_val_if_fail (reg < 256, NULL);

  ret = g_object_new (VIVI_TYPE_CODE_ASM_STORE, NULL);
  ret->reg = reg;

  return VIVI_CODE_ASM (ret);
}

guint
vivi_code_asm_store_get_register (ViviCodeAsmStore *store)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_STORE (store), 0);

  return store->reg;
}

