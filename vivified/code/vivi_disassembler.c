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
#include "vivi_code_asm_function.h"
#include "vivi_code_asm_if.h"
#include "vivi_code_asm_jump.h"
#include "vivi_code_asm_pool.h"
#include "vivi_code_asm_push.h"
#include "vivi_code_asm_store.h"
#include "vivi_code_assembler.h"
#include "vivi_code_comment.h"
#include "vivi_code_label.h"

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
vivi_disassemble_store (ViviCodeAssembler *assembler, SwfdecBits *bits)
{
  ViviCodeAsm *code;

  code = vivi_code_asm_store_new (swfdec_bits_get_u8 (bits));
  vivi_code_assembler_add_code (assembler, code);
  g_object_unref (code);
}

static void
vivi_disassemble_pool (ViviCodeAssembler *assembler, SwfdecBits *bits, guint version)
{
  ViviCodeAsm *code;
  SwfdecConstantPool *pool;
  SwfdecBuffer *buffer;

  buffer = swfdec_bits_get_buffer (bits, -1);
  pool = swfdec_constant_pool_new (NULL, buffer, version);
  swfdec_buffer_unref (buffer);
  if (pool == NULL) {
    vivi_disassembler_warning (assembler, "invalid constant pool");
    return;
  }
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
	vivi_code_asm_push_add_pool_big (push, swfdec_bits_get_u16 (bits));
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

typedef GArray ViviDisassembleLabels;
typedef struct {
  const guint8 *	bytecode;
  ViviCodeLabel *	label;
} ViviLabelEntry;

static ViviDisassembleLabels *
vivi_disassemble_labels_new (void)
{
  return g_array_new (FALSE, FALSE, sizeof (ViviLabelEntry));
}

static ViviCodeLabel *
vivi_disassemble_labels_get_label (ViviDisassembleLabels *labels, const guint8 *bytecode)
{
  ViviLabelEntry ins;
  guint i;
  char *s;

  for (i = 0; i < labels->len; i++) {
    ViviLabelEntry *entry = &g_array_index (labels, ViviLabelEntry, i);
    if (entry->bytecode < bytecode)
      continue;
    if (entry->bytecode == bytecode)
      return entry->label;
    break;
  }
  s = g_strdup_printf ("label_%p", bytecode);
  ins.bytecode = bytecode;
  ins.label = VIVI_CODE_LABEL (vivi_code_label_new (s));
  g_free (s);
  g_array_insert_val (labels, i, ins);
  return ins.label;
}

static void
vivi_disassemble_labels_resolve (ViviDisassembleLabels *labels,
    ViviCodeAssembler *assembler, SwfdecScript *script)
{
  guint li, ai;
  const guint8 *pc, *end;

  if (labels->len == 0)
    return;

  ai = li = 0;
  pc = script->main;
  end = script->buffer->data + script->buffer->length;
  while (pc < end) {
    ViviLabelEntry *entry = &g_array_index (labels, ViviLabelEntry, li);

    if (entry->bytecode < pc) {
      char *s = g_strdup_printf ("broken jump: goes to %p, but next bytecode is %p",
	  entry->bytecode, pc);
      ViviCodeStatement *comment = vivi_code_comment_new (s);
      vivi_code_assembler_insert_code (assembler, ai, VIVI_CODE_ASM (comment));
      ai++;
      g_object_unref (comment);
      g_free (s);
    }
    if (entry->bytecode <= pc) {
      vivi_code_assembler_insert_code (assembler, ai, VIVI_CODE_ASM (entry->label));
      ai++;
      li++;
      if (li >= labels->len)
	break;
      continue;
    }
    if (pc[0] & 0x80) {
      pc += 3 + (pc[1] | pc[2] << 8);
    } else {
      pc++;
    }
    ai++;
  }
}

static void
vivi_disassemble_labels_free (ViviDisassembleLabels *labels)
{
  guint i;

  for (i = 0; i < labels->len; i++) {
    g_object_unref (g_array_index (labels, ViviLabelEntry, i).label);
  }
  g_array_free (labels, TRUE);
}

ViviCodeStatement *
vivi_disassemble_script (SwfdecScript *script)
{
  ViviCodeAssembler *assembler = VIVI_CODE_ASSEMBLER (vivi_code_assembler_new ());
  ViviDisassembleLabels *labels;
  const guint8 *pc, *start, *end, *exit;
  const guint8 *data;
  guint code, len;
  SwfdecBits bits;

  exit = script->exit;
  pc = script->main;
  start = script->buffer->data;
  end = start + script->buffer->length;

  /* NB:
   * every operation must put EXACTLY one asm object onto the stack, or the 
   * label resolving will break.
   */
  labels = vivi_disassemble_labels_new ();

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
      pc = data + len;
      switch (code) {
        case SWFDEC_AS_ACTION_CONSTANT_POOL:
	  vivi_disassemble_pool (assembler, &bits, script->version);
	  break;
	case SWFDEC_AS_ACTION_PUSH:
	  vivi_disassemble_push (assembler, &bits, script->version);
	  break;
        case SWFDEC_AS_ACTION_STORE_REGISTER:
	  vivi_disassemble_store (assembler, &bits);
	  break;
        case SWFDEC_AS_ACTION_JUMP:
	  {
	    ViviCodeLabel *label = vivi_disassemble_labels_get_label (labels, 
		pc + swfdec_bits_get_s16 (&bits));
	    ViviCodeAsm *asm_code = vivi_code_asm_jump_new (label);
	    vivi_code_assembler_add_code (assembler, asm_code);
	    g_object_unref (asm_code);
	  }
	  break;
        case SWFDEC_AS_ACTION_IF:
	  {
	    ViviCodeLabel *label = vivi_disassemble_labels_get_label (labels, 
		pc + swfdec_bits_get_s16 (&bits));
	    ViviCodeAsm *asm_code = vivi_code_asm_if_new (label);
	    vivi_code_assembler_add_code (assembler, asm_code);
	    g_object_unref (asm_code);
	  }
	  break;
        case SWFDEC_AS_ACTION_DEFINE_FUNCTION:
	  {
	    char *name;
	    ViviCodeLabel *label;
	    ViviCodeAsm *fun;
	    GPtrArray *args;
	    guint i, n_args;

	    name = swfdec_bits_get_string (&bits, script->version);
	    n_args = swfdec_bits_get_u16 (&bits);
	    args = g_ptr_array_sized_new (n_args + 1);
	    for (i = 0; i < n_args; i++) {
	      g_ptr_array_add (args, swfdec_bits_get_string (&bits, script->version));
	    }
	    g_ptr_array_add (args, NULL);
	    label = vivi_disassemble_labels_get_label (labels, 
		pc + swfdec_bits_get_u16 (&bits));
	    fun = vivi_code_asm_function_new (label, 
		name && *name ? name : NULL,
		(char **) (n_args > 0 ? args->pdata : NULL));
	    g_free (name);
	    g_strfreev ((char **) g_ptr_array_free (args, FALSE));
	    vivi_code_assembler_add_code (assembler, fun);
	    g_object_unref (fun);
	  }
	  break;
        case SWFDEC_AS_ACTION_GOTO_FRAME:
        case SWFDEC_AS_ACTION_GET_URL:
        case SWFDEC_AS_ACTION_STRICT_MODE:
        case SWFDEC_AS_ACTION_WAIT_FOR_FRAME:
        case SWFDEC_AS_ACTION_SET_TARGET:
        case SWFDEC_AS_ACTION_GOTO_LABEL:
        case SWFDEC_AS_ACTION_WAIT_FOR_FRAME2:
        case SWFDEC_AS_ACTION_DEFINE_FUNCTION2:
        case SWFDEC_AS_ACTION_TRY:
        case SWFDEC_AS_ACTION_WITH:
        case SWFDEC_AS_ACTION_GET_URL2:
        case SWFDEC_AS_ACTION_CALL:
        case SWFDEC_AS_ACTION_GOTO_FRAME2:
	default:
	  vivi_disassembler_warning (assembler, "unknown bytecode 0x%02X", code);
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
  vivi_disassemble_labels_resolve (labels, assembler, script);
  vivi_disassemble_labels_free (labels);
  return VIVI_CODE_STATEMENT (assembler);
}

