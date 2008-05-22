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

typedef struct {
  ViviCodeConstantType		type;
  union {
    char *			v_string;
    float			v_float;
    guint			v_register;
    gboolean			v_boolean;
    double			v_double;
    int				v_integer;
    guint			v_constant_pool;
  };
} ViviCodeAsmPushValue;

static gboolean
vivi_code_asm_push_emit (ViviCodeAsm *code, ViviCodeEmitter *emitter, GError **error)
{
  ViviCodeAsmPush *push = VIVI_CODE_ASM_PUSH (code);
  SwfdecBots *emit = vivi_code_emitter_get_bots (emitter);
  SwfdecBots *bots = swfdec_bots_open ();
  guint i;
  gsize len;

  for (i = 0; i < push->values->len; i++) {
    ViviCodeAsmPushValue *value =
      &g_array_index (push->values, ViviCodeAsmPushValue, i);

    swfdec_bots_put_u8 (bots, value->type);

    switch (value->type) {
      case VIVI_CODE_CONSTANT_STRING:
	swfdec_bots_put_string (bots, value->v_string);
	break;
      case VIVI_CODE_CONSTANT_FLOAT:
	swfdec_bots_put_float (bots, value->v_float);
	break;
      case VIVI_CODE_CONSTANT_NULL:
	/* nothing */
	break;
      case VIVI_CODE_CONSTANT_UNDEFINED:
	/* nothing */
	break;
      case VIVI_CODE_CONSTANT_REGISTER:
	swfdec_bots_put_u8 (bots, value->v_register);
	break;
      case VIVI_CODE_CONSTANT_BOOLEAN:
	swfdec_bots_put_u8 (bots, value->v_boolean ? 1 : 0);
	break;
      case VIVI_CODE_CONSTANT_DOUBLE:
	swfdec_bots_put_double (bots, value->v_double);
	break;
      case VIVI_CODE_CONSTANT_INTEGER:
	swfdec_bots_put_u32 (bots, value->v_integer);
	break;
      case VIVI_CODE_CONSTANT_CONSTANT_POOL:
	swfdec_bots_put_u8 (bots, value->v_constant_pool);
	break;
      case VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG:
	swfdec_bots_put_u16 (bots, value->v_constant_pool);
	break;
      default:
	g_assert_not_reached ();
    }
  }

  len = swfdec_bots_get_bytes (bots);
  if (len > G_MAXUINT16) {
    g_set_error (error, VIVI_CODE_ERROR, VIVI_CODE_ERROR_SIZE,
	"Push action too big");
    swfdec_bots_free (bots);
    return FALSE;
  }
  swfdec_bots_put_u8 (emit, SWFDEC_AS_ACTION_PUSH);
  swfdec_bots_put_u16 (emit, len);
  swfdec_bots_put_bots (emit, bots);
  swfdec_bots_free (bots);
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
  char *s;
  guint i;

  vivi_code_printer_print (printer, "push ");

  for (i = 0; i < push->values->len; i++) {
    ViviCodeAsmPushValue *value =
      &g_array_index (push->values, ViviCodeAsmPushValue, i);

    if (i != 0)
      vivi_code_printer_print (printer, ", ");

    switch (value->type) {
      case VIVI_CODE_CONSTANT_STRING:
	s = vivi_code_escape_string (value->v_string);
	vivi_code_printer_print (printer, s);
	g_free (s);
	break;
      case VIVI_CODE_CONSTANT_FLOAT:
	s = g_malloc (G_ASCII_DTOSTR_BUF_SIZE);
	g_ascii_dtostr (s, G_ASCII_DTOSTR_BUF_SIZE, value->v_float);
	vivi_code_printer_print (printer, s);
	vivi_code_printer_print (printer, "f");
	g_free (s);
	break;
      case VIVI_CODE_CONSTANT_NULL:
	vivi_code_printer_print (printer, "null");
	break;
      case VIVI_CODE_CONSTANT_UNDEFINED:
	vivi_code_printer_print (printer, "undefined");
	break;
      case VIVI_CODE_CONSTANT_REGISTER:
	s = g_strdup_printf ("reg %u", value->v_register);
	vivi_code_printer_print (printer, s);
	g_free (s);
	break;
      case VIVI_CODE_CONSTANT_BOOLEAN:
	vivi_code_printer_print (printer, value->v_boolean ? "true" : "false");
	break;
      case VIVI_CODE_CONSTANT_DOUBLE:
	s = g_malloc (G_ASCII_DTOSTR_BUF_SIZE);
	g_ascii_dtostr (s, G_ASCII_DTOSTR_BUF_SIZE, value->v_double);
	vivi_code_printer_print (printer, s);
	if (value->v_double == swfdec_as_double_to_integer (value->v_double))
	  vivi_code_printer_print (printer, "d");
	g_free (s);
	break;
      case VIVI_CODE_CONSTANT_INTEGER:
	s = g_strdup_printf ("%d", value->v_integer);
	vivi_code_printer_print (printer, s);
	g_free (s);
	break;
      case VIVI_CODE_CONSTANT_CONSTANT_POOL:
	s = g_strdup_printf ("pool %u", value->v_constant_pool);
	vivi_code_printer_print (printer, s);
	g_free (s);
	break;
      case VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG:
	/* FIXME: allow differentiation between pool type for values < 256? */
	s = g_strdup_printf ("pool %u", value->v_constant_pool);
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
  guint i;

  for (i = 0; i < push->values->len; i++) {
    ViviCodeAsmPushValue *value =
      &g_array_index (push->values, ViviCodeAsmPushValue, i);
    if (value->type == VIVI_CODE_CONSTANT_STRING) {
      g_free (value->v_string);
    }
  }

  g_array_free (push->values, TRUE);

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

  code_class->bytecode = SWFDEC_AS_ACTION_PUSH;
}

static void
vivi_code_asm_push_init (ViviCodeAsmPush *push)
{
  push->values = g_array_new (FALSE, FALSE, sizeof(ViviCodeAsmPushValue));
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

  return push->values->len;
}

ViviCodeConstantType
vivi_code_asm_push_get_value_type (const ViviCodeAsmPush *push, guint i)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push),
      VIVI_CODE_CONSTANT_UNDEFINED);
  g_return_val_if_fail (i < push->values->len, VIVI_CODE_CONSTANT_UNDEFINED);

  return g_array_index (push->values, ViviCodeAsmPushValue, i).type;
}

