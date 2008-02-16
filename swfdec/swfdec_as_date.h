/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_AS_DATE_H_
#define _SWFDEC_AS_DATE_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_script.h>

G_BEGIN_DECLS

typedef struct _SwfdecAsDate SwfdecAsDate;
typedef struct _SwfdecAsDateClass SwfdecAsDateClass;

#define SWFDEC_TYPE_AS_DATE                    (swfdec_as_date_get_type())
#define SWFDEC_IS_AS_DATE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AS_DATE))
#define SWFDEC_IS_AS_DATE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AS_DATE))
#define SWFDEC_AS_DATE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AS_DATE, SwfdecAsDate))
#define SWFDEC_AS_DATE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AS_DATE, SwfdecAsDateClass))
#define SWFDEC_AS_DATE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AS_DATE, SwfdecAsDateClass))

struct _SwfdecAsDate {
  SwfdecAsObject	object;

  // time from epoch in UTC
  double		milliseconds;

  // the difference between local timezone and UTC in minutes
  int			utc_offset;
};

struct _SwfdecAsDateClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_as_date_get_type	(void);

SwfdecAsObject *swfdec_as_date_new		(SwfdecAsContext *	context,
						 double			milliseconds,
						 int			utc_offset);


G_END_DECLS
#endif
