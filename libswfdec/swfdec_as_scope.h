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

#ifndef _SWFDEC_AS_SCOPE_H_
#define _SWFDEC_AS_SCOPE_H_

#include <libswfdec/swfdec_as_object.h>
#include <libswfdec/swfdec_as_types.h>
#include <libswfdec/swfdec_script.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsScopeClass SwfdecAsScopeClass;

#define SWFDEC_TYPE_AS_SCOPE                    (swfdec_as_scope_get_type())
#define SWFDEC_IS_AS_SCOPE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_SCOPE))
#define SWFDEC_IS_AS_SCOPE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_SCOPE))
#define SWFDEC_AS_SCOPE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_SCOPE, SwfdecAsScope))
#define SWFDEC_AS_SCOPE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_SCOPE, SwfdecAsScopeClass))
#define SWFDEC_AS_SCOPE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_SCOPE, SwfdecAsScopeClass))

struct _SwfdecAsScope {
  SwfdecAsObject	object;

  /*< private >*/
  SwfdecAsScope *	next;		/* next scope or NULL if last */
};

struct _SwfdecAsScopeClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_as_scope_get_type	(void);


G_END_DECLS
#endif