static void
vivi_code_asm_push_insert_value (ViviCodeAsmPush *push, guint index_,
    ViviCodeAsmPushValue *value)
{
  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (value != NULL);

  if (index_ < push->values->len) {
    g_array_insert_val (push->values, index_, *value);
  } else {
    g_array_append_val (push->values, *value);
  }
}

void
vivi_code_asm_push_insert_string (ViviCodeAsmPush *push, guint index_,
    const char *string)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (string != NULL);

  value.type = VIVI_CODE_CONSTANT_STRING;
  value.v_string = g_strdup (string);
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_insert_float (ViviCodeAsmPush *push, guint index_, float f)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  value.type = VIVI_CODE_CONSTANT_FLOAT;
  value.v_float = f;
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_insert_null (ViviCodeAsmPush *push, guint index_)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  value.type = VIVI_CODE_CONSTANT_NULL;
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_insert_undefined (ViviCodeAsmPush *push, guint index_)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  value.type = VIVI_CODE_CONSTANT_UNDEFINED;
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_insert_register (ViviCodeAsmPush *push, guint index_,
    guint id)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (id < 256);

  value.type = VIVI_CODE_CONSTANT_REGISTER;
  value.v_register = id;
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_insert_boolean (ViviCodeAsmPush *push, guint index_,
    gboolean b)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  value.type = VIVI_CODE_CONSTANT_BOOLEAN;
  value.v_boolean = b;
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_insert_double (ViviCodeAsmPush *push, guint index_,
    double d)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));

  value.type = VIVI_CODE_CONSTANT_DOUBLE;
  value.v_double = d;
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_insert_integer (ViviCodeAsmPush *push, guint index_, int i)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (i >= G_MININT32 && i <= G_MAXINT32);

  value.type = VIVI_CODE_CONSTANT_INTEGER;
  value.v_integer = i;
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_insert_pool (ViviCodeAsmPush *push, guint index_, guint id)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (id < 256);

  value.type = VIVI_CODE_CONSTANT_CONSTANT_POOL;
  value.v_constant_pool = id;
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_insert_pool_big (ViviCodeAsmPush *push, guint index_, guint id)
{
  ViviCodeAsmPushValue value;

  g_return_if_fail (VIVI_IS_CODE_ASM_PUSH (push));
  g_return_if_fail (id <= G_MAXUINT16);

  value.type = VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG;
  value.v_constant_pool = id;
  vivi_code_asm_push_insert_value (push, index_, &value);
}

