/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_SLOW_LOADER_H_
#define _SWFDEC_SLOW_LOADER_H_

#include <libswfdec/swfdec.h>

G_BEGIN_DECLS


typedef struct _SwfdecSlowLoader SwfdecSlowLoader;
typedef struct _SwfdecSlowLoaderClass SwfdecSlowLoaderClass;

#define SWFDEC_TYPE_SLOW_LOADER                    (swfdec_slow_loader_get_type())
#define SWFDEC_IS_SLOW_LOADER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SLOW_LOADER))
#define SWFDEC_IS_SLOW_LOADER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SLOW_LOADER))
#define SWFDEC_SLOW_LOADER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SLOW_LOADER, SwfdecSlowLoader))
#define SWFDEC_SLOW_LOADER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SLOW_LOADER, SwfdecSlowLoaderClass))
#define SWFDEC_SLOW_LOADER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_SLOW_LOADER, SwfdecSlowLoaderClass))

struct _SwfdecSlowLoader
{
  SwfdecLoader		parent;

  SwfdecLoader *	loader;		/* what we load */
  guint			tick_time;	/* how many msecs each tick takes */
  guint			duration;	/* total duration in msecs for loading */
  guint			timeout_id;	/* id from g_timeout_add */
};

struct _SwfdecSlowLoaderClass
{
  SwfdecLoaderClass	loader_class;
};

GType		swfdec_slow_loader_get_type   	(void);

SwfdecLoader *	swfdec_slow_loader_new	  	(SwfdecLoader *	loader,
						 guint		duration);
					 

G_END_DECLS
#endif
