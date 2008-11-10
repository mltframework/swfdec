/* Swfdec
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

#ifndef _SWFDEC_AS_GCABLE_H_
#define _SWFDEC_AS_GCABLE_H_

#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

#define SWFDEC_AS_GC_FLAG_MASK 7

#define SWFDEC_AS_GC_MARK (1 << 0)
#define SWFDEC_AS_GC_ROOT (1 << 1)
#define SWFDEC_AS_GC_ALIGN (1 << 2)

typedef struct _SwfdecAsGcable SwfdecAsGcable;
struct _SwfdecAsGcable {
  SwfdecAsGcable *	next;
};

typedef void (* SwfdecAsGcableDestroyNotify) (SwfdecAsContext *context, gpointer mem);

#define SWFDEC_AS_GCABLE_FLAG_IS_SET(gc,flag) (GPOINTER_TO_SIZE((gc)->next) & (flag))
#define SWFDEC_AS_GCABLE_SET_FLAG(gc,flag) (gc)->next = GSIZE_TO_POINTER (GPOINTER_TO_SIZE((gc)->next) | (flag))
#define SWFDEC_AS_GCABLE_UNSET_FLAG(gc,flag) (gc)->next = GSIZE_TO_POINTER (GPOINTER_TO_SIZE((gc)->next) & ~(flag))

#define SWFDEC_AS_GCABLE_NEXT(gc) ((SwfdecAsGcable *) GSIZE_TO_POINTER (GPOINTER_TO_SIZE((gc)->next) & ~7))
#define SWFDEC_AS_GCABLE_SET_NEXT(gc, val) (gc)->next = GSIZE_TO_POINTER (GPOINTER_TO_SIZE (val) | (GPOINTER_TO_SIZE((gc)->next) & 7))

gpointer	swfdec_as_gcable_alloc		(SwfdecAsContext *	context,
						 gsize			size);
#define swfdec_as_gcable_new(cx,type) ((type *) swfdec_as_gcable_alloc (cx, sizeof (type)))
void		swfdec_as_gcable_free		(SwfdecAsContext *	context,
						 gpointer		mem,
						 gsize			size);

SwfdecAsGcable *swfdec_as_gcable_collect	(SwfdecAsContext *	context,
						 SwfdecAsGcable *	gc,
						 SwfdecAsGcableDestroyNotify notify);


G_END_DECLS
#endif
