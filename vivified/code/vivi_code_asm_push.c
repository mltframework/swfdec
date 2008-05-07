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

#include "vivi_code_asm_push.h"
#include "vivi_code_asm.h"
#include "vivi_code_emitter.h"
#include "vivi_code_error.h"
#include "vivi_code_printer.h"

static gboolean
vivi_code_asm_push_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  ViviCodeAsmPush *push = VIVI_CODE_ASM_PUSH (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);
  gsize len;

  len = swfdec_bots_get_bytes (push->contents);
  if (len > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"Push action too big");
    return FALSE;
  }
  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_PUSH);
  swfdec_bots_put_u16 (emit, len);
  swfdec_bots_put_bots (emit, push->contents);
  return TRUE;
}

static void
vivi_code_asm_push_asm_init (ViviCodeAsmInterface *iface)
{
  iface->emit = vivi_code_asm_push_emit;
}

G_DEFINE_TYPE_WITH_CODE (ViviCodeAsmPush, vivi_code_asm_push, VIVI_TYPE_CODE_ASM_CODE,
    G_IMPLEMENT_INTERFACE (VIVI_TYPE_CODE_ASM, vivi_code_asm_push_asm_init))

static void
vivi_code_asm_push_print (ViviCodeToken *token, ViviCodePrinter*printer)
{
  ViviCodeAsmPush *push = VIVI_CODE_ASM_PUSH (token);
  SwfdecBits bits;
  char *s, *s2;
  gboolean first = TRUE;
  guint type;

  vivi_code_printer_print (printer, "push");
  swfdec_bits_init_data (&bits, push->contents->data, swfdec_bots_get_bytes (push->contents));
  while (swfdec_bits_left (&bits)) {
    if (first) {
      vivi_code_printer_print (printer, " ");
      first = FALSE;
    } else {
      vivi_code_printer_print (printer, ", ");
    }
    type = swfdec_bits_get_u8 (&bits);
    switch (type) {
      case VIVI_CODE_CONSTANT_STRING:
	s = swfdec_bits_get_string (&bits, 7);
	s2 = vivi_code_escape_string (s);
	g_free (s);
	vivi_code_printer_print (printer, s2);
	g_free (s2);
	break;
      case VIVI_CODE_CONSTANT_FLOAT:
	{
	  s = g_malloc (G_ASCII_DTOSTR_BUF_SIZE);
	  g_ascii_dtostr (s, G_ASCII_DTOSTR_BUF_SIZE,
	      swfdec_bits_get_float (&bits));
	  vivi_code_printer_print (printer, s);
	  vivi_code_printer_print (printer, "f");
	  g_free (s);
	}
	break;
      case VIVI_CODE_CONSTANT_NULL:
	vivi_code_printer_print (printer, "null");
	break;
      case VIVI_CODE_CONSTANT_UNDEFINED:
	vivi_code_printer_print (printer, "undefined");
	break;
      case VIVI_CODE_CONSTANT_REGISTER:
	s = g_strdup_printf ("reg %u", swfdec_bits_get_u8 (&bits));
	vivi_code_printer_print (printer, s);
	g_free (s);
	break;
      case VIVI_CODE_CONSTANT_BOOLEAN:
	vivi_code_printer_print (printer, swfdec_bits_get_u8 (&bits) ? "true" : "false");
	break;
      case VIVI_CODE_CONSTANT_DOUBLE:
	{
	  double number = swfdec_bits_get_double (&bits);
	  s = g_malloc (G_ASCII_DTOSTR_BUF_SIZE);
	  g_ascii_dtostr (s, G_ASCII_DTOSTR_BUF_SIZE, number);
	  vivi_code_printer_print (printer, s);
	  if (number == swfdec_as_double_to_integer (number))
	    vivi_code_printer_print (printer, "d");
	  g_free (s);
	}
	break;
      case VIVI_CODE_CONSTANT_INTEGER:
	s = g_strdup_printf ("%d", swfdec_bits_get_s32 (&bits));
	vivi_code_printer_print (printer, s);
	g_free (s);
	break;
      case VIVI_CODE_CONSTANT_CONSTANT_POOL:
	s = g_strdup_printf ("pool %u", swfdec_bits_get_u8 (&bits));
	vivi_code_printer_print (printer, s);
	g_free (s);
	break;
      case VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG:
	/* FIXME: allow differentiation between pool type for values < 256? */
	s = g_strdup_printf ("pool %u", swfdec_bits_get_u16 (&bits));
	vivi_code_printer_print (printer, s);
	g_free (s);
	break;
      default:
	g_assert_not_reached ();
    }
  }
  vivi_code_printer_new_line (printer, FALSE);
}

static void
vivi_code_asm_push_dispose (GObject *object)
{
  ViviCodeAsmPush *push = VIVI_CODE_ASM_PUSH (object);

  swfdec_bots_free (push->contents);
  g_ptr_array_free (push->offsets, TRUE);

  G_OBJECT_CLASS (vivi_code_asm_push_parent_class)->dispose (object);
}

