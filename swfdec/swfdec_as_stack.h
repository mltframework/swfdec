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

#ifndef _SWFDEC_AS_STACK_H_
#define _SWFDEC_AS_STACK_H_

#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

struct _SwfdecAsStack {
  guint			n_elements;	/* number of elements */
  guint			used_elements;	/* elements in use (only set on stored stack) */
  SwfdecAsStack *	next;		/* pointer to next stack */
  SwfdecAsValue		elements[0];	/* the elements */
};

gboolean	swfdec_as_stack_push_segment  	(SwfdecAsContext *	context);
void		swfdec_as_stack_pop_segment   	(SwfdecAsContext *	context);

//#define SWFDEC_MAD_CHECKS
#ifdef SWFDEC_MAD_CHECKS
#include <swfdec/swfdec_as_context.h>
static inline SwfdecAsValue *
swfdec_as_stack_peek (SwfdecAsContext *cx, guint n)
{
  g_assert (cx != NULL);
  g_assert (n > 0);
  g_assert (cx->cur - n >= cx->base);

  return &(cx)->cur[-(gssize)(n)];
}

static inline SwfdecAsValue *
swfdec_as_stack_pop (SwfdecAsContext *cx)
{
  g_assert (cx != NULL);
  g_assert (cx->cur > cx->base);

  return --cx->cur;
}

static inline SwfdecAsValue *
swfdec_as_stack_pop_n (SwfdecAsContext *cx, guint n)
{
  g_assert (cx != NULL);
  g_assert ((guint) (cx->cur - cx->base) >= n);

  return cx->cur -= n;
}

static inline SwfdecAsValue *
swfdec_as_stack_push (SwfdecAsContext *cx)
{
  g_assert (cx != NULL);
  g_assert (cx->cur < cx->end);

  cx->cur->type = SWFDEC_AS_TYPE_OBJECT + 1;
  return cx->cur++;
}
#else /* SWFDEC_MAD_CHECKS */
#define swfdec_as_stack_peek(cx,n) (&(cx)->cur[-(gssize)(n)])
#define swfdec_as_stack_pop(cx) (--(cx)->cur)
#define swfdec_as_stack_pop_n(cx, n) ((cx)->cur -= (n))
#define swfdec_as_stack_push(cx) ((cx)->cur++)
#endif
#define swfdec_as_stack_swap(cx,x,y) G_STMT_START { \
  SwfdecAsContext *__cx = (cx); \
  SwfdecAsValue __tmp; \
  guint __x = (x), __y = (y); \
  __tmp = *swfdec_as_stack_peek(__cx, __x); \
  *swfdec_as_stack_peek(__cx, __x) = *swfdec_as_stack_peek(__cx, __y); \
  *swfdec_as_stack_peek(__cx, __y) = __tmp; \
}G_STMT_END

void		swfdec_as_stack_mark		(SwfdecAsStack *	stack);
void		swfdec_as_stack_ensure_size	(SwfdecAsContext *	context,
						 guint	  		n_elements);
void		swfdec_as_stack_ensure_free	(SwfdecAsContext *	context,
						 guint	  		n_elements);
guint		swfdec_as_stack_get_size	(SwfdecAsContext *	context);
						

G_END_DECLS
#endif
