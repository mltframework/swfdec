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

#ifndef _SWFDEC_LOADER_H_
#define _SWFDEC_LOADER_H_

#include <glib-object.h>
#include <swfdec/swfdec_buffer.h>
#include <swfdec/swfdec_player.h>
#include <swfdec/swfdec_stream.h>
#include <swfdec/swfdec_url.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_LOADER_DATA_UNKNOWN,
  SWFDEC_LOADER_DATA_SWF,
  SWFDEC_LOADER_DATA_FLV,
  SWFDEC_LOADER_DATA_XML,
  SWFDEC_LOADER_DATA_TEXT,
  SWFDEC_LOADER_DATA_JPEG,
  SWFDEC_LOADER_DATA_PNG
} SwfdecLoaderDataType;

typedef struct _SwfdecLoader SwfdecLoader;
typedef struct _SwfdecLoaderClass SwfdecLoaderClass;

#define SWFDEC_TYPE_LOADER                    (swfdec_loader_get_type())
#define SWFDEC_IS_LOADER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_LOADER))
#define SWFDEC_IS_LOADER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_LOADER))
#define SWFDEC_LOADER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_LOADER, SwfdecLoader))
#define SWFDEC_LOADER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_LOADER, SwfdecLoaderClass))
#define SWFDEC_LOADER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_LOADER, SwfdecLoaderClass))

struct _SwfdecLoader
{
  SwfdecStream		stream;

  /*< protected >*/
  SwfdecURL *		url;		/* the URL for this loader in UTF-8 - must be set on creation */
  glong			size;		/* number of bytes in stream or -1 if unknown */
  SwfdecLoaderDataType	data_type;	/* type this stream is in (identified by swfdec) */
};

struct _SwfdecLoaderClass
{
  /*< private >*/
  SwfdecStreamClass	stream_class;

  /*< public >*/
  void			(* load)	(SwfdecLoader *			loader, 
					 SwfdecPlayer *			player,
					 const char *			url,
					 SwfdecBuffer *			buffer,
					 guint				n_headers,
					 const char **			header_names,
					 const char **			header_values);
};

GType		swfdec_loader_get_type		(void);

void		swfdec_loader_set_url		(SwfdecLoader *		loader,
						 const char *		url);
const SwfdecURL *
		swfdec_loader_get_url		(SwfdecLoader *		loader);
void		swfdec_loader_set_size		(SwfdecLoader *		loader,
						 gulong			size);
glong		swfdec_loader_get_size		(SwfdecLoader *		loader);
gulong		swfdec_loader_get_loaded	(SwfdecLoader *		loader);
SwfdecLoaderDataType
		swfdec_loader_get_data_type	(SwfdecLoader *		loader);

const char *	swfdec_loader_data_type_get_extension
						(SwfdecLoaderDataType	type);
					 

G_END_DECLS
#endif
