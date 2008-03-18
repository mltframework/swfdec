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

#include "vivi_code_statement.h"

G_DEFINE_ABSTRACT_TYPE (ViviCodeStatement, vivi_code_statement, VIVI_TYPE_CODE_TOKEN)

static void
vivi_code_statement_dispose (GObject *object)
{
  //ViviCodeStatement *statement = VIVI_CODE_STATEMENT (object);

  G_OBJECT_CLASS (vivi_code_statement_parent_class)->dispose (object);
}

static void
vivi_code_statement_class_init (ViviCodeStatementClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_code_statement_dispose;
}

static void
vivi_code_statement_init (ViviCodeStatement *token)
{
}

