/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_STREAM_H_
#define _SWFDEC_STREAM_H_

#include <glib-object.h>
#include <libswfdec/swfdec_buffer.h>
#include <libswfdec/swfdec_player.h>

G_BEGIN_DECLS

typedef struct _SwfdecStream SwfdecStream;
typedef struct _SwfdecStreamClass SwfdecStreamClass;

#define SWFDEC_TYPE_STREAM                    (swfdec_stream_get_type())
#define SWFDEC_IS_STREAM(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_STREAM))
#define SWFDEC_IS_STREAM_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_STREAM))
#define SWFDEC_STREAM(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_STREAM, SwfdecStream))
#define SWFDEC_STREAM_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_STREAM, SwfdecStreamClass))
#define SWFDEC_STREAM_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_STREAM, SwfdecStreamClass))

struct _SwfdecStream
{
  GObject		object;

  /*< private >*/
  SwfdecPlayer *	player;		/* player to queue target notificaions in */
  gpointer		target;		/* SwfdecStreamTarget that gets notified about loading progress */
  guint			state;		/* SwfdecStreamState the stream is currently in */
  guint			processed_state;/* SwfdecStreamState the target knows about */
  gboolean		queued;		/* TRUE if we have queued an action already */
  char *		error;		/* error message if in error state or NULL */
  SwfdecBufferQueue *	queue;		/* SwfdecBufferQueue managing the input buffers */
};

struct _SwfdecStreamClass
{
  GObjectClass		object_class;

  /* get a nice description string */
  const char *		(* describe)		(SwfdecStream *		stream);
  /* close the stream. */
  void			(* close)		(SwfdecStream *		stream);
};

GType		swfdec_stream_get_type		(void);

void		swfdec_stream_open		(SwfdecStream *		stream,
						 const char *		url);
void		swfdec_stream_push		(SwfdecStream *		stream,
						 SwfdecBuffer *		buffer);
void		swfdec_stream_eof		(SwfdecStream *		stream);
void		swfdec_stream_error		(SwfdecStream *		stream,
						 const char *		error);
					 

G_END_DECLS
#endif
