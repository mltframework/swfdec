/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_AS_FRAME_INTERNAL_H_
#define _SWFDEC_AS_FRAME_INTERNAL_H_

#include <swfdec/swfdec_as_super.h>
#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_script_internal.h>

G_BEGIN_DECLS

typedef void (* SwfdecAsFrameBlockFunc) (SwfdecAsContext *cx, SwfdecAsFrame *frame, gpointer data);

struct _SwfdecAsFrame {
  SwfdecAsFrame *	next;		/* next frame (FIXME: keep a list in the context instead?) */
  SwfdecAsFunction *	function;	/* function we're executing or NULL if toplevel */
  SwfdecAsValue		thisp;		/* this object in current frame or undefined if none */
  SwfdecAsSuper *	super;		/* super object in current frame or NULL if none */
  gboolean		construct;	/* TRUE if this is the constructor for thisp */
  SwfdecAsValue *	return_value;	/* pointer to where to store the return value */
  guint			argc;		/* number of arguments */
  const SwfdecAsValue *	argv;		/* arguments or %NULL if taken from stack */
  /* script execution */
  SwfdecScript *	script;		/* script being executed */
  GSList *		scope_chain;  	/* the scope chain (with objects etc) */
  const guint8 *      	block_start;	/* start of current block */
  const guint8 *      	block_end;	/* end of current block */
  GArray *		blocks;		/* blocks we have entered (like With) */
  SwfdecAsObject *	target;		/* target to use as last object in scope chain or for SetVariable */
  SwfdecAsObject *	original_target;/* original target (used when resetting target) */
  SwfdecAsObject *	activation;	/* activation object or NULL if the frame takes no local variables */
  SwfdecAsValue *	registers;	/* the registers */
  guint			n_registers;	/* number of allocated registers */
  SwfdecConstantPool *	constant_pool;	/* constant pool currently in use */
  SwfdecAsValue *	stack_begin;	/* beginning of stack */
  const guint8 *	pc;		/* program counter on stack */
  /* native function */
};

void		swfdec_as_frame_init		(SwfdecAsFrame *	frame,
						 SwfdecAsContext *	context,
						 SwfdecScript *		script);
void		swfdec_as_frame_init_native	(SwfdecAsFrame *	frame,
						 SwfdecAsContext *	context);
void		swfdec_as_frame_return		(SwfdecAsFrame *	frame,
						 SwfdecAsValue *	return_value);

void		swfdec_as_frame_set_this	(SwfdecAsFrame *	frame,
						 SwfdecAsObject *	thisp);
void		swfdec_as_frame_preload		(SwfdecAsFrame *	frame);

#define swfdec_as_frame_get_variable(frame, variable, value) \
  swfdec_as_frame_get_variable_and_flags (frame, variable, value, NULL, NULL)
SwfdecAsObject *swfdec_as_frame_get_variable_and_flags 
						(SwfdecAsFrame *	frame,
						 const char *		variable,
						 SwfdecAsValue *	value,
						 guint *		flags,
						 SwfdecAsObject **	pobject);
#define swfdec_as_frame_set_variable(frame, variable, value, overwrite, local) \
  swfdec_as_frame_set_variable_and_flags (frame, variable, value, 0, overwrite, local)
void		swfdec_as_frame_set_variable_and_flags
						(SwfdecAsFrame *	frame,
						 const char *		variable,
						 const SwfdecAsValue *	value,
						 guint			default_flags,
						 gboolean		overwrite,
						 gboolean		local);
SwfdecAsDeleteReturn
		swfdec_as_frame_delete_variable	(SwfdecAsFrame *	frame,
						 const char *		variable);

void		swfdec_as_frame_set_target	(SwfdecAsFrame *	frame,
						 SwfdecAsObject *	target);
void		swfdec_as_frame_push_block	(SwfdecAsFrame *	frame,
						 const guint8 *		start,
						 const guint8 *		end,
						 SwfdecAsFrameBlockFunc	func,
						 gpointer		data);
void		swfdec_as_frame_pop_block	(SwfdecAsFrame *	frame,
						 SwfdecAsContext *	context);
void		swfdec_as_frame_handle_exception(SwfdecAsFrame *	frame);


G_END_DECLS
#endif
