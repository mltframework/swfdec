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

#ifndef _SWFDEC_LOADER_INTERNAL_H_
#define _SWFDEC_LOADER_INTERNAL_H_

#include "swfdec_loader.h"
#include "swfdec_loadertarget.h"

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

  char *		dir;		/* base directory for load operations */
};

struct _SwfdecFileLoaderClass
{
  SwfdecLoaderClass   	loader_class;
};

GType			swfdec_file_loader_get_type	(void);

SwfdecLoader *		swfdec_loader_load		(SwfdecLoader *		loader,
							 const char *		url,
							 SwfdecLoaderRequest	request,
							 const char *		data,
							 gsize			data_len);
void			swfdec_loader_parse		(SwfdecLoader *		loader);
void			swfdec_loader_queue_parse	(SwfdecLoader *		loader);
void			swfdec_loader_set_target	(SwfdecLoader *		loader,
							 SwfdecLoaderTarget *	target);
void			swfdec_loader_set_data_type	(SwfdecLoader *		loader,
							 SwfdecLoaderDataType	type);
void	  		swfdec_loader_error_locked	(SwfdecLoader *		loader,
							 const char *		error);

gboolean		swfdec_urldecode_one		(const char *		string,
							 char **		name,
							 char **		value,
							 const char **		end);
void			swfdec_string_append_urlencoded	(GString *		str,
							 const char *		name,
							 const char *		value);

G_END_DECLS
#endif
