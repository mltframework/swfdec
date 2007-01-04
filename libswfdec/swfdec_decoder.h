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

#ifndef _SWFDEC_DECODER_H_
#define _SWFDEC_DECODER_H_

#include <glib-object.h>
#include <libswfdec/swfdec_buffer.h>
#include <libswfdec/swfdec_player.h>
#include <libswfdec/swfdec_types.h>

G_BEGIN_DECLS


typedef enum {
  /* An error occured during parsing */
  SWFDEC_STATUS_ERROR = 0,
  /* processing continued as expected */
  SWFDEC_STATUS_OK,
  /* more data needs to be made available for processing */
  SWFDEC_STATUS_NEEDBITS,
  /* at least one new image is available for display */
  SWFDEC_STATUS_IMAGE,
  /* header parsing is complete, framerate, image size etc are known */
  SWFDEC_STATUS_INIT,
  /* parsing is finished */
  SWFDEC_STATUS_EOF
} SwfdecStatus;

//typedef struct _SwfdecDecoder SwfdecDecoder;
typedef struct _SwfdecDecoderClass SwfdecDecoderClass;

#define SWFDEC_TYPE_DECODER                    (swfdec_decoder_get_type())
#define SWFDEC_IS_DECODER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DECODER))
#define SWFDEC_IS_DECODER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DECODER))
#define SWFDEC_DECODER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DECODER, SwfdecDecoder))
#define SWFDEC_DECODER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DECODER, SwfdecDecoderClass))
#define SWFDEC_DECODER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_DECODER, SwfdecDecoderClass))

struct _SwfdecDecoder
{
  GObject		object;

  SwfdecPlayer *	player;		/* FIXME: only needed to get the JS Context, I want it gone */
  guint			rate;		/* rate of stream */
  guint			width;		/* width of stream */
  guint			height;		/* guess */
  guint			bytes_loaded; 	/* bytes already loaded */
  guint			bytes_total;	/* total bytes in the file or 0 if not known */
  guint			frames_loaded;	/* frames already loaded */
  guint			frames_total;	/* total frames */
  SwfdecBufferQueue *	queue;		/* the queue containing the data to be parsed */
};

struct _SwfdecDecoderClass
{
  GObjectClass		object_class;

  SwfdecStatus		(* parse)		(SwfdecDecoder *	decoder);
};

GType		swfdec_decoder_get_type		(void);

#define swfdec_decoder_can_detect(queue) (swfdec_buffer_queue_get_depth (queue) >= 3)
SwfdecDecoder *	swfdec_decoder_new		(SwfdecPlayer *		player,
						 SwfdecBufferQueue *	queue);

G_END_DECLS
#endif
