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

#ifndef _SWFDEC_PLAYER_INTERNAL_H_
#define _SWFDEC_PLAYER_INTERNAL_H_

#include <glib-object.h>
#include <libswfdec/swfdec_player.h>
#include <libswfdec/swfdec_rect.h>
#include <libswfdec/swfdec_ringbuffer.h>
#include <js/jsapi.h>

G_BEGIN_DECLS


struct _SwfdecPlayer
{
  GObject		object;

  /* global properties */
  unsigned int		rate;			/* divide by 256 to get iterations per second */
  unsigned int		width;			/* width of movie */
  unsigned int		height;			/* height of movie */
  GList *		roots;			/* all the root movies */

  /* javascript */
  JSContext *		jscx;			/* global Javascript context */
  JSObject *		jsobj;			/* the global object */

  /* rendering */
  SwfdecRect		invalid;      		/* area that needs a rredraw */

  /* mouse */
  gboolean		mouse_visible;	  	/* show the mouse (actionscriptable) */

  /* audio */
  GArray *		audio;		 	/* SwfdecAudioEvent array of running streams */
  guint			samples_this_frame;   	/* amount of samples to be played this frame */
  guint			samples_overhead;     	/* 44100*256th of sample missing each frame due to weird rate */
  guint			samples_overhead_left;	/* 44100*256th of sample we spit out too much so far */
  guint			samples_latency;	/* latency in samples */

  /* iterating */
  GList *		movies;			/* list of all moveis that want to be iterated */
  SwfdecRingBuffer *	gotos;			/* all gotos we've queued up so far */
};

struct _SwfdecPlayerClass
{
  GObjectClass		object_class;
};

void		swfdec_player_add_movie		(SwfdecPlayer *		player,
						 guint			depth,
						 const char *		url);
void		swfdec_player_remove_movie	(SwfdecPlayer *		player,
						 SwfdecMovie *		movie);

void		swfdec_player_invalidate	(SwfdecPlayer *		player,
						 const SwfdecRect *	rect);
gboolean	swfdec_player_do_goto		(SwfdecPlayer *		player);
					 

G_END_DECLS
#endif
