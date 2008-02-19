/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_LOAD_OBJECT_H_
#define _SWFDEC_LOAD_OBJECT_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_resource.h>

G_BEGIN_DECLS


typedef struct _SwfdecLoadObject SwfdecLoadObject;
typedef struct _SwfdecLoadObjectClass SwfdecLoadObjectClass;

typedef void (* SwfdecLoadObjectProgress) (SwfdecAsObject *target,
    glong loaded, glong size);
typedef void (* SwfdecLoadObjectFinish) (SwfdecAsObject *target,
    const char *text);

#define SWFDEC_TYPE_LOAD_OBJECT                    (swfdec_load_object_get_type())
#define SWFDEC_IS_LOAD_OBJECT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_LOAD_OBJECT))
#define SWFDEC_IS_LOAD_OBJECT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_LOAD_OBJECT))
#define SWFDEC_LOAD_OBJECT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_LOAD_OBJECT, SwfdecLoadObject))
#define SWFDEC_LOAD_OBJECT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_LOAD_OBJECT, SwfdecLoadObjectClass))
#define SWFDEC_LOAD_OBJECT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_LOAD_OBJECT, SwfdecLoadObjectClass))

struct _SwfdecLoadObject {
  GObject			object;
  
  const char *			url;		/* GC'ed url to request */
  SwfdecLoaderRequest		request;	/* type of request */
  SwfdecBuffer *		buffer;		/* data to send */
  SwfdecLoader *		loader;		/* loader when loading or NULL */

  SwfdecSandbox *		sandbox;	/* sandbox that inited the loading */
  guint				version;	/* version used when initiating the load - for parsing the data */
  SwfdecAsObject *		target;		/* target object */
  SwfdecLoadObjectProgress	progress;	/* progress callback */
  SwfdecLoadObjectFinish	finish;		/* finish callback */
};

struct _SwfdecLoadObjectClass {
  GObjectClass			object_class;
};

GType		swfdec_load_object_get_type	(void);

void		swfdec_load_object_create     	(SwfdecAsObject *		target,
						 const char *			url,
						 SwfdecLoaderRequest		request,
						 SwfdecBuffer *			data,
						 SwfdecLoadObjectProgress	progress,
						 SwfdecLoadObjectFinish		finish);


G_END_DECLS
#endif
