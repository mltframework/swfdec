/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_NET_STREAM_H_
#define _SWFDEC_NET_STREAM_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_codec_video.h>
#include <swfdec/swfdec_net_connection.h>
#include <swfdec/swfdec_flv_decoder.h>
#include <swfdec/swfdec_player_internal.h>
#include <swfdec/swfdec_sandbox.h>
#include <swfdec/swfdec_video_movie.h>

G_BEGIN_DECLS

typedef struct _SwfdecNetStream SwfdecNetStream;
typedef struct _SwfdecNetStreamClass SwfdecNetStreamClass;

#define SWFDEC_TYPE_NET_STREAM                    (swfdec_net_stream_get_type())
#define SWFDEC_IS_NET_STREAM(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_NET_STREAM))
#define SWFDEC_IS_NET_STREAM_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_NET_STREAM))
#define SWFDEC_NET_STREAM(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_NET_STREAM, SwfdecNetStream))
#define SWFDEC_NET_STREAM_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_NET_STREAM, SwfdecNetStreamClass))
#define SWFDEC_NET_STREAM_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_NET_STREAM, SwfdecNetStreamClass))

struct _SwfdecNetStream
{
  SwfdecAsObject	object;

  SwfdecNetConnection *	conn;		/* connection used for opening streams */
  char *		requested_url;	/* URL we have requested that isn't loaded yet */
  SwfdecLoader *	loader;		/* input stream */
  SwfdecSandbox *	sandbox;	/* sandbox to emit events in */
  SwfdecFlvDecoder *	flvdecoder;	/* flv decoder */
  gboolean		playing;	/* TRUE if this stream is playing */
  gboolean		buffering;	/* TRUE if we're waiting for more input data */
  gboolean		error;		/* in error */

  /* properties */
  guint			buffer_time;	/* buffering time in msecs */

  /* video decoding */
  guint			current_time;	/* current playback timestamp */
  guint			next_time;	/* next video image at this timestamp */
  guint			format;		/* current format */
  SwfdecVideoDecoder *	decoder;	/* decoder used for decoding */
  guint			decoder_time;	/* last timestamp the decoder decoded */
  cairo_surface_t *	surface;	/* current image */
  SwfdecTimeout		timeout;	/* timeout to advance to */
  GList *		movies;		/* movies we're connected to */

  /* audio */
  SwfdecAudio *		audio;		/* audio stream or NULL when not playing */
};

struct _SwfdecNetStreamClass
{
  SwfdecAsObjectClass	object_class;
};

GType			swfdec_net_stream_get_type	(void);

SwfdecNetStream *	swfdec_net_stream_new		(SwfdecNetConnection *	conn);

void			swfdec_net_stream_set_url	(SwfdecNetStream *	stream,
							 const char *		url);
void			swfdec_net_stream_set_loader	(SwfdecNetStream *	stream,
							 SwfdecLoader *		loader);
void			swfdec_net_stream_set_playing	(SwfdecNetStream *	stream,
							 gboolean		playing);
gboolean		swfdec_net_stream_get_playing	(SwfdecNetStream *	stream);
void			swfdec_net_stream_set_buffer_time (SwfdecNetStream *	stream,
							 double			secs);
double			swfdec_net_stream_get_buffer_time (SwfdecNetStream *	stream);
void			swfdec_net_stream_seek		(SwfdecNetStream *	stream,
							 double			secs);


G_END_DECLS
#endif
