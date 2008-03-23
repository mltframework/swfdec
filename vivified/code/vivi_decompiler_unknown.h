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

#ifndef _VIVI_DECOMPILER_UNKNOWN_H_
#define _VIVI_DECOMPILER_UNKNOWN_H_

#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviDecompilerUnknown ViviDecompilerUnknown;
typedef struct _ViviDecompilerUnknownClass ViviDecompilerUnknownClass;

#define VIVI_TYPE_DECOMPILER_UNKNOWN                    (vivi_decompiler_unknown_get_type())
#define VIVI_IS_DECOMPILER_UNKNOWN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_DECOMPILER_UNKNOWN))
#define VIVI_IS_DECOMPILER_UNKNOWN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_DECOMPILER_UNKNOWN))
#define VIVI_DECOMPILER_UNKNOWN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_DECOMPILER_UNKNOWN, ViviDecompilerUnknown))
#define VIVI_DECOMPILER_UNKNOWN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_DECOMPILER_UNKNOWN, ViviDecompilerUnknownClass))
#define VIVI_DECOMPILER_UNKNOWN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_DECOMPILER_UNKNOWN, ViviDecompilerUnknownClass))

struct _ViviDecompilerUnknown
{
  ViviCodeValue		parent;

  char *		name;
  ViviCodeValue *	value;
};

struct _ViviDecompilerUnknownClass
{
  ViviCodeValueClass	value_class;
};

GType			vivi_decompiler_unknown_get_type   	(void);

ViviCodeValue *		vivi_decompiler_unknown_new		(const char *		name);

void			vivi_decompiler_unknown_set_value	(ViviDecompilerUnknown *unknown,
								 ViviCodeValue *	value);
ViviCodeValue *		vivi_decompiler_unknown_get_value	(ViviDecompilerUnknown *unknown);
const char *		vivi_decompiler_unknown_get_name	(ViviDecompilerUnknown *unknown);


G_END_DECLS
#endif
