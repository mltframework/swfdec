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

#ifndef _VIVI_CODE_NUMBER_H_
#define _VIVI_CODE_NUMBER_H_

#include <vivified/code/vivi_code_constant.h>

G_BEGIN_DECLS


typedef struct _ViviCodeNumber ViviCodeNumber;
typedef struct _ViviCodeNumberClass ViviCodeNumberClass;

#define VIVI_TYPE_CODE_NUMBER                    (vivi_code_number_get_type())
#define VIVI_IS_CODE_NUMBER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_NUMBER))
#define VIVI_IS_CODE_NUMBER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_NUMBER))
#define VIVI_CODE_NUMBER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_NUMBER, ViviCodeNumber))
#define VIVI_CODE_NUMBER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_NUMBER, ViviCodeNumberClass))
#define VIVI_CODE_NUMBER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_NUMBER, ViviCodeNumberClass))

struct _ViviCodeNumber
{
  ViviCodeConstant	constant;

  double		value;
};

struct _ViviCodeNumberClass
{
  ViviCodeConstantClass constant_class;
};

GType		vivi_code_number_get_type   	(void);

ViviCodeValue *	vivi_code_number_new		(double			value);

double		vivi_code_number_get_value	(ViviCodeNumber *	number);


G_END_DECLS
#endif
