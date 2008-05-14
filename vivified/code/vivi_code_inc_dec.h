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

#ifndef _VIVI_CODE_INC_DEC_H_
#define _VIVI_CODE_INC_DEC_H_

#include <vivified/code/vivi_code_value.h>
#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviCodeIncDec ViviCodeIncDec;
typedef struct _ViviCodeIncDecClass ViviCodeIncDecClass;

#define VIVI_TYPE_CODE_INC_DEC                    (vivi_code_inc_dec_get_type())
#define VIVI_IS_CODE_INC_DEC(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_INC_DEC))
#define VIVI_IS_CODE_INC_DEC_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_INC_DEC))
#define VIVI_CODE_INC_DEC(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_INC_DEC, ViviCodeIncDec))
#define VIVI_CODE_INC_DEC_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_INC_DEC, ViviCodeIncDecClass))
#define VIVI_CODE_INC_DEC_INC_DEC_CLASS(obj)          (G_TYPE_INSTANCE_INC_DEC_CLASS ((obj), VIVI_TYPE_CODE_INC_DEC, ViviCodeIncDecClass))

struct _ViviCodeIncDec
{
  ViviCodeValue		token;

  ViviCodeValue *	from;
  ViviCodeValue *	name;
  gboolean		increment;
  gboolean		pre_assignment;
};

struct _ViviCodeIncDecClass
{
  ViviCodeValueClass	value_class;
};

GType			vivi_code_inc_dec_get_type		(void);

ViviCodeValue *		vivi_code_inc_dec_new			(ViviCodeValue *	name,
								 ViviCodeValue *	from,
								 gboolean		increment,
								 gboolean		pre_assignment);


G_END_DECLS
#endif
