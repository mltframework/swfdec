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

#ifndef _SWFDEC_AS_SUPER_H_
#define _SWFDEC_AS_SUPER_H_

#include <swfdec/swfdec_as_function.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsSuper SwfdecAsSuper;
typedef struct _SwfdecAsSuperClass SwfdecAsSuperClass;

#define SWFDEC_TYPE_AS_SUPER                    (swfdec_as_super_get_type())
#define SWFDEC_IS_AS_SUPER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_SUPER))
#define SWFDEC_IS_AS_SUPER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_SUPER))
#define SWFDEC_AS_SUPER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_SUPER, SwfdecAsSuper))
#define SWFDEC_AS_SUPER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_SUPER, SwfdecAsSuperClass))
#define SWFDEC_AS_SUPER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_SUPER, SwfdecAsSuperClass))

struct _SwfdecAsSuper {
  SwfdecAsFunction	function;

  SwfdecAsObject *	thisp;		/* object super was called on */
  SwfdecAsObject *	object;		/* current prototype we reference or NULL if none */
};

struct _SwfdecAsSuperClass {
  SwfdecAsFunctionClass	function_class;
};

GType		swfdec_as_super_get_type	(void);

void		swfdec_as_super_new		(SwfdecAsFrame *	frame,
						 SwfdecAsObject *	thisp,
						 SwfdecAsObject *	ref);
SwfdecAsObject *swfdec_as_super_resolve_property(SwfdecAsSuper *	super,
						 const char *		name);


G_END_DECLS
#endif
