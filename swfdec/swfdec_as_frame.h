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

typedef struct _SwfdecAsFrameClass SwfdecAsFrameClass;
typedef struct _SwfdecAsStackIterator SwfdecAsStackIterator;

struct _SwfdecAsStackIterator {
  /*< private >*/
  SwfdecAsStack *	stack;
  SwfdecAsValue *	current;
  guint			i;
  guint			n;
};


#define SWFDEC_TYPE_AS_FRAME                    (swfdec_as_frame_get_type())
#define SWFDEC_IS_AS_FRAME(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_FRAME))
#define SWFDEC_IS_AS_FRAME_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_FRAME))
#define SWFDEC_AS_FRAME(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_FRAME, SwfdecAsFrame))
#define SWFDEC_AS_FRAME_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_FRAME, SwfdecAsFrameClass))
#define SWFDEC_AS_FRAME_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_FRAME, SwfdecAsFrameClass))

GType		swfdec_as_frame_get_type	(void);

SwfdecAsFrame *	swfdec_as_frame_get_next	(SwfdecAsFrame *		frame);
SwfdecScript *	swfdec_as_frame_get_script	(SwfdecAsFrame *		frame);
SwfdecAsObject *swfdec_as_frame_get_this	(SwfdecAsFrame *		frame);

SwfdecAsValue *	swfdec_as_stack_iterator_init	(SwfdecAsStackIterator *	iter,
						 SwfdecAsFrame *		frame);
SwfdecAsValue *	swfdec_as_stack_iterator_init_arguments 
						(SwfdecAsStackIterator *	iter,
						 SwfdecAsFrame *		frame);
SwfdecAsValue *	swfdec_as_stack_iterator_next	(SwfdecAsStackIterator *	iter);


G_END_DECLS
#endif
