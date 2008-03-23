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

#include <vivified/code/vivi_code_block.h>
#include <vivified/code/vivi_decompiler_state.h>

G_BEGIN_DECLS


typedef struct _ViviDecompilerBlock ViviDecompilerBlock;
typedef struct _ViviDecompilerBlockClass ViviDecompilerBlockClass;

#define VIVI_TYPE_DECOMPILER_BLOCK                    (vivi_decompiler_block_get_type())
#define VIVI_IS_DECOMPILER_BLOCK(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_DECOMPILER_BLOCK))
#define VIVI_IS_DECOMPILER_BLOCK_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_DECOMPILER_BLOCK))
#define VIVI_DECOMPILER_BLOCK(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_DECOMPILER_BLOCK, ViviDecompilerBlock))
#define VIVI_DECOMPILER_BLOCK_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_DECOMPILER_BLOCK, ViviDecompilerBlockClass))
#define VIVI_DECOMPILER_BLOCK_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_DECOMPILER_BLOCK, ViviDecompilerBlockClass))

struct _ViviDecompilerBlock
{
  ViviCodeBlock		block;

  ViviDecompilerState *	start;		/* starting state */
  guint			incoming;	/* number of incoming blocks */
  const guint8 *	startpc;	/* pointer to first command in block */
  /* set by parsing the block */
  ViviDecompilerState *	end;		/* pointer to after last parsed command or NULL if not parsed yet */
  ViviDecompilerBlock *	next;		/* block following this one or NULL if returning */
  ViviDecompilerBlock *	branch;		/* NULL or block branched to i if statement */
  ViviCodeValue *	branch_condition;/* NULL or value for deciding if a branch should be taken */
};

struct _ViviDecompilerBlockClass
{
  ViviCodeBlockClass	block_class;
};

GType			vivi_decompiler_block_get_type   	(void);

ViviDecompilerBlock *	vivi_decompiler_block_new		(ViviDecompilerState *		state);
void			vivi_decompiler_block_reset		(ViviDecompilerBlock *		block,
								 gboolean			generalize_start_state);

ViviCodeStatement *	vivi_decompiler_block_get_label		(ViviDecompilerBlock *  	block);
void			vivi_decompiler_block_force_label	(ViviDecompilerBlock *		block);
const guint8 *		vivi_decompiler_block_get_start		(ViviDecompilerBlock *		block);
const ViviDecompilerState *
			vivi_decompiler_block_get_start_state	(ViviDecompilerBlock *	        block);
const ViviDecompilerState *
			vivi_decompiler_block_get_end_state	(ViviDecompilerBlock *	        block);
gboolean		vivi_decompiler_block_contains		(ViviDecompilerBlock *  	block,
								 const guint8 *			pc);
void			vivi_decompiler_block_finish		(ViviDecompilerBlock *		block,
								 const ViviDecompilerState *	state);
gboolean		vivi_decompiler_block_is_finished	(ViviDecompilerBlock *		block);

guint			vivi_decompiler_block_get_n_incoming	(ViviDecompilerBlock *		block);
void			vivi_decompiler_block_set_next		(ViviDecompilerBlock *		block,
								 ViviDecompilerBlock *		next);
ViviDecompilerBlock *	vivi_decompiler_block_get_next		(ViviDecompilerBlock *		block);
void			vivi_decompiler_block_set_branch	(ViviDecompilerBlock *		block,
								 ViviDecompilerBlock *		branch,
								 ViviCodeValue *		branch_condition);
ViviDecompilerBlock *	vivi_decompiler_block_get_branch	(ViviDecompilerBlock *		block);
ViviCodeValue *		vivi_decompiler_block_get_branch_condition
								(ViviDecompilerBlock *		block);
void			vivi_decompiler_block_add_error		(ViviDecompilerBlock *		block,
								 const char *			format,
								 ...) G_GNUC_PRINTF (2, 3);

void			vivi_decompiler_block_add_to_block	(ViviDecompilerBlock *		block,
								 ViviCodeBlock *		target);
void			vivi_decompiler_block_add_state_transition 
								(ViviDecompilerBlock *		from,
								 ViviDecompilerBlock *		to,
								 ViviCodeBlock *		block);


G_END_DECLS
#endif
