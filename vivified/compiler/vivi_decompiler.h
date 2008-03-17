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

#ifndef _VIVI_DECOMPILER_H_
#define _VIVI_DECOMPILER_H_

#include <swfdec/swfdec.h>

G_BEGIN_DECLS


typedef struct _ViviDecompiler ViviDecompiler;
typedef struct _ViviDecompilerClass ViviDecompilerClass;

#define VIVI_TYPE_DECOMPILER                    (vivi_decompiler_get_type())
#define VIVI_IS_DECOMPILER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_DECOMPILER))
#define VIVI_IS_DECOMPILER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_DECOMPILER))
#define VIVI_DECOMPILER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_DECOMPILER, ViviDecompiler))
#define VIVI_DECOMPILER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_DECOMPILER, ViviDecompilerClass))
#define VIVI_DECOMPILER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_DECOMPILER, ViviDecompilerClass))

struct _ViviDecompiler
{
  SwfdecAsObject	object;

  SwfdecScript *	script;		/* script that we decompile */
  GList *		blocks;		/* list of all blocks in this script ordered by pc (should be one after decompilation is done) */
};

struct _ViviDecompilerClass
{
  SwfdecAsObjectClass	object_class;
};

GType			vivi_decompiler_get_type   	(void);

ViviDecompiler *	vivi_decompiler_new		(SwfdecScript *		script);
guint			vivi_decompiler_get_n_lines	(ViviDecompiler *	dec);
const char *		vivi_decompiler_get_line	(ViviDecompiler *	dec,
							 guint			i);

G_END_DECLS
#endif
