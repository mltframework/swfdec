/* Swfdec
 * Copyright (c) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_CACHED_H_
#define _SWFDEC_CACHED_H_

#include <glib-object.h>

G_BEGIN_DECLS

typedef struct _SwfdecCached SwfdecCached;
typedef struct _SwfdecCachedClass SwfdecCachedClass;

#define SWFDEC_TYPE_CACHED                    (swfdec_cached_get_type())
#define SWFDEC_IS_CACHED(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_CACHED))
#define SWFDEC_IS_CACHED_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_CACHED))
#define SWFDEC_CACHED(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_CACHED, SwfdecCached))
#define SWFDEC_CACHED_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_CACHED, SwfdecCachedClass))
#define SWFDEC_CACHED_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_CACHED, SwfdecCachedClass))


struct _SwfdecCached {
  GObject		object;

  gsize			size;
};

struct _SwfdecCachedClass
{
  GObjectClass		object_class;

  /* signals */
  void			(* use)				(SwfdecCached *	cached);
  void			(* unuse)			(SwfdecCached *	cached);
};

GType			swfdec_cached_get_type		(void);

gsize			swfdec_cached_get_size		(SwfdecCached *	cached);

/* for subclasses */
void			swfdec_cached_use		(SwfdecCached *	cached);
void			swfdec_cached_unuse		(SwfdecCached *	cached);


G_END_DECLS
#endif
