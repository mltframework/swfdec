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

#ifndef _VIVI_DECOMPILER_BLOCK_H_
#define _VIVI_DECOMPILER_BLOCK_H_

#include <swfdec/swfdec.h>
#include <vivified/compiler/vivi_decompiler_state.h>

G_BEGIN_DECLS


typedef struct _ViviDecompilerBlock ViviDecompilerBlock;


void			vivi_decompiler_block_free	(ViviDecompilerBlock *		block);
void			vivi_decompiler_block_reset	(ViviDecompilerBlock *		block);
ViviDecompilerBlock *	vivi_decompiler_block_new	(ViviDecompilerState *		state);

const char *		vivi_decompiler_block_get_label	(const ViviDecompilerBlock *  	block);
void			vivi_decompiler_block_force_label(ViviDecompilerBlock *		block);
const ViviDecompilerState *
		  vivi_decompiler_block_get_start_state	(const ViviDecompilerBlock *    block);
const guint8 *		vivi_decompiler_block_get_start (const ViviDecompilerBlock *	block);
gboolean		vivi_decompiler_block_contains	(const ViviDecompilerBlock *  	block,
							 const guint8 *			pc);
void			vivi_decompiler_block_finish	(ViviDecompilerBlock *		block,
							 const ViviDecompilerState *	state);
gboolean		vivi_decompiler_block_is_finished (const ViviDecompilerBlock *	block);

guint			vivi_decompiler_block_get_n_incoming
							(const ViviDecompilerBlock *	block);
void			vivi_decompiler_block_set_next	(ViviDecompilerBlock *		block,
							 ViviDecompilerBlock *		next);
ViviDecompilerBlock *	vivi_decompiler_block_get_next	(ViviDecompilerBlock *		block);
void			vivi_decompiler_block_set_branch(ViviDecompilerBlock *		block,
							 ViviDecompilerBlock *		branch,
							 ViviDecompilerValue *		branch_condition);
ViviDecompilerBlock *	vivi_decompiler_block_get_branch(ViviDecompilerBlock *		block);
const ViviDecompilerValue *
			vivi_decompiler_block_get_branch_condition
							(ViviDecompilerBlock *		block);
void			vivi_decompiler_block_emit_error(ViviDecompilerBlock *		block,
							 const char *			format,
							 ...) G_GNUC_PRINTF (2, 3);
void			vivi_decompiler_block_emit	(ViviDecompilerBlock *		block,
							 ViviDecompilerState *		state,
							 const char *			format,
							 ...) G_GNUC_PRINTF (3, 4);
guint			vivi_decompiler_block_get_n_lines(ViviDecompilerBlock *		block);
const char *		vivi_decompiler_block_get_line	(ViviDecompilerBlock *		block,
							 guint				i);


G_END_DECLS
#endif
