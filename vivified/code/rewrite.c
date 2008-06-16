/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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
#include <swfdec/swfdec.h>
#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_bots.h>
#include <swfdec/swfdec_script_internal.h>
#include <swfdec/swfdec_tag.h>

#include <vivified/code/vivi_code_asm_code_default.h>
#include <vivified/code/vivi_code_asm_function.h>
#include <vivified/code/vivi_code_asm_function2.h>
#include <vivified/code/vivi_code_asm_pool.h>
#include <vivified/code/vivi_code_asm_push.h>
#include <vivified/code/vivi_code_compiler.h>
#include <vivified/code/vivi_code_assembler.h>
#include <vivified/code/vivi_decompiler.h>
#include <vivified/code/vivi_parser.h>

typedef enum {
  REWRITE_TRACE_FUNCTION_NAME = (1 << 0),
  REWRITE_RANDOM              = (1 << 1),
  REWRITE_GETTERS	      = (1 << 2),

  REWRITE_INIT		      =	(1 << 17)
} RewriteOptions;

static const char *
assembler_lookup_pool (ViviCodeAssembler *assembler, guint i)
{
  ViviCodeAsm *code = vivi_code_assembler_get_code (assembler, 0);
  SwfdecConstantPool *pool;

  if (!VIVI_IS_CODE_ASM_POOL (code))
    return NULL;
  pool = vivi_code_asm_pool_get_pool (VIVI_CODE_ASM_POOL (code));
  if (i >= swfdec_constant_pool_size (pool))
    return NULL;
  return swfdec_constant_pool_get (pool, i);
}

static const char *
get_function_name (ViviCodeAssembler *assembler, guint i)
{
  ViviCodeAsmPush *push;
  guint n;

  if (i == 0)
    return NULL;
  if (!VIVI_IS_CODE_ASM_PUSH (vivi_code_assembler_get_code (assembler, i - 1)))
    return NULL;
  push = VIVI_CODE_ASM_PUSH (vivi_code_assembler_get_code (assembler, i - 1));
  n = vivi_code_asm_push_get_n_values (push);
  if (n > 0) {
    ViviCodeConstantType type;
    n--;
    type = vivi_code_asm_push_get_value_type (push, n);
    if (type == VIVI_CODE_CONSTANT_STRING) {
      return vivi_code_asm_push_get_string (push, n);
    } else if (type == VIVI_CODE_CONSTANT_CONSTANT_POOL ||
	       type == VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG) {
      return assembler_lookup_pool (assembler, 
	  vivi_code_asm_push_get_pool (push, n));
    }
  }
  return NULL;
}

