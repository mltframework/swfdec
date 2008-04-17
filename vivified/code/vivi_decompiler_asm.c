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

#include "vivi_decompiler.h"
#include "vivi_code_assembler.h"
#include "vivi_code_comment.h"
#include "vivi_code_asm_code_default.h"

#define vivi_decompiler_warning(assembler,...) G_STMT_START { \
  char *__s = g_strdup_printf (__VA_ARGS__); \
  ViviCodeStatement *comment = vivi_code_comment_new (__s); \
  vivi_code_assembler_add_code(assembler, VIVI_CODE_ASM (comment)); \
  g_object_unref (comment); \
  g_free (__s); \
}G_STMT_END

static ViviCodeAsm * (* simple_commands[0x80]) (void) = {
#define DEFAULT_ASM(a,b,c) [c] = vivi_code_asm_ ## b ## _new,
#include "vivi_code_defaults.h"
};


ViviCodeStatement *
vivi_decompile_script_asm (SwfdecScript *script)
{
  ViviCodeAssembler *assembler = VIVI_CODE_ASSEMBLER (vivi_code_assembler_new ());
  GHashTable *bytecodes;
  const guint8 *pc, *start, *end, *exit;
  const guint8 *data;
  guint code, len;
  SwfdecBits bits;

  start = script->buffer->data;
  end = start + script->buffer->length;
  exit = script->exit;
  pc = script->main;

  bytecodes = g_hash_table_new (g_direct_hash, g_direct_equal);

  while (pc != exit) {
    if (pc < start || pc >= end) {
      vivi_decompiler_warning (assembler, "program counter out of range");
      goto error;
    }
    code = pc[0];
    if (code & 0x80) {
      if (pc + 2 >= end) {
	vivi_decompiler_warning (assembler, "bytecode 0x%02X length value out of range", code);
	goto error;
      }
      data = pc + 3;
      len = pc[1] | pc[2] << 8;
      if (data + len > end) {
	vivi_decompiler_warning (assembler, "bytecode 0x%02X length %u out of range", code, len);
	goto error;
      }
      swfdec_bits_init_data (&bits, data, len);
      switch (code) {
	default:
	  vivi_decompiler_warning (assembler, "unknown bytecode 0x%02X", code);
	  pc = data + len;
      }
    } else {
      if (simple_commands[code]) {
	ViviCodeAsm *asmcode = simple_commands[code] ();
	vivi_code_assembler_add_code (assembler, asmcode);
	g_object_unref (asmcode);
      } else {
	vivi_decompiler_warning (assembler, "unknown bytecode 0x%02X", code);
      }
      pc++;
    }
  }

error:
  g_hash_table_destroy (bytecodes);
  return VIVI_CODE_STATEMENT (assembler);
}