void
vivi_code_asm_push_copy_value (ViviCodeAsmPush *push,
    const ViviCodeAsmPush *other, guint index_)
{
  ViviCodeAsmPushValue *value =
    &g_array_index (other->values, ViviCodeAsmPushValue, index_);

  if (value->type == VIVI_CODE_CONSTANT_STRING) {
    ViviCodeAsmPushValue value_new;

    value_new.type = VIVI_CODE_CONSTANT_STRING;
    value_new.v_string = g_strdup (value->v_string);

    vivi_code_asm_push_insert_value (push, G_MAXUINT, &value_new);
  } else {
    vivi_code_asm_push_insert_value (push, G_MAXUINT, value);
  }
}

const char *
vivi_code_asm_push_get_string (const ViviCodeAsmPush *push, guint index_)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), NULL);
  g_return_val_if_fail (index_ < push->values->len, NULL);
  g_return_val_if_fail (vivi_code_asm_push_get_value_type (push, index_)
      == VIVI_CODE_CONSTANT_STRING, NULL);

  return g_array_index (push->values, ViviCodeAsmPushValue, index_).v_string;
}

float
vivi_code_asm_push_get_float (const ViviCodeAsmPush *push, guint index_)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0.0f);
  g_return_val_if_fail (index_ < push->values->len, 0.0f);
  g_return_val_if_fail (vivi_code_asm_push_get_value_type (push, index_)
      == VIVI_CODE_CONSTANT_FLOAT, 0.0f);

  return g_array_index (push->values, ViviCodeAsmPushValue, index_).v_float;
}

guint
vivi_code_asm_push_get_register (const ViviCodeAsmPush *push, guint index_)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0);
  g_return_val_if_fail (index_ < push->values->len, 0);
  g_return_val_if_fail (vivi_code_asm_push_get_value_type (push, index_)
      == VIVI_CODE_CONSTANT_REGISTER, 0);

  return g_array_index (push->values, ViviCodeAsmPushValue, index_).v_register;
}

double
vivi_code_asm_push_get_double (const ViviCodeAsmPush *push, guint index_)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0.0);
  g_return_val_if_fail (index_ < push->values->len, 0.0);
  g_return_val_if_fail (vivi_code_asm_push_get_value_type (push, index_)
      == VIVI_CODE_CONSTANT_DOUBLE, 0);

  return g_array_index (push->values, ViviCodeAsmPushValue, index_).v_double;
}

int
vivi_code_asm_push_get_integer (const ViviCodeAsmPush *push, guint index_)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0);
  g_return_val_if_fail (index_ < push->values->len, 0);
  g_return_val_if_fail (vivi_code_asm_push_get_value_type (push, index_)
      == VIVI_CODE_CONSTANT_INTEGER, 0);

  return g_array_index (push->values, ViviCodeAsmPushValue, index_).v_integer;
}

gboolean
vivi_code_asm_push_get_boolean (const ViviCodeAsmPush *push, guint index_)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), FALSE);
  g_return_val_if_fail (index_ < push->values->len, FALSE);
  g_return_val_if_fail (vivi_code_asm_push_get_value_type (push, index_)
      == VIVI_CODE_CONSTANT_BOOLEAN, FALSE);

  return g_array_index (push->values, ViviCodeAsmPushValue, index_).v_boolean;
}

guint
vivi_code_asm_push_get_pool (const ViviCodeAsmPush *push, guint index_)
{
  g_return_val_if_fail (VIVI_IS_CODE_ASM_PUSH (push), 0);
  g_return_val_if_fail (index_ < push->values->len, 0);
  g_return_val_if_fail (vivi_code_asm_push_get_value_type (push, index_) ==
      VIVI_CODE_CONSTANT_CONSTANT_POOL ||
      vivi_code_asm_push_get_value_type (push, index_) ==
      VIVI_CODE_CONSTANT_CONSTANT_POOL_BIG, FALSE);

  return
    g_array_index (push->values, ViviCodeAsmPushValue, index_).v_constant_pool;
}
