/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_INTERNAL_H_
#define _SWFDEC_INTERNAL_H_

#include <libswfdec/swfdec_types.h>
#include <libswfdec/swfdec_codec_audio.h>
#include <libswfdec/swfdec_codec_video.h>

G_BEGIN_DECLS


/* audio codecs */

SwfdecAudioDecoder *	swfdec_audio_decoder_adpcm_new		(SwfdecAudioFormat	type, 
								 gboolean		width,
								 SwfdecAudioOut		format);
#ifdef HAVE_MAD
SwfdecAudioDecoder *	swfdec_audio_decoder_mad_new		(SwfdecAudioFormat	type, 
								 gboolean		width,
								 SwfdecAudioOut		format);
#endif
#ifdef HAVE_FFMPEG
SwfdecAudioDecoder *	swfdec_audio_decoder_ffmpeg_new		(SwfdecAudioFormat	type, 
								 gboolean		width,
								 SwfdecAudioOut		format);
#endif
#ifdef HAVE_GST
SwfdecAudioDecoder *	swfdec_audio_decoder_gst_new		(SwfdecAudioFormat	type, 
								 gboolean		width,
								 SwfdecAudioOut		format);
#endif

/* video codecs */

SwfdecVideoDecoder *	swfdec_video_decoder_screen_new		(SwfdecVideoFormat	format);
#ifdef HAVE_FFMPEG
SwfdecVideoDecoder *	swfdec_video_decoder_ffmpeg_new		(SwfdecVideoFormat	format);
#endif
#ifdef HAVE_GST
SwfdecVideoDecoder *	swfdec_video_decoder_gst_new		(SwfdecVideoFormat	format);
#endif

/* AS engine setup code */

void			swfdec_player_init_global		(SwfdecPlayer *		player,
								 guint			version);
void			swfdec_movie_color_init_context		(SwfdecPlayer *		player,
								 guint			version);
void			swfdec_net_connection_init_context	(SwfdecPlayer *		player,
								 guint			version);
void			swfdec_net_stream_init_context		(SwfdecPlayer *		player,
								 guint			version);
void			swfdec_sprite_movie_init_context	(SwfdecPlayer *		player,
								 guint			version);
void			swfdec_video_movie_init_context		(SwfdecPlayer *		player,
								 guint			version);
void			swfdec_xml_node_init_context		(SwfdecPlayer *		player,
								 guint			version);
void			swfdec_xml_init_context			(SwfdecPlayer *		player,
								 guint			version);
void			swfdec_xml_init_context2		(SwfdecPlayer *		player,
								 guint			version);

G_END_DECLS
#endif