#define INSERT_CODE(assembler, i, code) G_STMT_START { \
  ViviCodeAsm *__code = (code); \
  vivi_code_assembler_insert_code (assembler, i++, __code); \
  g_object_unref (__code); \
} G_STMT_END
#define INSERT_PUSH_STRING(assembler, i, str) G_STMT_START { \
  ViviCodeAsm *__push = vivi_code_asm_push_new (); \
  vivi_code_asm_push_add_string (VIVI_CODE_ASM_PUSH (__push), str); \
  INSERT_CODE (assembler, i, __push); \
} G_STMT_END
static void
insert_function_trace (ViviCodeAssembler *assembler, const char *name)
{
  guint i, j;

  i = 0;
  if (VIVI_IS_CODE_ASM_POOL (vivi_code_assembler_get_code (assembler, i)))
    i++;
  INSERT_PUSH_STRING (assembler, i, name);
  INSERT_CODE (assembler, i, vivi_code_asm_trace_new ());
  for (i = 0; i < vivi_code_assembler_get_n_codes (assembler); i++) {
    ViviCodeAsm *code = vivi_code_assembler_get_code (assembler, i);
    if (VIVI_IS_CODE_ASM_FUNCTION (code)) {
      const char *function_name = get_function_name (assembler, i);
      char * const *args;
      if (function_name == NULL)
	function_name = "anonymous function";
      i++;
      INSERT_PUSH_STRING (assembler, i, function_name);
      INSERT_PUSH_STRING (assembler, i, " (");
      INSERT_CODE (assembler, i, vivi_code_asm_add2_new ());
      args = vivi_code_asm_function_get_arguments (VIVI_CODE_ASM_FUNCTION (code));
      for (j = 0; args && args[j]; j++) {
	if (j > 0) {
	  INSERT_PUSH_STRING (assembler, i, ", ");
	  INSERT_CODE (assembler, i, vivi_code_asm_add2_new ());
	}
	INSERT_PUSH_STRING (assembler, i, args[j]);
	INSERT_CODE (assembler, i, vivi_code_asm_get_variable_new ());
	INSERT_CODE (assembler, i, vivi_code_asm_add2_new ());
      }
      INSERT_PUSH_STRING (assembler, i, ")");
      INSERT_CODE (assembler, i, vivi_code_asm_add2_new ());
      INSERT_CODE (assembler, i, vivi_code_asm_trace_new ());
      i--;
    } else if (VIVI_IS_CODE_ASM_FUNCTION2 (code)) {
      const char *function_name = get_function_name (assembler, i);
      ViviCodeAsmFunction2 *fun = VIVI_CODE_ASM_FUNCTION2 (code);
      if (function_name == NULL)
	function_name = "anonymous function";
      i++;
      INSERT_PUSH_STRING (assembler, i, function_name);
      INSERT_PUSH_STRING (assembler, i, " (");
      INSERT_CODE (assembler, i, vivi_code_asm_add2_new ());
      for (j = 0; j < vivi_code_asm_function2_get_n_arguments (fun); j++) {
	guint preload = vivi_code_asm_function2_get_argument_preload (fun, j);
	if (j > 0) {
	  INSERT_PUSH_STRING (assembler, i, ", ");
	  INSERT_CODE (assembler, i, vivi_code_asm_add2_new ());
	}
	if (preload) {
	  ViviCodeAsm *push = vivi_code_asm_push_new ();
	  vivi_code_asm_push_add_register (VIVI_CODE_ASM_PUSH (push), preload);
	  INSERT_CODE (assembler, i, push);
	} else {
	  INSERT_PUSH_STRING (assembler, i, 
	      vivi_code_asm_function2_get_argument_name (fun, j));
	  INSERT_CODE (assembler, i, vivi_code_asm_get_variable_new ());
	}
	INSERT_CODE (assembler, i, vivi_code_asm_add2_new ());
      }
      INSERT_PUSH_STRING (assembler, i, ")");
      INSERT_CODE (assembler, i, vivi_code_asm_add2_new ());
      INSERT_CODE (assembler, i, vivi_code_asm_trace_new ());
      i--;
    }
  }
}

static void
replace_random (ViviCodeAssembler *assembler, guint init)
{
  guint i = 0;

  if (init) {
    guint j;
    ViviCodeStatement *statement;
    ViviCodeCompiler *compiler;
    ViviCodeAssembler *asm2;
    if (VIVI_IS_CODE_ASM_POOL (vivi_code_assembler_get_code (assembler, i)))
      i++;

    statement = vivi_parse_string (
	"ASSetPropFlags (Math, \"random\", 0, 0-1);"
	"Math.random = function () { return 0; };"
	"ASSetPropFlags (Math, \"random\", 7);"
	);
    g_assert (statement);
    compiler = vivi_code_compiler_new (7); // FIXME: version
    vivi_code_compiler_compile_statement (compiler, statement);
    g_object_unref (statement);
    asm2 = g_object_ref (vivi_code_compiler_get_assembler (compiler));
    g_object_unref (compiler);
    for (j = 0; j < vivi_code_assembler_get_n_codes (asm2); j++) {
      vivi_code_assembler_insert_code (assembler, i++,
	  vivi_code_assembler_get_code (asm2, j));
    }
    g_object_unref (asm2);
  }

  for (; i < vivi_code_assembler_get_n_codes (assembler); i++) {
    ViviCodeAsm *code = vivi_code_assembler_get_code (assembler, i);
    if (VIVI_IS_CODE_ASM_RANDOM (code)) {
      ViviCodeAsm *push = vivi_code_asm_push_new ();
      vivi_code_assembler_remove_code (assembler, code);
      INSERT_CODE (assembler, i, vivi_code_asm_pop_new ());
      vivi_code_asm_push_add_integer (VIVI_CODE_ASM_PUSH (push), 0);
      INSERT_CODE (assembler, i, push);
      i--;
    }
  }
}

static void
rewrite_getters (ViviCodeAssembler *assembler)
{
  guint i;

  for (i = 0; i < vivi_code_assembler_get_n_codes (assembler); i++) {
    ViviCodeAsm *code = vivi_code_assembler_get_code (assembler, i);
    if (VIVI_IS_CODE_ASM_GET_VARIABLE (code) ||
	VIVI_IS_CODE_ASM_GET_MEMBER (code)) {
      i++;
      INSERT_CODE (assembler, i, vivi_code_asm_push_duplicate_new ());
      INSERT_CODE (assembler, i, vivi_code_asm_trace_new ());
      i--;
    }
  }
}

