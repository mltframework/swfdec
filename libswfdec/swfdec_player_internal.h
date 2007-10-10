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

#ifndef _SWFDEC_PLAYER_INTERNAL_H_
#define _SWFDEC_PLAYER_INTERNAL_H_

#include <libswfdec/swfdec_player.h>
#include <libswfdec/swfdec_as_context.h>
#include <libswfdec/swfdec_audio.h>
#include <libswfdec/swfdec_rect.h>
#include <libswfdec/swfdec_ringbuffer.h>
#include <libswfdec/swfdec_system.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_ALIGN_FLAG_TOP		= (1 << 0),
  SWFDEC_ALIGN_FLAG_BOTTOM	= (1 << 1),
  SWFDEC_ALIGN_FLAG_LEFT	= (1 << 2),
  SWFDEC_ALIGN_FLAG_RIGHT	= (1 << 3)
} SwfdecAlignFlag;

typedef void (* SwfdecActionFunc) (gpointer object, gpointer data);
typedef gboolean (* SwfdecAudioRemoveFunc) (SwfdecAudio *audio, gpointer data);

typedef struct _SwfdecTimeout SwfdecTimeout;
struct _SwfdecTimeout {
  SwfdecTick		timestamp;		/* timestamp at which this thing is supposed to trigger */
  void			(* callback)		(SwfdecTimeout *advance);
  void			(* free)		(SwfdecTimeout *advance);
};

struct _SwfdecPlayer
{
  SwfdecAsContext	context;

  /* global properties */
  SwfdecSystem *	system;			/* our system properties */
  gboolean		initialized;		/* if width and height are set already */
  guint		  	rate;			/* divide by 256 to get iterations per second */
  guint			width;			/* width of movie */
  guint			height;			/* height of movie */
  GList *		roots;			/* all the root movies */
  GList *		load_objects;		/* all the load objects */
  SwfdecCache *		cache;			/* player cache */
  gboolean		bgcolor_set;		/* TRUE if the background color has been set */
  SwfdecColor		bgcolor;		/* background color */
  SwfdecLoader *	loader;			/* initial loader */
  /* stage properties */
  guint			internal_width;		/* width used by the scripting engine */
  guint			internal_height;	/* height used by the scripting engine */
  gint			stage_width;		/* width set by the user */
  gint			stage_height;		/* height set by the user */
  SwfdecRectangle     	stage;			/* size of the stage set by user */
  guint			align_flags;		/* SwfdecAlignFlag */
  SwfdecScaleMode	scale_mode;		/* scale mode */
  double		scale_x;		/* cached x scale value */
  double		scale_y;		/* cached y scale value */
  int			offset_x;		/* x offset from top left edge after scaling */
  int			offset_y;		/* y offset from top left edge after scaling */
  gboolean		text_format_properties_initialized; /* whether TextFormat object's native properties have been initialized */
  gboolean		xml_properties_initialized; /* whether XML object's native properties have been initialized */
  gboolean		xml_node_properties_initialized; /* same for XMLNode */

  guint			unnamed_count;		/* variable used for naming unnamed movies */
  /* ActionScript */
  guint			interval_id;		/* id returned from setInterval call */
  GList *		intervals;		/* all currently running intervals */
  GHashTable *		registered_classes;	/* name => SwfdecAsObject constructor */
  SwfdecAsObject *	MovieClip;		/* MovieClip object */
  SwfdecAsObject *	Video;			/* Video object */

  /* rendering */
  SwfdecRectangle     	invalid_extents;      	/* extents of area that needs a redraw in global coordinates */
  GArray *		invalidations;		/* fine-grained areas in need of redraw */

  /* mouse */
  gboolean		mouse_visible;	  	/* show the mouse (actionscriptable) */
  SwfdecMouseCursor	mouse_cursor;		/* cursor that should be shown */
  double      		mouse_x;		/* in stage coordinates */
  double		mouse_y;		/* in stage coordinates */
  int			mouse_button; 		/* 0 for not pressed, 1 for pressed */
  SwfdecMovie *		mouse_grab;		/* movie that currently has the mouse */
  SwfdecMovie *		mouse_drag;		/* current movie activated by startDrag */
  gboolean		mouse_drag_center;	/* TRUE to use center of movie at mouse, FALSE for movie's (0,0) */
  SwfdecRect		mouse_drag_rect;	/* clipping rectangle for movements */
  double		mouse_drag_x;		/* offset of mouse in x direction */
  double		mouse_drag_y;		/* offset of mouse in y direction */

