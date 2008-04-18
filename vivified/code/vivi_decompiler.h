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
#include <vivified/code/vivi_code_block.h>

G_BEGIN_DECLS


ViviCodeStatement *	vivi_decompile_script		(SwfdecScript *		script);

ViviCodeStatement *	vivi_disassemble_script		(SwfdecScript *		script);


G_END_DECLS
#endif