static void
vivi_code_asm_push_class_init (ViviCodeAsmPushClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);
  ViviCodeAsmCodeClass *code_class = VIVI_CODE_ASM_CODE_CLASS (klass);

  object_class->dispose = vivi_code_asm_push_dispose;

  token_class->print = vivi_code_asm_push_print;

  code_class->bytecode = SWFDEC_AS_ACTION_STORE_REGISTER;
}

static void
vivi_code_asm_push_init (ViviCodeAsmPush *push)
{
  push->contents = swfdec_bots_open ();
  push->offsets = g_ptr_array_new ();
}

ViviCodeAsm *
vivi_code_asm_push_new (void)
{
  return g_object_new (VIVI_TYPE_CODE_ASM_PUSH, NULL);
}

guint
vivi_code_asm_push_get_n_values (const ViviCodeAsmPush *push)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0);
  
  return push->offsets->len;
}

ViviCodeConstantType
vivi_code_asm_push_get_value_type (const ViviCodeAsmPush *push, guint i)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0);
  g_return_val_if_fail (i < push->offsets->len, 0);

  return push->contents->data[GPOINTER_TO_SIZE (g_ptr_array_index (push->offsets, i))];
}

void
vivi_code_asm_push_add_string (ViviCodeAsmPush *push, const char *string)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (string != NULL);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_STRING);
  swfdec_bots_put_string (push->contents, string);
}

void
vivi_code_asm_push_add_float (ViviCodeAsmPush *push, float f)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_FLOAT);
  swfdec_bots_put_float (push->contents, f);
}

void
vivi_code_asm_push_add_null (ViviCodeAsmPush *push)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_NULL);
}

void
vivi_code_asm_push_add_undefined (ViviCodeAsmPush *push)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_UNDEFINED);
}

void
vivi_code_asm_push_add_register (ViviCodeAsmPush *push, guint id)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (id < 256);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_REGISTER);
  swfdec_bots_put_u8 (push->contents, id);
}

void
vivi_code_asm_push_add_boolean (ViviCodeAsmPush *push, gboolean b)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_BOOLEAN);
  swfdec_bots_put_u8 (push->contents, b ? 1 : 0);
}

void
vivi_code_asm_push_add_double (ViviCodeAsmPush *push, double d)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_DOUBLE);
  swfdec_bots_put_double (push->contents, d);
}

void
vivi_code_asm_push_add_integer (ViviCodeAsmPush *push, int i)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (i >= G_MININT32 && i <= G_MAXINT32);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_INTEGER);
  swfdec_bots_put_u32 (push->contents, i);
}

void
vivi_code_asm_push_add_pool (ViviCodeAsmPush *push, guint id)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (id < 256);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_CONSTANT_POOL);
  swfdec_bots_put_u8 (push->contents, id);
}

void
vivi_code_asm_push_add_pool_big (ViviCodeAsmPush *push, guint id)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (id <= G_MAXUINT16);

  g_ptr_array_add (push->offsets, GSIZE_TO_POINTER (swfdec_bots_get_bytes (push->contents)));
  swfdec_bots_put_u8 (push->contents, VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG);
  swfdec_bots_put_u16 (push->contents, id);
}

void
vivi_code_asm_push_copy_value (ViviCodeAsmPush *push,
    const ViviCodeAsmPush *other, guint id)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (other));
  g_return_if_fail (id <= G_MAXUINT16);

  switch (vivi_code_asm_push_get_value_type (other, id)) {
    case VIVI_CODE_CONSTANT_STRING:
      vivi_code_asm_push_add_string (push,
	  vivi_code_asm_push_get_string (other, id));
      break;
    case VIVI_CODE_CONSTANT_FLOAT:
      vivi_code_asm_push_add_float (push,
	  vivi_code_asm_push_get_float (other, id));
      break;
    case VIVI_CODE_CONSTANT_NULL:
      vivi_code_asm_push_add_null (push);
      break;
    case VIVI_CODE_CONSTANT_UNDEFINED:
      vivi_code_asm_push_add_undefined (push);
      break;
    case VIVI_CODE_CONSTANT_REGISTER:
      vivi_code_asm_push_add_register (push,
	  vivi_code_asm_push_get_register (other, id));
      break;
    case VIVI_CODE_CONSTANT_BOOLEAN:
      vivi_code_asm_push_add_boolean (push,
	  vivi_code_asm_push_get_boolean (other, id));
      break;
    case VIVI_CODE_CONSTANT_DOUBLE:
      vivi_code_asm_push_add_double (push,
	  vivi_code_asm_push_get_double (other, id));
      break;
    case VIVI_CODE_CONSTANT_INTEGER:
      vivi_code_asm_push_add_integer (push,
	  vivi_code_asm_push_get_integer (other, id));
      break;
    case VIVI_CODE_CONSTANT_CONSTANT_POOL:
      vivi_code_asm_push_add_pool (push,
	  vivi_code_asm_push_get_pool (other, id));
      break;
    case VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG:
      vivi_code_asm_push_add_pool_big (push,
	  vivi_code_asm_push_get_pool (other, id));
      break;
    default:
      g_assert_not_reached ();
  }
}

