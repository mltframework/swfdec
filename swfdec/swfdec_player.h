/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include <cairo.h>
#include <swfdec/swfdec_as_context.h>
#include <swfdec/swfdec_as_types.h>
#include <swfdec/swfdec_url.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_MOUSE_CURSOR_NORMAL,
  SWFDEC_MOUSE_CURSOR_NONE,
  SWFDEC_MOUSE_CURSOR_TEXT,
  SWFDEC_MOUSE_CURSOR_CLICK
} SwfdecMouseCursor;

typedef enum {
  SWFDEC_ALIGNMENT_TOP_LEFT,
  SWFDEC_ALIGNMENT_TOP,
  SWFDEC_ALIGNMENT_TOP_RIGHT,
  SWFDEC_ALIGNMENT_LEFT,
  SWFDEC_ALIGNMENT_CENTER,
  SWFDEC_ALIGNMENT_RIGHT,
  SWFDEC_ALIGNMENT_BOTTOM_LEFT,
  SWFDEC_ALIGNMENT_BOTTOM,
  SWFDEC_ALIGNMENT_BOTTOM_RIGHT
} SwfdecAlignment;

typedef enum {
  SWFDEC_SCALE_SHOW_ALL,
  SWFDEC_SCALE_NO_BORDER,
  SWFDEC_SCALE_EXACT_FIT,
  SWFDEC_SCALE_NONE
} SwfdecScaleMode;

#define SWFDEC_TYPE_TIME_VAL swfdec_time_val_get_type()
GType swfdec_time_val_get_type  (void);

/* forward declarations */
typedef struct _SwfdecPlayerScripting SwfdecPlayerScripting;
typedef struct _SwfdecRenderer SwfdecRenderer;

typedef struct _SwfdecPlayer SwfdecPlayer;
typedef struct _SwfdecPlayerPrivate SwfdecPlayerPrivate;
typedef struct _SwfdecPlayerClass SwfdecPlayerClass;

#define SWFDEC_TYPE_PLAYER                    (swfdec_player_get_type())
#define SWFDEC_IS_PLAYER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_PLAYER))
#define SWFDEC_IS_PLAYER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_PLAYER))
#define SWFDEC_PLAYER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_PLAYER, SwfdecPlayer))
#define SWFDEC_PLAYER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_PLAYER, SwfdecPlayerClass))
#define SWFDEC_PLAYER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_PLAYER, SwfdecPlayerClass))

struct _SwfdecPlayer
{
  SwfdecAsContext	context;
  SwfdecPlayerPrivate *	priv;
};

struct _SwfdecPlayerClass
{
  SwfdecAsContextClass	context_class;

  void			(* advance)		(SwfdecPlayer *		player,
						 gulong			msecs,
						 guint			audio_samples);
  gboolean		(* handle_key)		(SwfdecPlayer *		player,
						 guint			key,
						 guint			character,
						 gboolean		down);
  gboolean		(* handle_mouse)	(SwfdecPlayer *		player,
						 double			x,
						 double			y,
						 int			button);
  void			(* missing_plugins)	(SwfdecPlayer *		player,
						 const char **		details);
};

void		swfdec_init			(void);

GType		swfdec_player_get_type		(void);

SwfdecPlayer *	swfdec_player_new		(SwfdecAsDebugger *	debugger);

gboolean	swfdec_player_is_initialized	(SwfdecPlayer *	player);
glong		swfdec_player_get_next_event  	(SwfdecPlayer *	player);
double		swfdec_player_get_rate		(SwfdecPlayer *	player);
void		swfdec_player_get_default_size	(SwfdecPlayer *	player,
						 guint *	width,
						 guint *	height);
void		swfdec_player_get_size		(SwfdecPlayer *	player,
						 int *		width,
						 int *		height);
void		swfdec_player_set_size		(SwfdecPlayer *	player,
						 int		width,
						 int		height);
guint		swfdec_player_get_background_color 
						(SwfdecPlayer *	player);
void		swfdec_player_set_background_color 
						(SwfdecPlayer *	player,
						 guint	color);
SwfdecScaleMode	swfdec_player_get_scale_mode	(SwfdecPlayer *		player);
void		swfdec_player_set_scale_mode	(SwfdecPlayer *		player,
						 SwfdecScaleMode	mode);
SwfdecAlignment	swfdec_player_get_alignment	(SwfdecPlayer *		player);
void		swfdec_player_set_alignment	(SwfdecPlayer *		player,
						 SwfdecAlignment	align);
gulong		swfdec_player_get_maximum_runtime
						(SwfdecPlayer *		player);
void		swfdec_player_set_maximum_runtime 
						(SwfdecPlayer *		player,
						 gulong			msecs);
const SwfdecURL *
		swfdec_player_get_url		(SwfdecPlayer *		player);
void		swfdec_player_set_url    	(SwfdecPlayer *		player,
						 const SwfdecURL *	url);
const SwfdecURL *
		swfdec_player_get_base_url    	(SwfdecPlayer *		player);
void		swfdec_player_set_base_url	(SwfdecPlayer *		player,
						 const SwfdecURL *	url);
const char*   	swfdec_player_get_variables   	(SwfdecPlayer *		player);
void		swfdec_player_set_variables    	(SwfdecPlayer *		player,
						 const char *		variables);
SwfdecPlayerScripting *
		swfdec_player_get_scripting	(SwfdecPlayer *		player);
void		swfdec_player_set_scripting	(SwfdecPlayer *		player,
						 SwfdecPlayerScripting *scripting);
gboolean	swfdec_player_get_focus		(SwfdecPlayer *		player);
void		swfdec_player_set_focus		(SwfdecPlayer *		player,
						 gboolean		focus);
SwfdecRenderer *swfdec_player_get_renderer	(SwfdecPlayer *		player);
void		swfdec_player_set_renderer	(SwfdecPlayer *		player,
						 SwfdecRenderer *	renderer);
					 
void		swfdec_player_render		(SwfdecPlayer *		player,
						 cairo_t *		cr,
						 double			x,
						 double			y,
						 double			width,
						 double			height);
void		swfdec_player_render_with_renderer (SwfdecPlayer *	player,
						 cairo_t *		cr,
						 SwfdecRenderer *	renderer,
						 double			x,
						 double			y,
						 double			width,
						 double			height);
gulong		swfdec_player_advance		(SwfdecPlayer *	player,
						 gulong		msecs);
gboolean	swfdec_player_mouse_move	(SwfdecPlayer *	player, 
						 double		x,
						 double		y);
gboolean	swfdec_player_mouse_press	(SwfdecPlayer *	player, 
						 double		x,
						 double		y,
						 guint		button);
gboolean	swfdec_player_mouse_release	(SwfdecPlayer *	player, 
						 double		x,
						 double		y,
						 guint		button);
gboolean	swfdec_player_key_press		(SwfdecPlayer *	player,
						 guint		keycode,
						 guint		character);
gboolean	swfdec_player_key_release	(SwfdecPlayer *	player,
						 guint		keycode,
						 guint		character);
/* audio - see swfdec_audio.c */
void		swfdec_player_render_audio	(SwfdecPlayer *	player,
						 gint16 *	dest, 
						 guint		start_offset,
						 guint		n_samples);
const GList *	swfdec_player_get_audio		(SwfdecPlayer *	player);

G_END_DECLS
#endif
