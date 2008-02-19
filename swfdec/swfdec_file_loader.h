/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_FILE_LOADER_H_
#define _SWFDEC_FILE_LOADER_H_

#include <swfdec/swfdec_loader.h>

G_BEGIN_DECLS

typedef struct _SwfdecFileLoader SwfdecFileLoader;
typedef struct _SwfdecFileLoaderClass SwfdecFileLoaderClass;

#define SWFDEC_TYPE_FILE_LOADER                    (swfdec_file_loader_get_type())
#define SWFDEC_IS_FILE_LOADER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_FILE_LOADER))
#define SWFDEC_IS_FILE_LOADER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_FILE_LOADER))
#define SWFDEC_FILE_LOADER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_FILE_LOADER, SwfdecFileLoader))
#define SWFDEC_FILE_LOADER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_FILE_LOADER, SwfdecFileLoaderClass))
#define SWFDEC_FILE_LOADER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_FILE_LOADER, SwfdecFileLoaderClass))

struct _SwfdecFileLoader
{
  SwfdecLoader		loader;
};

struct _SwfdecFileLoaderClass
{
  SwfdecLoaderClass   	loader_class;
};

GType		swfdec_file_loader_get_type	(void);


G_END_DECLS

#endif /* _SWFDEC_FILE_LOADER_H_ */
