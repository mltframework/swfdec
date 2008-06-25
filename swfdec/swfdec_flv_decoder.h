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

#ifndef __SWFDEC_FLV_DECODER_H__
#define __SWFDEC_FLV_DECODER_H__

#include <swfdec/swfdec_audio_internal.h>
#include <swfdec/swfdec_decoder.h>

G_BEGIN_DECLS


typedef struct _SwfdecFlvDecoder SwfdecFlvDecoder;
typedef struct _SwfdecFlvDecoderClass SwfdecFlvDecoderClass;

#define SWFDEC_TYPE_FLV_DECODER                    (swfdec_flv_decoder_get_type())
#define SWFDEC_IS_FLV_DECODER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_FLV_DECODER))
#define SWFDEC_IS_FLV_DECODER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_FLV_DECODER))
#define SWFDEC_FLV_DECODER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_FLV_DECODER, SwfdecFlvDecoder))
#define SWFDEC_FLV_DECODER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_FLV_DECODER, SwfdecFlvDecoderClass))
#define SWFDEC_FLV_DECODER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_FLV_DECODER, SwfdecFlvDecoderClass))

struct _SwfdecFlvDecoder
{
  SwfdecDecoder		decoder;

  int			version;	/* only version 1 is known for now */
  int			state;		/* parsing state we're in */
  GArray *		audio;		/* audio tags */
  GArray *		video;		/* video tags */
  GArray *		data;		/* data tags (if any) */
  SwfdecBufferQueue *	queue;		/* queue for parsing */
};

struct _SwfdecFlvDecoderClass {
  SwfdecDecoderClass	decoder_class;
};

GType		swfdec_flv_decoder_get_type		(void);

gboolean	swfdec_flv_decoder_is_eof		(SwfdecFlvDecoder *	flv);

SwfdecBuffer *	swfdec_flv_decoder_get_video  		(SwfdecFlvDecoder *	flv,
							 guint			timestamp,
							 gboolean		keyframe,
							 guint *		format,
							 guint *		real_timestamp,
							 guint *		next_timestamp);
gboolean	swfdec_flv_decoder_get_video_info     	(SwfdecFlvDecoder *	flv,
							 guint *		first_timestamp,
							 guint *		last_timestamp);
SwfdecBuffer *	swfdec_flv_decoder_get_audio		(SwfdecFlvDecoder *	flv,
							 guint			timestamp,
							 guint *		codec,
							 SwfdecAudioFormat *	format,
							 guint *		real_timestamp,
							 guint *		next_timestamp);
SwfdecBuffer *	swfdec_flv_decoder_get_data		(SwfdecFlvDecoder *	flv,
							 guint			timestamp,
							 guint *		real_timestamp);


G_END_DECLS

#endif
