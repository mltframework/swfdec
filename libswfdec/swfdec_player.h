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

#ifndef _SWFDEC_PLAYER_H_
#define _SWFDEC_PLAYER_H_

#include <glib-object.h>
#include <libswfdec/swfdec_loader.h>

G_BEGIN_DECLS


typedef struct _SwfdecPlayer SwfdecPlayer;
typedef struct _SwfdecPlayerClass SwfdecPlayerClass;

#define SWFDEC_TYPE_PLAYER                    (swfdec_player_get_type())
#define SWFDEC_IS_PLAYER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_PLAYER))
#define SWFDEC_IS_PLAYER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_PLAYER))
#define SWFDEC_PLAYER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_PLAYER, SwfdecPlayer))
#define SWFDEC_PLAYER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_PLAYER, SwfdecPlayerClass))
#define SWFDEC_PLAYER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_PLAYER, SwfdecPlayerClass))

void		swfdec_init			(void);

GType		swfdec_player_get_type		(void);

SwfdecPlayer *	swfdec_player_new		(void);
SwfdecPlayer *	swfdec_player_new_from_file	(const char *	filename,
						 GError **	error);
void		swfdec_player_set_loader	(SwfdecPlayer *	player,
						 SwfdecLoader *	loader);

double		swfdec_player_get_rate		(SwfdecPlayer *	player);
void		swfdec_player_get_image_size	(SwfdecPlayer *	player,
						 int *		width,
						 int *		height);
void		swfdec_player_get_invalid	(SwfdecPlayer *	player,
						 SwfdecRect *	rect);
void		swfdec_player_clear_invalid	(SwfdecPlayer *	player);
					 
void		swfdec_player_iterate		(SwfdecPlayer *	player);
void		swfdec_player_render		(SwfdecPlayer *	player,
						 cairo_t *	cr,
						 SwfdecRect *	area);
void		swfdec_player_handle_mouse	(SwfdecPlayer *	player, 
						 double		x,
						 double		y,
						 int		button);
/* audio - see swfdec_audio.c */
guint		swfdec_player_get_audio_samples	(SwfdecPlayer *	player);
guint		swfdec_player_get_latency	(SwfdecPlayer *	player);
void		swfdec_player_set_latency	(SwfdecPlayer *	player,
						 guint		samples);
void		swfdec_player_render_audio	(SwfdecPlayer *	player,
						 gint16 *	dest, 
						 guint		start_offset,
						 guint		n_samples);
SwfdecBuffer *	swfdec_player_render_audio_to_buffer 
						(SwfdecPlayer *	player);

G_END_DECLS
#endif
