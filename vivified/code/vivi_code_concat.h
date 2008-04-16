/* Vivified
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _VIVI_CODE_CONCAT_H_
#define _VIVI_CODE_CONCAT_H_

#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviCodeConcat ViviCodeConcat;
typedef struct _ViviCodeConcatClass ViviCodeConcatClass;

#define VIVI_TYPE_CODE_CONCAT                    (vivi_code_concat_get_type())
#define VIVI_IS_CODE_CONCAT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_CONCAT))
#define VIVI_IS_CODE_CONCAT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_CONCAT))
#define VIVI_CODE_CONCAT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_CONCAT, ViviCodeConcat))
#define VIVI_CODE_CONCAT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_CONCAT, ViviCodeConcatClass))
#define VIVI_CODE_CONCAT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_CONCAT, ViviCodeConcatClass))

struct _ViviCodeConcat
{
  ViviCodeValue		value;

  ViviCodeValue *	first;
  ViviCodeValue *	second;
};

struct _ViviCodeConcatClass
{
  ViviCodeValueClass	value_class;
};

GType			vivi_code_concat_get_type   	(void);

ViviCodeValue *	vivi_code_concat_new		(ViviCodeValue *	first,
						 ViviCodeValue *	second);


G_END_DECLS
#endif
