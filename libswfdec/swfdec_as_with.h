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

#ifndef _SWFDEC_AS_WITH_H_
#define _SWFDEC_AS_WITH_H_

#include <libswfdec/swfdec_as_scope.h>
#include <libswfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsWith SwfdecAsWith;
typedef struct _SwfdecAsWithClass SwfdecAsWithClass;

#define SWFDEC_TYPE_AS_WITH                    (swfdec_as_with_get_type())
#define SWFDEC_IS_AS_WITH(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_WITH))
#define SWFDEC_IS_AS_WITH_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_WITH))
#define SWFDEC_AS_WITH(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_WITH, SwfdecAsWith))
#define SWFDEC_AS_WITH_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_WITH, SwfdecAsWithClass))
#define SWFDEC_AS_WITH_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_WITH, SwfdecAsWithClass))

struct _SwfdecAsWith {
  SwfdecAsScope		scope;

  /*< private >*/
  SwfdecAsObject *	object;		/* the object of this With context */
};

struct _SwfdecAsWithClass {
  SwfdecAsScopeClass	scope_class;
};

GType		swfdec_as_with_get_type	(void);

SwfdecAsScope *	swfdec_as_with_new	(SwfdecAsObject *	object,
					 const guint8 *		startpc,
					 guint			n_bytes);

G_END_DECLS
#endif
