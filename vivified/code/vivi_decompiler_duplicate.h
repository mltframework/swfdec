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

#ifndef _VIVI_DECOMPILER_DUPLICATE_H_
#define _VIVI_DECOMPILER_DUPLICATE_H_

#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviDecompilerDuplicate ViviDecompilerDuplicate;
typedef struct _ViviDecompilerDuplicateClass ViviDecompilerDuplicateClass;

#define VIVI_TYPE_DECOMPILER_DUPLICATE                    (vivi_decompiler_duplicate_get_type())
#define VIVI_IS_DECOMPILER_DUPLICATE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_DECOMPILER_DUPLICATE))
#define VIVI_IS_DECOMPILER_DUPLICATE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_DECOMPILER_DUPLICATE))
#define VIVI_DECOMPILER_DUPLICATE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_DECOMPILER_DUPLICATE, ViviDecompilerDuplicate))
#define VIVI_DECOMPILER_DUPLICATE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_DECOMPILER_DUPLICATE, ViviDecompilerDuplicateClass))
#define VIVI_DECOMPILER_DUPLICATE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_DECOMPILER_DUPLICATE, ViviDecompilerDuplicateClass))

struct _ViviDecompilerDuplicate
{
  ViviCodeValue		parent;

  ViviCodeValue *	value;
};

struct _ViviDecompilerDuplicateClass
{
  ViviCodeValueClass	value_class;
};

GType			vivi_decompiler_duplicate_get_type   	(void);

ViviCodeValue *		vivi_decompiler_duplicate_new		(ViviCodeValue *	value);

ViviCodeValue *		vivi_decompiler_duplicate_get_value	(ViviDecompilerDuplicate *dupl);


G_END_DECLS
#endif