  /* key */
  guint			last_keycode;		/* last keycode that was pressed/released */
  guint			last_character;		/* UCS4 of last character pressed/released */
  guint8		key_pressed[256/8];   	/* boolean array for isDown */

  /* audio */
  GList *		audio;		 	/* list of playing SwfdecAudio */

  /* events and advancing */
  SwfdecTick		time;			/* current time */
  GList *		timeouts;	      	/* list of events, sorted by timestamp */
  guint			tick;			/* next tick */
  SwfdecTimeout		iterate_timeout;      	/* callback for iterating */
  /* iterating */
  GList *		movies;			/* list of all moveis that want to be iterated */
  SwfdecRingBuffer *	actions;		/* all actions we've queued up so far */
  SwfdecRingBuffer *	external_actions;     	/* external actions we've queued up, like resize or loader stuff */
  SwfdecTimeout		external_timeout;      	/* callback for iterating */
  GQueue *		init_queue;		/* all movies that require an init event */
  GQueue *		construct_queue;      	/* all movies that require an construct event */
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
};

void		swfdec_player_initialize	(SwfdecPlayer *		player,
						 guint			version,
						 guint			rate,
						 guint			width,
						 guint			height);
void		swfdec_player_add_movie		(SwfdecPlayer *		player,
						 guint			depth,
						 const char *		url);
void		swfdec_player_remove_movie	(SwfdecPlayer *		player,
						 SwfdecMovie *		movie);

void		swfdec_player_lock		(SwfdecPlayer *		player);
void		swfdec_player_lock_soft		(SwfdecPlayer *		player);
void		swfdec_player_unlock		(SwfdecPlayer *		player);
void		swfdec_player_unlock_soft	(SwfdecPlayer *		player);
void		swfdec_player_perform_actions	(SwfdecPlayer *		player);

SwfdecAsObject *swfdec_player_get_export_class	(SwfdecPlayer *		player,
						 const char *		name);
void		swfdec_player_set_export_class	(SwfdecPlayer *		player,
						 const char *		name,
						 SwfdecAsObject *	object);

void		swfdec_player_invalidate	(SwfdecPlayer *		player,
						 const SwfdecRect *	rect);
void		swfdec_player_add_timeout	(SwfdecPlayer *		player,
						 SwfdecTimeout *	timeout);
void		swfdec_player_remove_timeout	(SwfdecPlayer *		player,
						 SwfdecTimeout *	timeout);
void		swfdec_player_add_external_action
						(SwfdecPlayer *		player,
						 gpointer		object,
						 SwfdecActionFunc   	action_func,
						 gpointer		action_data);
void		swfdec_player_remove_all_external_actions
						(SwfdecPlayer *      	player,
						 gpointer		object);
void		swfdec_player_add_action	(SwfdecPlayer *		player,
						 gpointer		object,
						 SwfdecActionFunc   	action_func,
						 gpointer		action_data);
void		swfdec_player_remove_all_actions (SwfdecPlayer *      	player,
						 gpointer		object);

void		swfdec_player_set_drag_movie	(SwfdecPlayer *		player,
						 SwfdecMovie *		drag,
						 gboolean		center,
						 SwfdecRect *		rect);
void		swfdec_player_set_align_flags	(SwfdecPlayer *		player,
						 guint			flags);
void		swfdec_player_stop_sounds	(SwfdecPlayer *		player,
						 SwfdecAudioRemoveFunc	func,
						 gpointer		data);
void		swfdec_player_stop_all_sounds	(SwfdecPlayer *		player);
SwfdecMovie *	swfdec_player_add_level_from_loader 
						(SwfdecPlayer *		player,
						 guint			depth,
						 SwfdecLoader *		loader,
						 const char *		variables);
void		swfdec_player_remove_level	(SwfdecPlayer *		player,
						 guint			depth);
SwfdecLoader *	swfdec_player_load		(SwfdecPlayer *         player,
						 const char *		url);
void		swfdec_player_launch		(SwfdecPlayer *         player,
						 const char *		url,
						 const char *		target);
void		swfdec_player_stage_to_global	(SwfdecPlayer *		player,
						 double *		x,
						 double *		y);
void		swfdec_player_global_to_stage	(SwfdecPlayer *		player,
						 double *		x,
						 double *		y);
/* in swfdec_as_interpret.c */
SwfdecMovie *	swfdec_player_get_movie_from_value 
						(SwfdecPlayer *		player,
						 SwfdecAsValue *	val);


G_END_DECLS
#endif
