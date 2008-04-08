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

#ifndef _VIVI_CODE_INIT_ARRAY_H_
#define _VIVI_CODE_INIT_ARRAY_H_

#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviCodeInitArray ViviCodeInitArray;
typedef struct _ViviCodeInitArrayClass ViviCodeInitArrayClass;

#define VIVI_TYPE_CODE_INIT_ARRAY                    (vivi_code_init_array_get_type())
#define VIVI_IS_CODE_INIT_ARRAY(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_INIT_ARRAY))
#define VIVI_IS_CODE_INIT_ARRAY_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_INIT_ARRAY))
#define VIVI_CODE_INIT_ARRAY(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_INIT_ARRAY, ViviCodeInitArray))
#define VIVI_CODE_INIT_ARRAY_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_INIT_ARRAY, ViviCodeInitArrayClass))
#define VIVI_CODE_INIT_ARRAY_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_INIT_ARRAY, ViviCodeInitArrayClass))

struct _ViviCodeInitArray
{
  ViviCodeValue		value;

  GPtrArray *		variables;
};

struct _ViviCodeInitArrayClass
{
  ViviCodeValueClass	value_class;
};

GType			vivi_code_init_array_get_type   	(void);

ViviCodeValue *		vivi_code_init_array_new		(void);

void			vivi_code_init_array_add_variable	(ViviCodeInitArray *	array,
								 ViviCodeValue *	value);

G_END_DECLS
#endif
