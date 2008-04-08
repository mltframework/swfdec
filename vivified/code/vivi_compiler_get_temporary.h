/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
 *               2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _VIVI_COMPILER_GET_TEMPORARY_H_
#define _VIVI_COMPILER_GET_TEMPORARY_H_

#include <vivified/code/vivi_code_get.h>

G_BEGIN_DECLS


typedef struct _ViviCompilerGetTemporary ViviCompilerGetTemporary;
typedef struct _ViviCompilerGetTemporaryClass ViviCompilerGetTemporaryClass;

#define VIVI_TYPE_COMPILER_GET_TEMPORARY                    (vivi_compiler_get_temporary_get_type())
#define VIVI_IS_COMPILER_GET_TEMPORARY(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_COMPILER_GET_TEMPORARY))
#define VIVI_IS_COMPILER_GET_TEMPORARY_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_COMPILER_GET_TEMPORARY))
#define VIVI_COMPILER_GET_TEMPORARY(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_COMPILER_GET_TEMPORARY, ViviCompilerGetTemporary))
#define VIVI_COMPILER_GET_TEMPORARY_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_COMPILER_GET_TEMPORARY, ViviCompilerGetTemporaryClass))
#define VIVI_COMPILER_GET_TEMPORARY_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_COMPILER_GET_TEMPORARY, ViviCompilerGetTemporaryClass))

struct _ViviCompilerGetTemporary
{
  ViviCodeGet		value;
};

struct _ViviCompilerGetTemporaryClass
{
  ViviCodeGetClass	value_class;
};

GType			vivi_compiler_get_temporary_get_type   	(void);

ViviCodeValue *		vivi_compiler_get_temporary_new		(void);


G_END_DECLS
#endif
