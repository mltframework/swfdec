/* SwfdecAs
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

#ifndef _SWFDEC_AS_STACK_H_
#define _SWFDEC_AS_STACK_H_

#include <libswfdec/swfdec_as_types.h>

G_BEGIN_DECLS

struct _SwfdecAsStack {
  SwfdecAsContext *	context;	/* context we operate on */

  SwfdecAsValue	*	base;		/* stack base */
  SwfdecAsValue	*	end;		/* end of stack */
  SwfdecAsValue	*	cur;		/* pointer to current top of stack */

  SwfdecAsStack *	next;		/* pointer to next stack area */
};

SwfdecAsStack *	swfdec_as_stack_new		(SwfdecAsContext *	context,
						 guint	  		n_elements);
void		swfdec_as_stack_free		(SwfdecAsStack *	stack);

#define swfdec_as_stack_peek(stack,n) (&(stack)->cur[-(gssize)(n)])
#define swfdec_as_stack_pop(stack) (--(stack)->cur)
#define swfdec_as_stack_push(stack,val) (*(stack)->cur++)

void		swfdec_as_stack_mark		(SwfdecAsStack *	stack);
void		swfdec_as_stack_ensure_size	(SwfdecAsStack *	stack,
						 guint	  		n_elements);
void		swfdec_as_stack_ensure_left	(SwfdecAsStack *	stack,
						 guint	  		n_elements);
						

G_END_DECLS
#endif