const char *
vivi_code_asm_push_get_string (const ViviCodeAsmPush *push, guint id)
{
  SwfdecBits bits;

  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), NULL);
  g_return_val_if_fail (id < push->offsets->len, NULL);

  swfdec_bits_init_data (&bits, push->contents->data, swfdec_bots_get_bytes (push->contents));
  swfdec_bits_skip_bytes (&bits, GPOINTER_TO_SIZE (g_ptr_array_index (push->offsets, id)));
  g_return_val_if_fail (swfdec_bits_get_u8 (&bits) == VIVI_CODE_CONSTANT_STRING, NULL);

  return (char *) bits.ptr;
}

float
vivi_code_asm_push_get_float (const ViviCodeAsmPush *push, guint id)
{
  SwfdecBits bits;

  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0.0f);
  g_return_val_if_fail (id < push->offsets->len, 0.0f);

  swfdec_bits_init_data (&bits, push->contents->data, swfdec_bots_get_bytes (push->contents));
  swfdec_bits_skip_bytes (&bits, GPOINTER_TO_SIZE (g_ptr_array_index (push->offsets, id)));
  g_return_val_if_fail (swfdec_bits_get_u8 (&bits) == VIVI_CODE_CONSTANT_FLOAT, 0.0f);

  return swfdec_bits_get_float (&bits);
}

guint
vivi_code_asm_push_get_register (const ViviCodeAsmPush *push, guint id)
{
  SwfdecBits bits;

  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0);
  g_return_val_if_fail (id < push->offsets->len, 0);

  swfdec_bits_init_data (&bits, push->contents->data, swfdec_bots_get_bytes (push->contents));
  swfdec_bits_skip_bytes (&bits, GPOINTER_TO_SIZE (g_ptr_array_index (push->offsets, id)));
  g_return_val_if_fail (swfdec_bits_get_u8 (&bits) == VIVI_CODE_CONSTANT_REGISTER, 0);

  return swfdec_bits_get_u8 (&bits);
}

double
vivi_code_asm_push_get_double (const ViviCodeAsmPush *push, guint id)
{
  SwfdecBits bits;

  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0.0);
  g_return_val_if_fail (id < push->offsets->len, 0.0);

  swfdec_bits_init_data (&bits, push->contents->data, swfdec_bots_get_bytes (push->contents));
  swfdec_bits_skip_bytes (&bits, GPOINTER_TO_SIZE (g_ptr_array_index (push->offsets, id)));
  g_return_val_if_fail (swfdec_bits_get_u8 (&bits) == VIVI_CODE_CONSTANT_DOUBLE, 0.0);

  return swfdec_bits_get_double (&bits);
}

int
vivi_code_asm_push_get_integer (const ViviCodeAsmPush *push, guint id)
{
  SwfdecBits bits;

  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), FALSE);
  g_return_val_if_fail (id < push->offsets->len, FALSE);

  swfdec_bits_init_data (&bits, push->contents->data, swfdec_bots_get_bytes (push->contents));
  swfdec_bits_skip_bytes (&bits, GPOINTER_TO_SIZE (g_ptr_array_index (push->offsets, id)));
  g_return_val_if_fail (swfdec_bits_get_u8 (&bits) == VIVI_CODE_CONSTANT_INTEGER, FALSE);

  return swfdec_bits_get_s32 (&bits);
}

gboolean
vivi_code_asm_push_get_boolean (const ViviCodeAsmPush *push, guint id)
{
  SwfdecBits bits;

  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), FALSE);
  g_return_val_if_fail (id < push->offsets->len, FALSE);

  swfdec_bits_init_data (&bits, push->contents->data, swfdec_bots_get_bytes (push->contents));
  swfdec_bits_skip_bytes (&bits, GPOINTER_TO_SIZE (g_ptr_array_index (push->offsets, id)));
  g_return_val_if_fail (swfdec_bits_get_u8 (&bits) == VIVI_CODE_CONSTANT_BOOLEAN, FALSE);

  return swfdec_bits_get_u8 (&bits) ? TRUE : FALSE;
}

guint
vivi_code_asm_push_get_pool (const ViviCodeAsmPush *push, guint id)
{
  SwfdecBits bits;
  guint type;

  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0);
  g_return_val_if_fail (id < push->offsets->len, 0);

  swfdec_bits_init_data (&bits, push->contents->data, swfdec_bots_get_bytes (push->contents));
  swfdec_bits_skip_bytes (&bits, GPOINTER_TO_SIZE (g_ptr_array_index (push->offsets, id)));
  type = swfdec_bits_get_u8 (&bits);
  g_return_val_if_fail (type == VIVI_CODE_CONSTANT_CONSTANT_POOL ||
      type == VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG, 0);

  return type == VIVI_CODE_CONSTANT_CONSTANT_POOL ? swfdec_bits_get_u8 (&bits) : swfdec_bits_get_u16 (&bits);
}
