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

#ifndef _SWFDEC_INTERNAL_H_
#define _SWFDEC_INTERNAL_H_

#include <swfdec/swfdec_buffer.h>
#include <swfdec/swfdec_player.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS


/* AS engine setup code */

void			swfdec_player_preinit_global		(SwfdecAsContext *	context);
void			swfdec_net_stream_init_context		(SwfdecPlayer *		player);
void			swfdec_sprite_movie_init_context	(SwfdecPlayer *		player);
void			swfdec_video_movie_init_context		(SwfdecPlayer *		player);

/* functions that shouldn't go into public headers */

char *			swfdec_buffer_queue_pull_text		(SwfdecBufferQueue *	queue,
								 guint			version);

gboolean		swfdec_as_value_to_twips		(SwfdecAsContext *	context,
								 const SwfdecAsValue *	val,
								 gboolean		is_length,
								 SwfdecTwips *		result);


G_END_DECLS
#endif
