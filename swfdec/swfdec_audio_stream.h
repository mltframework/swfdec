/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_AUDIO_STREAM_H_
#define _SWFDEC_AUDIO_STREAM_H_

#include <swfdec/swfdec_audio_decoder.h>
#include <swfdec/swfdec_audio_internal.h>

G_BEGIN_DECLS

typedef struct _SwfdecAudioStream SwfdecAudioStream;
typedef struct _SwfdecAudioStreamClass SwfdecAudioStreamClass;

#define SWFDEC_TYPE_AUDIO_STREAM                    (swfdec_audio_stream_get_type())
#define SWFDEC_IS_AUDIO_STREAM(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AUDIO_STREAM))
#define SWFDEC_IS_AUDIO_STREAM_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AUDIO_STREAM))
#define SWFDEC_AUDIO_STREAM(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AUDIO_STREAM, SwfdecAudioStream))
#define SWFDEC_AUDIO_STREAM_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AUDIO_STREAM, SwfdecAudioStreamClass))
#define SWFDEC_AUDIO_STREAM_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AUDIO_STREAM, SwfdecAudioStreamClass))

struct _SwfdecAudioStream
{
  SwfdecAudio		audio;

  SwfdecAudioDecoder *	decoder;	/* decoder in use */
  GQueue *		queue;		/* all the samples we've decoded so far */
  guint			queue_size;	/* size of queue in samples */
  gboolean		done;		/* no more data will arrive */
};

struct _SwfdecAudioStreamClass
{
  SwfdecAudioClass    	audio_class;

  /* get another buffer if available */
  SwfdecBuffer *	(* pull)			(SwfdecAudioStream *	stream);
};

GType		swfdec_audio_stream_get_type		(void);

/* to be called from pull callback */
void		swfdec_audio_stream_use_decoder		(SwfdecAudioStream *	stream,
							 guint			codec,
							 SwfdecAudioFormat	format);
void		swfdec_audio_stream_done		(SwfdecAudioStream *	stream);

G_END_DECLS
#endif
