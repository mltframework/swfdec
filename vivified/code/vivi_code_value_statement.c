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

#include "vivi_code_value_statement.h"

G_DEFINE_TYPE (ViviCodeValueStatement, vivi_code_value_statement, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_value_statement_dispose (GObject *object)
{
  ViviCodeValueStatement *stmt = VIVI_CODE_VALUE_STATEMENT (object);

  g_object_unref (stmt->value);

  G_OBJECT_CLASS (vivi_code_value_statement_parent_class)->dispose (object);
}

static char *
vivi_code_value_statement_to_code (ViviCodeToken *token)
{
  ViviCodeValueStatement *stmt = VIVI_CODE_VALUE_STATEMENT (token);
  char *s, *ret;

  s = vivi_code_token_to_code (VIVI_CODE_TOKEN (stmt->value));
  ret = g_strdup_printf ("  %s;\n", s);
  g_free (s);

  return ret;
}

static void
vivi_code_value_statement_class_init (ViviCodeValueStatementClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_value_statement_dispose;

  token_class->to_code = vivi_code_value_statement_to_code;
}

static void
vivi_code_value_statement_init (ViviCodeValueStatement *token)
{
}

ViviCodeToken *
vivi_code_value_statement_new (ViviCodeValue *value)
{
  ViviCodeValueStatement *stmt;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (value), NULL);

  stmt = g_object_new (VIVI_TYPE_CODE_VALUE_STATEMENT, NULL);
  stmt->value = value;

  return VIVI_CODE_TOKEN (stmt);
}

