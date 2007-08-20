/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include <libswfdec/swfdec_as_scope.h>
#include <libswfdec/swfdec_as_types.h>
#include <libswfdec/swfdec_script_internal.h>

G_BEGIN_DECLS

struct _SwfdecAsFrame {
  SwfdecAsScope		scope_object;

  SwfdecAsFrame *	next;		/* next frame (FIXME: keep a list in the context instead?) */
  SwfdecAsFunction *	function;	/* function we're executing or NULL if toplevel */
  SwfdecAsObject *	thisp;		/* this object in current frame or NULL if none */
  SwfdecAsObject *	super;		/* super object in current frame or NULL if none */
  gboolean		construct;	/* TRUE if this is the constructor for thisp */
  SwfdecAsValue *	return_value;	/* pointer to where to store the return value */
  guint			argc;		/* number of arguments */
  const SwfdecAsValue *	argv;		/* arguments or %NULL if taken from stack */
  /* debugging */
  const char *		function_name;	/* name of function */
  /* script execution */
  SwfdecScript *	script;		/* script being executed */
  SwfdecAsScope *	scope;		/* first object in scope chain (either this frame or a with object) */
  SwfdecAsObject *	target;		/* target to use as last object in scope chain or for SetVariable */
  SwfdecAsObject *	original_target;/* original target (used when resetting target) */
  gboolean		is_local;	/* TRUE if this frame takes local variables */
  SwfdecAsValue *	registers;	/* the registers */
  guint			n_registers;	/* number of allocated registers */
  SwfdecConstantPool *	constant_pool;	/* constant pool currently in use */
  SwfdecBuffer *	constant_pool_buffer;	/* buffer containing the raw data for constant_pool */
  SwfdecAsValue *	stack_begin;	/* beginning of stack */
  guint8 *		pc;		/* program counter on stack */
  /* native function */
};

struct _SwfdecAsFrameClass {
  SwfdecAsScopeClass	scope_class;
};

SwfdecAsFrame *	swfdec_as_frame_new		(SwfdecAsContext *	context,
						 SwfdecScript *		script);
SwfdecAsFrame *	swfdec_as_frame_new_native	(SwfdecAsContext *	context);
void		swfdec_as_frame_return		(SwfdecAsFrame *	frame,
						 SwfdecAsValue *	return_value);

void		swfdec_as_frame_set_this	(SwfdecAsFrame *	frame,
						 SwfdecAsObject *	thisp);
void		swfdec_as_frame_preload		(SwfdecAsFrame *	frame);

SwfdecAsObject *swfdec_as_frame_find_variable	(SwfdecAsFrame *	frame,
						 const char *		variable);
SwfdecAsDeleteReturn
		swfdec_as_frame_delete_variable	(SwfdecAsFrame *	frame,
						 const char *		variable);

void		swfdec_as_frame_set_target	(SwfdecAsFrame *	frame,
						 SwfdecAsObject *	target);
void		swfdec_as_frame_check_scope	(SwfdecAsFrame *	frame);


G_END_DECLS
#endif
