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

#include "vivi_code_asm_code.h"
#include "vivi_code_asm.h"
#include "vivi_code_printer.h"

static void
vivi_code_asm_code_asm_init (ViviCodeAsmInterface *iface)
{
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmCode, vivi_code_asm_code, VIVI_TYPE_CODE_TOKEN,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_code_asm_init))

static void
vivi_code_asm_code_class_init (ViviCodeAsmCodeClass *klass)
{
}

static void
vivi_code_asm_code_init (ViviCodeAsmCode *asm_code)
{
}

SwfdecAsAction
vivi_code_asm_code_get_action (ViviCodeAsmCode *code)
{
  ViviCodeAsmCodeClass *klass;

  g_return_val_if_fail (VIVI_IS_CODE_ASM_CODE (code), 0);

  klass = VIVI_CODE_ASM_CODE_GET_CLASS (code);

  return klass->bytecode;
}
