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
#include <libswfdec/swfdec_buffer.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_LOADER_DATA_UNKNOWN,
  SWFDEC_LOADER_DATA_SWF,
  SWFDEC_LOADER_DATA_FLV,
  SWFDEC_LOADER_DATA_XML,
  SWFDEC_LOADER_DATA_TEXT
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
  GObject		object;

  char *		url;		/* the URL for this loader in UTF-8 - must be set on creation */
  /*< private >*/
  gulong		size;		/* number of bytes in stream or 0 if unknown */
  gboolean		eof;		/* if we're in EOF already */
  char *		error;		/* if there's an error (from parsing the loader) */
  gpointer		target;		/* SwfdecLoaderTarget that gets notified about loading progress */
  SwfdecBufferQueue *	queue;		/* SwfdecBufferQueue managing the input buffers */
  SwfdecLoaderDataType	data_type;	/* type this stream is in (identified by swfdec) */
};

struct _SwfdecLoaderClass
{
  GObjectClass		object_class;

  /* loads the given URL. Must return a loader, the loader can be in the error state */
  SwfdecLoader *      	(* load)	(SwfdecLoader *			loader, 
					 const char *			url);
};

GType		swfdec_loader_get_type		(void);

SwfdecLoader *	swfdec_loader_new_from_file	(const char *	 	filename);

void		swfdec_loader_push		(SwfdecLoader *		loader,
						 SwfdecBuffer *		buffer);
void		swfdec_loader_eof		(SwfdecLoader *		loader);
void		swfdec_loader_error		(SwfdecLoader *		loader,
						 const char *		error);
void		swfdec_loader_set_size		(SwfdecLoader *		loader,
						 gulong			size);
gulong		swfdec_loader_get_size		(SwfdecLoader *		loader);
gulong		swfdec_loader_get_loaded	(SwfdecLoader *		loader);
char *  	swfdec_loader_get_filename	(SwfdecLoader *		loader);
SwfdecLoaderDataType
		swfdec_loader_get_data_type	(SwfdecLoader *		loader);

const char *	swfdec_loader_data_type_get_extension
						(SwfdecLoaderDataType	type);
					 

G_END_DECLS
#endif
