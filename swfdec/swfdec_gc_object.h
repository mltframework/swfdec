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

#ifndef _SWFDEC_GC_OBJECT_H_
#define _SWFDEC_GC_OBJECT_H_

#include <glib-object.h>
#include <swfdec/swfdec_as_types.h>

G_BEGIN_DECLS

typedef struct _SwfdecGcObjectClass SwfdecGcObjectClass;

#define SWFDEC_TYPE_GC_OBJECT                    (swfdec_gc_object_get_type())
#define SWFDEC_IS_GC_OBJECT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_GC_OBJECT))
#define SWFDEC_IS_GC_OBJECT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_GC_OBJECT))
#define SWFDEC_GC_OBJECT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_GC_OBJECT, SwfdecGcObject))
#define SWFDEC_GC_OBJECT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_GC_OBJECT, SwfdecGcObjectClass))
#define SWFDEC_GC_OBJECT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_GC_OBJECT, SwfdecGcObjectClass))

struct _SwfdecGcObject {
  /*< protected >*/
  GObject		object;
  SwfdecAsContext *	context;	/* context the object belongs to - NB: object holds no reference */
  /*< private >*/
  SwfdecGcObject *	next;		/* next GcObject in list of context's object */
  guint8		flags;		/* GC flags */
  gsize			size;		/* size reserved in GC */
};

struct _SwfdecGcObjectClass {
  /*< private >*/
  GObjectClass		object_class;

  /*< public >*/
  /* mark everything that should survive during GC */
  void			(* mark)			(SwfdecGcObject *	object);
};

GType			swfdec_gc_object_get_type	(void);

void			swfdec_gc_object_mark		(gpointer		object);
SwfdecAsContext *	swfdec_gc_object_get_context	(gpointer		object);


G_END_DECLS
#endif