/*** INFRASTRUCTURE ***/

static SwfdecBuffer *
do_script (SwfdecBuffer *buffer, guint flags, const char *name, guint version)
{
  ViviCodeAssembler *assembler;
  SwfdecScript *script;
  GError *error = NULL;

  script = swfdec_script_new (buffer, name, version);

  assembler = VIVI_CODE_ASSEMBLER (vivi_disassemble_script (script));
  swfdec_script_unref (script);

  if (flags & REWRITE_TRACE_FUNCTION_NAME)
    insert_function_trace (assembler, name);
  if (flags & REWRITE_RANDOM)
    replace_random (assembler, flags & REWRITE_INIT);
  if (flags & REWRITE_GETTERS)
    rewrite_getters (assembler);

  script = vivi_code_assembler_assemble_script (assembler, version, &error);
  g_object_unref (assembler);
  if (script == NULL) {
    g_print ("error: %s\n", error->message);
    g_error_free (error);
    return NULL;
  }

  /* FIXME: We want swfdec_script_get_buffer() */
  g_assert (script->main == script->buffer->data);
  buffer = swfdec_buffer_ref (script->buffer);
  swfdec_script_unref (script);

  return buffer;
}

static SwfdecBuffer *
process_buffer (SwfdecBuffer *original, guint flags)
{
  SwfdecRect rect;
  SwfdecBits bits;
  SwfdecBots *bots, *full;
  SwfdecBuffer *buffer;
  guint version, tag, len;
  gboolean long_header, needs_init = TRUE;

  bots = swfdec_bots_open ();
  swfdec_bits_init (&bits, original);

  /* copy header */
  swfdec_bits_get_u8 (&bits);
  swfdec_bits_get_u8 (&bits);
  swfdec_bits_get_u8 (&bits);
  version = swfdec_bits_get_u8 (&bits);
  swfdec_bits_get_u32 (&bits);
  swfdec_bits_get_rect (&bits, &rect);
  swfdec_bots_put_rect (bots, &rect);
  swfdec_bots_put_u16 (bots, swfdec_bits_get_u16 (&bits));
  swfdec_bots_put_u16 (bots, swfdec_bits_get_u16 (&bits));

  while (swfdec_bits_left (&bits)) {
    len = swfdec_bits_get_u16 (&bits);
    tag = (len >> 6);
    len = len & 0x3f;
    long_header = (len == 0x3f);
    if (long_header)
      len = swfdec_bits_get_u32 (&bits);

    buffer = swfdec_bits_get_buffer (&bits, len);
    if (buffer == NULL) {
      swfdec_bots_free (bots);
      swfdec_buffer_unref (original);
      return NULL;
    }

    switch (tag) {
      case SWFDEC_TAG_DOINITACTION:
	{
	  SwfdecBuffer *sub = swfdec_buffer_new_subbuffer (buffer, 2, buffer->length - 2);
	  SwfdecBots *bots2 = swfdec_bots_open ();
	  guint sprite = buffer->data[0] | buffer->data[1] << 8;
	  char *name = g_strdup_printf ("DoInitAction %u", sprite);
	  swfdec_bots_put_u16 (bots2, sprite);
	  sub = do_script (sub, flags | (needs_init ? REWRITE_INIT : 0), name, version);
	  /* FIXME: we should add init when there's no init actions, too */
	  needs_init = FALSE;
	  g_free (name);
	  if (sub == NULL) {
	    swfdec_bots_free (bots2);
	    swfdec_bots_free (bots);
	    swfdec_buffer_unref (original);
	    return NULL;
	  }
	  swfdec_bots_put_buffer (bots2, sub);
	  swfdec_buffer_unref (sub);
	  swfdec_buffer_unref (buffer);
	  buffer = swfdec_bots_close (bots2);
	}
	break;
      case SWFDEC_TAG_DOACTION:
	buffer = do_script (buffer, flags, "DoAction", version);
	if (buffer == NULL) {
	  swfdec_bots_free (bots);
	  swfdec_buffer_unref (original);
	  return NULL;
	}
	break;
      default:
	break;
    }
    long_header |= buffer->length >= 0x3f;
    swfdec_bots_put_u16 (bots, tag << 6 | (long_header ? 0x3f : buffer->length));
    if (long_header)
      swfdec_bots_put_u32 (bots, buffer->length);
    swfdec_bots_put_buffer (bots, buffer);
    swfdec_buffer_unref (buffer);
  }

  swfdec_buffer_unref (original);
  full = swfdec_bots_open ();
  swfdec_bots_put_u8 (full, 'F');
  swfdec_bots_put_u8 (full, 'W');
  swfdec_bots_put_u8 (full, 'S');
  swfdec_bots_put_u8 (full, version);
  swfdec_bots_put_u32 (full, swfdec_bots_get_bytes (bots) + 8);
  swfdec_bots_put_bots (full, bots);
  swfdec_bots_free (bots);
  return swfdec_bots_close (full);
}

