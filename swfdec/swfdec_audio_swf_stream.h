/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_AUDIO_SWF_STREAM_H_
#define _SWFDEC_AUDIO_SWF_STREAM_H_

#include <swfdec/swfdec_audio_stream.h>
#include <swfdec/swfdec_sprite.h>

G_BEGIN_DECLS

typedef struct _SwfdecAudioSwfStream SwfdecAudioSwfStream;
typedef struct _SwfdecAudioSwfStreamClass SwfdecAudioSwfStreamClass;

#define SWFDEC_TYPE_AUDIO_SWF_STREAM                    (swfdec_audio_swf_stream_get_type())
#define SWFDEC_IS_AUDIO_SWF_STREAM(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AUDIO_SWF_STREAM))
#define SWFDEC_IS_AUDIO_SWF_STREAM_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AUDIO_SWF_STREAM))
#define SWFDEC_AUDIO_SWF_STREAM(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AUDIO_SWF_STREAM, SwfdecAudioSwfStream))
#define SWFDEC_AUDIO_SWF_STREAM_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AUDIO_SWF_STREAM, SwfdecAudioSwfStreamClass))
#define SWFDEC_AUDIO_SWF_STREAM_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AUDIO_SWF_STREAM, SwfdecAudioSwfStreamClass))

struct _SwfdecAudioSwfStream
{
  SwfdecAudioStream	stream;

  SwfdecSprite *	sprite;		/* sprite we play audio in */
  guint			id;		/* if of next tag in sprite we need to decode */
};

struct _SwfdecAudioSwfStreamClass
{
  SwfdecAudioStreamClass stream_class;
};

GType		swfdec_audio_swf_stream_get_type	(void);

SwfdecAudio *	swfdec_audio_swf_stream_new		(SwfdecPlayer *		player,
							 SwfdecSprite *		sprite,
							 guint			id);


G_END_DECLS
#endif
