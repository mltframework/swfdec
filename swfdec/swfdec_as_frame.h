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

#ifndef _SWFDEC_AS_FRAME_H_
#define _SWFDEC_AS_FRAME_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsStackIterator SwfdecAsStackIterator;

struct _SwfdecAsStackIterator {
  /*< private >*/
  SwfdecAsStack *	stack;
  SwfdecAsValue *	current;
  guint			i;
  guint			n;
};


SwfdecAsFrame *	swfdec_as_frame_get_next	(SwfdecAsFrame *		frame);
SwfdecScript *	swfdec_as_frame_get_script	(SwfdecAsFrame *		frame);

SwfdecAsValue *	swfdec_as_stack_iterator_init	(SwfdecAsStackIterator *	iter,
						 SwfdecAsFrame *		frame);
SwfdecAsValue *	swfdec_as_stack_iterator_init_arguments 
						(SwfdecAsStackIterator *	iter,
						 SwfdecAsFrame *		frame);
SwfdecAsValue *	swfdec_as_stack_iterator_next	(SwfdecAsStackIterator *	iter);


G_END_DECLS
#endif
