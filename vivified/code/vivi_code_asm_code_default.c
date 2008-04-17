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

#include "vivi_code_asm_code_default.h"
#include "vivi_code_printer.h"

#define DEFAULT_ASM(CapsName, underscore_name, bytecode) \
\
G_DEFINE_TYPE (ViviCodeAsm ## CapsName, vivi_code_asm_ ## underscore_name, VIVI_TYPE_CODE_ASM_CODE); \
\
static void \
vivi_code_asm_ ## underscore_name ## _print (ViviCodeToken *token, ViviCodePrinter *printer) \
{ \
  vivi_code_printer_print (printer, G_STRINGIFY (underscore_name)); \
  vivi_code_printer_new_line (printer, TRUE); \
} \
\
static void \
vivi_code_asm_ ## underscore_name ## _class_init (ViviCodeAsm ## CapsName ## Class *klass) \
{ \
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass); \
\
  token_class->print = vivi_code_asm_ ## underscore_name ## _print; \
} \
\
static void \
vivi_code_asm_ ## underscore_name ## _init (ViviCodeAsm ## CapsName *code) \
{ \
} \
\
ViviCodeAsm * \
vivi_code_asm_ ## underscore_name ## _new (void) \
{ \
  return g_object_new (vivi_code_asm_ ## underscore_name ## _get_type (), NULL); \
}

#include "vivi_code_defaults.h"
