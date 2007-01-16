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

#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_flv_decoder.h>
#include <libswfdec/swfdec_player_internal.h>
#include <libswfdec/swfdec_scriptable.h>
#include <libswfdec/swfdec_video_movie.h>

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
  SwfdecScriptable	scriptable;

  SwfdecPlayer *	player;		/* the player we play in */
  SwfdecLoader *	loader;		/* input connection */
  SwfdecFlvDecoder *	flvdecoder;	/* flv decoder */
  gboolean		playing;	/* TRUE if this stream is playing */
  gboolean		error;		/* in error */

  /* video decoding */
  guint			current_time;	/* current playback timestamp */
  guint			next_time;	/* next video image at this timestamp */
  SwfdecVideoFormat	format;		/* current format */
  const SwfdecVideoCodec *codec;	/* codec used to decode the video */
  gpointer		decoder;	/* decoder used for decoding */
  cairo_surface_t *	surface;	/* current image */
  SwfdecTimeout		timeout;	/* timeout to advance to */
  SwfdecVideoMovieInput	input;		/* used when attaching to a video movie */

  /* audio */
  SwfdecAudio *		audio;		/* audio stream or NULL when not playing */
};

struct _SwfdecNetStreamClass
{
  SwfdecScriptableClass	scriptable_class;
};

GType			swfdec_net_stream_get_type	(void);

SwfdecNetStream *	swfdec_net_stream_new		(SwfdecPlayer *		player);

void			swfdec_net_stream_set_loader	(SwfdecNetStream *	stream,
							 SwfdecLoader *		loader);
void			swfdec_net_stream_set_playing	(SwfdecNetStream *	stream,
							 gboolean		playing);
gboolean		swfdec_net_stream_get_playing	(SwfdecNetStream *	stream);


G_END_DECLS
#endif