static SwfdecBuffer *
buffer_decode (SwfdecBuffer *buffer)
{
  SwfdecBits bits;
  SwfdecBots *bots;
  SwfdecBuffer *decoded;
  guint u;

  swfdec_bits_init (&bits, buffer);
  u = swfdec_bits_get_u8 (&bits);
  if ((u != 'C' && u != 'F') ||
      swfdec_bits_get_u8 (&bits) != 'W' ||
      swfdec_bits_get_u8 (&bits) != 'S') {
    swfdec_buffer_unref (buffer);
    return NULL;
  }
  
  if (u == 'F')
    return buffer;

  bots = swfdec_bots_open ();
  swfdec_bots_put_u8 (bots, 'F');
  swfdec_bots_put_u8 (bots, 'W');
  swfdec_bots_put_u8 (bots, 'S');
  swfdec_bots_put_u8 (bots, swfdec_bits_get_u8 (&bits));
  u = swfdec_bits_get_u32 (&bits);
  g_assert (u <= G_MAXINT32);
  decoded = swfdec_bits_decompress (&bits, -1, u - 8);
  swfdec_buffer_unref (buffer);
  if (decoded == NULL)
    return NULL;
  swfdec_bots_put_u32 (bots, u);
  swfdec_bots_put_buffer (bots, decoded);
  swfdec_buffer_unref (decoded);
  buffer = swfdec_bots_close (bots);
  return buffer;
}

int 
main (int argc, char *argv[])
{
  GError *error = NULL;
  SwfdecBuffer *buffer;
  gboolean trace_function_names = FALSE;
  gboolean random = FALSE;
  gboolean getters = FALSE;

  GOptionEntry options[] = {
    { "getters", 'g', 0, G_OPTION_ARG_NONE, &getters, "trace all variable get operations", NULL },
    { "trace-function-names", 'n', 0, G_OPTION_ARG_NONE, &trace_function_names, "trace names of called functions", NULL },
    { "no-random", 'r', 0, G_OPTION_ARG_NONE, &random, "replace all random values with 0", NULL },
    { NULL }
  };
  GOptionContext *ctx;

  ctx = g_option_context_new ("");
  g_option_context_add_main_entries (ctx, options, "options");
  g_option_context_parse (ctx, &argc, &argv, &error);
  g_option_context_free (ctx);

  if (error) {
    g_printerr ("Error parsing command line arguments: %s\n", error->message);
    g_error_free (error);
    return 1;
  }

  if (argc < 3) {
    g_printerr ("%s [OPTIONS] INFILE OUTFILE\n", argv[0]);
    return 1;
  }

  swfdec_init ();

  buffer = swfdec_buffer_new_from_file (argv[1], &error);
  if (buffer == NULL) {
    g_printerr ("Error loading: %s\n", error->message);
    g_error_free (error);
    return 1;
  }
  buffer = buffer_decode (buffer);
  if (buffer == NULL) {
    g_printerr ("\"%s\" is not a Flash file\n", argv[1]);
    return 1;
  }

  buffer = process_buffer (buffer,
      (trace_function_names ? REWRITE_TRACE_FUNCTION_NAME : 0) |
      (random ? REWRITE_RANDOM : 0) |
      (getters ? REWRITE_GETTERS : 0));
  if (buffer == NULL) {
    g_printerr ("\"%s\": Broken Flash file\n", argv[1]);
    return 1;
  }

  if (!g_file_set_contents (argv[2], (char *) buffer->data, buffer->length, &error)) {
    swfdec_buffer_unref (buffer);
    g_printerr ("Error saving: %s\n", error->message);
    return 1;
  }
  
  swfdec_buffer_unref (buffer);
  return 0;
}

