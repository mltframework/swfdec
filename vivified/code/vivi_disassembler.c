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
#include "vivi_code_asm_code_default.h"
#include "vivi_code_asm_pool.h"
#include "vivi_code_asm_push.h"
#include "vivi_code_assembler.h"
#include "vivi_code_comment.h"

#define vivi_disassembler_warning(assembler,...) G_STMT_START { \
  char *__s = g_strdup_printf (__VA_ARGS__); \
  ViviCodeStatement *comment = vivi_code_comment_new (__s); \
  vivi_code_assembler_add_code (assembler, VIVI_CODE_ASM (comment)); \
  g_object_unref (comment); \
  g_free (__s); \
}G_STMT_END

static ViviCodeAsm * (* simple_commands[0x80]) (void) = {
#define DEFAULT_ASM(a,b,c) [c] = vivi_code_asm_ ## b ## _new,
#include "vivi_code_defaults.h"
};

static void
vivi_disassemble_pool (ViviCodeAssembler *assembler, SwfdecBits *bits, guint version)
{
  ViviCodeAsm *code;
  SwfdecConstantPool *pool;
  SwfdecBuffer *buffer;

  buffer = swfdec_bits_get_buffer (bits, -1);
  pool = swfdec_constant_pool_new (NULL, buffer, version);
  swfdec_buffer_unref (buffer);
  code = vivi_code_asm_pool_new (pool);
  swfdec_constant_pool_unref (pool);
  vivi_code_assembler_add_code (assembler, code);
  g_object_unref (code);
}

static void
vivi_disassemble_push (ViviCodeAssembler *assembler, SwfdecBits *bits, guint version)
{
  ViviCodeAsmPush *push;
  guint type;

  push = VIVI_CODE_ASM_PUSH (vivi_code_asm_push_new ());
  while (swfdec_bits_left (bits)) {
    type = swfdec_bits_get_u8 (bits);
    switch (type) {
      case 0: /* string */
	{
	  char *s = swfdec_bits_get_string (bits, version);
	  if (s == NULL) {
	    vivi_disassembler_warning (assembler, "could not read string");
	    goto fail;
	  }
	  vivi_code_asm_push_add_string (push, s);
	  g_free (s);
	  break;
	}
      case 1: /* float */
	vivi_code_asm_push_add_float (push, swfdec_bits_get_float (bits));
	break;
      case 2: /* null */
	vivi_code_asm_push_add_null (push);
	break;
      case 3: /* undefined */
	vivi_code_asm_push_add_undefined (push);
	break;
      case 4: /* register */
	vivi_code_asm_push_add_register (push, swfdec_bits_get_u8 (bits));
	break;
      case 5: /* boolean */
	vivi_code_asm_push_add_boolean (push, swfdec_bits_get_u8 (bits));
	break;
      case 6: /* double */
	vivi_code_asm_push_add_double (push, swfdec_bits_get_double (bits));
	break;
      case 7: /* 32bit int */
	vivi_code_asm_push_add_integer (push, swfdec_bits_get_s32 (bits));
	break;
      case 8: /* 8bit ConstantPool address */
	vivi_code_asm_push_add_pool (push, swfdec_bits_get_u8 (bits));
	break;
      case 9: /* 16bit ConstantPool address */
	vivi_code_asm_push_add_pool (push, swfdec_bits_get_u16 (bits));
	break;
      default:
	vivi_disassembler_warning (assembler, "Push: unknown type %u, ignoring", type);
	break;
    }
  }

  vivi_code_assembler_add_code (assembler, VIVI_CODE_ASM (push));
fail:
  g_object_unref (push);
}

ViviCodeStatement *
vivi_disassemble_script (SwfdecScript *script)
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
      vivi_disassembler_warning (assembler, "program counter out of range");
      goto error;
    }
    code = pc[0];
    if (code & 0x80) {
      if (pc + 2 >= end) {
	vivi_disassembler_warning (assembler, "bytecode 0x%02X length value out of range", code);
	goto error;
      }
      data = pc + 3;
      len = pc[1] | pc[2] << 8;
      if (data + len > end) {
	vivi_disassembler_warning (assembler, "bytecode 0x%02X length %u out of range", code, len);
	goto error;
      }
      swfdec_bits_init_data (&bits, data, len);
      switch (code) {
        case SWFDEC_AS_ACTION_CONSTANT_POOL:
	  vivi_disassemble_pool (assembler, &bits, script->version);
	  pc = data + len;
	  break;
	case SWFDEC_AS_ACTION_PUSH:
	  vivi_disassemble_push (assembler, &bits, script->version);
	  pc = data + len;
	  break;
        case SWFDEC_AS_ACTION_GOTO_FRAME:
        case SWFDEC_AS_ACTION_GET_URL:
        case SWFDEC_AS_ACTION_STORE_REGISTER:
        case SWFDEC_AS_ACTION_STRICT_MODE:
        case SWFDEC_AS_ACTION_WAIT_FOR_FRAME:
        case SWFDEC_AS_ACTION_SET_TARGET:
        case SWFDEC_AS_ACTION_GOTO_LABEL:
        case SWFDEC_AS_ACTION_WAIT_FOR_FRAME2:
        case SWFDEC_AS_ACTION_DEFINE_FUNCTION2:
        case SWFDEC_AS_ACTION_TRY:
        case SWFDEC_AS_ACTION_WITH:
        case SWFDEC_AS_ACTION_JUMP:
        case SWFDEC_AS_ACTION_GET_URL2:
        case SWFDEC_AS_ACTION_DEFINE_FUNCTION:
        case SWFDEC_AS_ACTION_IF:
        case SWFDEC_AS_ACTION_CALL:
        case SWFDEC_AS_ACTION_GOTO_FRAME2:
	default:
	  vivi_disassembler_warning (assembler, "unknown bytecode 0x%02X", code);
	  pc = data + len;
	  break;
      }
    } else {
      if (simple_commands[code]) {
	ViviCodeAsm *asmcode = simple_commands[code] ();
	vivi_code_assembler_add_code (assembler, asmcode);
	g_object_unref (asmcode);
      } else {
	vivi_disassembler_warning (assembler, "unknown bytecode 0x%02X", code);
      }
      pc++;
    }
  }

error:
  g_hash_table_destroy (bytecodes);
  return VIVI_CODE_STATEMENT (assembler);
}

