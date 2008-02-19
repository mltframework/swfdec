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

#ifndef _SWFDEC_AUDIO_FLV_H_
#define _SWFDEC_AUDIO_FLV_H_

#include <swfdec/swfdec_audio_internal.h>
#include <swfdec/swfdec_flv_decoder.h>

G_BEGIN_DECLS

typedef struct _SwfdecAudioFlv SwfdecAudioFlv;
typedef struct _SwfdecAudioFlvClass SwfdecAudioFlvClass;

#define SWFDEC_TYPE_AUDIO_FLV                    (swfdec_audio_flv_get_type())
#define SWFDEC_IS_AUDIO_FLV(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_AUDIO_FLV))
#define SWFDEC_IS_AUDIO_FLV_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_AUDIO_FLV))
#define SWFDEC_AUDIO_FLV(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_AUDIO_FLV, SwfdecAudioFlv))
#define SWFDEC_AUDIO_FLV_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_AUDIO_FLV, SwfdecAudioFlvClass))
#define SWFDEC_AUDIO_FLV_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_AUDIO_FLV, SwfdecAudioFlvClass))

struct _SwfdecAudioFlv
{
  SwfdecAudio		audio;

  SwfdecFlvDecoder *	flvdecoder;	/* decoder we play back */
  guint			format;		/* codec format of audio */
  gboolean		width;		/* width of audio */
  SwfdecAudioFormat	in;		/* input format of data */
  SwfdecAudioDecoder *	decoder;	/* decoder used for playback */

  SwfdecTick		timestamp;	/* current playback timestamp */
  guint			next_timestamp;	/* next timestamp in FLV file we request from */
  guint			playback_skip;	/* number of samples to skip at start of queue */
  GQueue *		playback_queue;	/* all the samples we've decoded so far */
};

struct _SwfdecAudioFlvClass
{
  SwfdecAudioClass    	audio_class;
};

GType		swfdec_audio_flv_get_type		(void);

SwfdecAudio *	swfdec_audio_flv_new			(SwfdecPlayer *	  	player,
							 SwfdecFlvDecoder *	decoder,
							 guint			timestamp);

G_END_DECLS
#endif
