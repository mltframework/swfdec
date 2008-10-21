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

#ifndef _SWFDEC_AS_BOOLEAN_H_
#define _SWFDEC_AS_BOOLEAN_H_

#include <swfdec/swfdec_as_relay.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsBoolean SwfdecAsBoolean;
typedef struct _SwfdecAsBooleanClass SwfdecAsBooleanClass;

#define SWFDEC_TYPE_AS_BOOLEAN                    (swfdec_as_boolean_get_type())
#define SWFDEC_IS_AS_BOOLEAN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_BOOLEAN))
#define SWFDEC_IS_AS_BOOLEAN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_BOOLEAN))
#define SWFDEC_AS_BOOLEAN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_BOOLEAN, SwfdecAsBoolean))
#define SWFDEC_AS_BOOLEAN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_BOOLEAN, SwfdecAsBooleanClass))
#define SWFDEC_AS_BOOLEAN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_BOOLEAN, SwfdecAsBooleanClass))

struct _SwfdecAsBoolean {
  SwfdecAsRelay		relay;

  gboolean		boolean;		/* boolean represented by this boolean object */
};

struct _SwfdecAsBooleanClass {
  SwfdecAsRelayClass	relay_class;
};

GType		swfdec_as_boolean_get_type	(void);


G_END_DECLS
#endif
