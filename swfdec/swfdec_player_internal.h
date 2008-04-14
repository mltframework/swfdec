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

#include <swfdec/swfdec_player.h>
#include <swfdec/swfdec_audio.h>
#include <swfdec/swfdec_cache.h>
#include <swfdec/swfdec_event.h>
#include <swfdec/swfdec_function_list.h>
#include <swfdec/swfdec_loader.h>
#include <swfdec/swfdec_player_scripting.h>
#include <swfdec/swfdec_rect.h>
#include <swfdec/swfdec_ringbuffer.h>
#include <swfdec/swfdec_socket.h>
#include <swfdec/swfdec_system.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_ALIGN_FLAG_TOP		= (1 << 0),
  SWFDEC_ALIGN_FLAG_BOTTOM	= (1 << 1),
  SWFDEC_ALIGN_FLAG_LEFT	= (1 << 2),
  SWFDEC_ALIGN_FLAG_RIGHT	= (1 << 3)
} SwfdecAlignFlag;

typedef void (* SwfdecActionFunc) (gpointer object, gpointer data);
typedef gboolean (* SwfdecAudioRemoveFunc) (SwfdecAudio *audio, gpointer data);
typedef void (* SwfdecPolicyFunc) (SwfdecPlayer *player, gboolean allow, gpointer data);

typedef struct _SwfdecTimeout SwfdecTimeout;
struct _SwfdecTimeout {
  SwfdecTick		timestamp;		/* timestamp at which this thing is supposed to trigger */
  void			(* callback)		(SwfdecTimeout *advance);
  void			(* free)		(SwfdecTimeout *advance);
};

#define SWFDEC_PLAYER_N_ACTION_QUEUES 4
#define SWFDEC_PLAYER_ACTION_QUEUE_INIT 0
#define SWFDEC_PLAYER_ACTION_QUEUE_CONSTRUCT 1
#define SWFDEC_PLAYER_ACTION_QUEUE_NORMAL 2
#define SWFDEC_PLAYER_ACTION_QUEUE_PRIORITY 3

struct _SwfdecPlayerPrivate
{
  SwfdecPlayer *	player;			/* backlink */

  /* global properties */
  SwfdecSystem *	system;			/* our system properties */
  gboolean		initialized;		/* if width and height are set already */
  guint		  	rate;			/* divide by 256 to get iterations per second */
  guint			width;			/* width of movie */
  guint			height;			/* height of movie */
  GList *		roots;			/* all the root movies */
  SwfdecCache *		cache;			/* player cache */
  SwfdecResource *	resource;		/* initial resource loaded */
  char *		variables;		/* variables to set on the player */
  SwfdecURL *		url;			/* url or NULL if not set yet */
  SwfdecURL *		base_url;	      	/* base url or NULL if not set yet */
  SwfdecRenderer *	renderer;		/* the renderer to use */
  SwfdecPlayerScripting *scripting;		/* scripting object */
  GHashTable *		scripting_callbacks;	/* GC string => SwfdecAsFunction mapping of script callbacks */
  GType			loader_type;		/* type to use for creating sockets */
  GType			socket_type;		/* type to use for creating sockets */
  gboolean		has_focus;		/* TRUE if this movie is given focus */
  gboolean		allow_fullscreen;	/* TRUE if this movie may go fullscreen */
  /* stage properties */
  guint			internal_width;		/* width used by the scripting engine */
  guint			internal_height;	/* height used by the scripting engine */
  guint			broadcasted_width;	/* width given in the last onResize event */
  guint			broadcasted_height;	/* height given in the last onResize event */
  gint			stage_width;		/* width set by the user */
  gint			stage_height;		/* height set by the user */
  SwfdecRectangle     	stage;			/* size of the stage set by user */
  guint			align_flags;		/* SwfdecAlignFlag */
  SwfdecScaleMode	scale_mode;		/* scale mode */
  double		scale_x;		/* cached x scale value */
  double		scale_y;		/* cached y scale value */
  int			offset_x;		/* x offset from top left edge after scaling */
  int			offset_y;		/* y offset from top left edge after scaling */

  SwfdecFunctionList	resource_requests;	/* all external requested URIs - see swfdec_resource_request.[ch] */
  guint			unnamed_count;		/* variable used for naming unnamed movies */
  /* ActionScript */
  SwfdecFunctionList	rooted;	  		/* all the objects we keep track of */
  guint			interval_id;		/* id returned from setInterval call */
  GList *		intervals;		/* all currently running intervals */
  GHashTable *		registered_classes;	/* name => SwfdecAsObject constructor */

  /* rendering */
  SwfdecRectangle     	invalid_extents;      	/* extents of area that needs a redraw in global coordinates */
  GArray *		invalidations;		/* fine-grained areas in need of redraw */
  GSList *		invalid_pending;	/* pending invalidations due to invalidate_last */
  gboolean		fullscreen;		/* TRUE if the player has gone fullscreen */

  /* mouse */
  gboolean		mouse_visible;	  	/* show the mouse (actionscriptable) */
  SwfdecMouseCursor	mouse_cursor;		/* cursor that should be shown */
  double      		mouse_x;		/* in stage coordinates */
  double		mouse_y;		/* in stage coordinates */
  guint			mouse_button; 		/* 0 for not pressed, 1 for pressed */
  SwfdecActor *		mouse_below;		/* actor that currently is below the mouse */
  SwfdecActor *		mouse_grab;		/* actor that currently has the mouse */
  SwfdecActor *		mouse_drag;		/* current actor activated by startDrag */
  gboolean		mouse_drag_center;	/* TRUE to use center of movie at mouse, FALSE for movie's (0,0) */
  SwfdecRect		mouse_drag_rect;	/* clipping rectangle for movements */
  double		mouse_drag_x;		/* offset of mouse in x direction */
  double		mouse_drag_y;		/* offset of mouse in y direction */

  /* key */
  guint			last_keycode;		/* last keycode that was pressed/released */
  guint			last_character;		/* UCS4 of last character pressed/released */
  guint8		key_pressed[256/8];   	/* boolean array for isDown */
  SwfdecActor *		focus;			/* actor that currently has keyboard focus (or NULL if none) */
  SwfdecActor *		focus_previous;		/* the previues actor that had focus */
  GList *		focus_list;	      	/* list of movies with a tabIndex set or NULL for no tabIndex usage */
  SwfdecRect		focusrect;		/* current focus rectangle in global coordinates or empty */

  /* audio */
  GList *		audio;		 	/* list of playing SwfdecAudio */
  GSList *		missing_plugins;	/* list of GStreamer detail strings for missing plugins */

  /* events and advancing */
  SwfdecTick		time;			/* current time */
  GList *		timeouts;	      	/* list of events, sorted by timestamp */
  guint			tick;			/* next tick */
  SwfdecTimeout		iterate_timeout;      	/* callback for iterating */
  GTimer *		runtime;		/* for checking how long we've been running */
  gulong		max_runtime;		/* maximum number of seconds the player may run */
  SwfdecRingBuffer *	external_actions;     	/* external actions we've queued up, like resize or loader stuff */
  SwfdecTimeout		external_timeout;      	/* callback for iterating */
  /* iterating */
  GList *		actors;			/* list of all SwfdecActor isntances active in this player */
  SwfdecRingBuffer *	actions[SWFDEC_PLAYER_N_ACTION_QUEUES]; /* all actions we've queued up so far */

  /* security */
  GSList *		sandboxes;		/* all existing sandboxes */
  GList *		loading_policy_files;	/* list of loading SwfdecPlayerLoader - newest first */
  GSList *		policy_files;		/* list of SwfdecPolicyLoader that finished loading */
};

void		swfdec_player_initialize	(SwfdecPlayer *		player,
						 guint			rate,
						 guint			width,
						 guint			height);
void		swfdec_player_start_ticking	(SwfdecPlayer *		player);

gboolean	swfdec_player_lock		(SwfdecPlayer *		player);
void		swfdec_player_lock_soft		(SwfdecPlayer *		player);
void		swfdec_player_unlock		(SwfdecPlayer *		player);
void		swfdec_player_unlock_soft	(SwfdecPlayer *		player);
void		swfdec_player_perform_actions	(SwfdecPlayer *		player);

#define swfdec_player_root(player, data, mark_func) \
    swfdec_function_list_add (&(player)->priv->rooted, (mark_func), (data), NULL)
#define swfdec_player_root_full(player, data, mark_func, destroy_notify) \
    swfdec_function_list_add (&(player)->priv->rooted, (mark_func), (data), (destroy_notify))
#define swfdec_player_unroot(player, data) \
    swfdec_function_list_remove (&(player)->priv->rooted, (data))
#define swfdec_player_request_resource(player, request_func, data, destroy_notify) \
    swfdec_function_list_add (&(player)->priv->resource_requests, (request_func), (data), (destroy_notify))
SwfdecURL *	swfdec_player_create_url	(SwfdecPlayer *		player,
						 const char *		string);
SwfdecLoader *	swfdec_player_load		(SwfdecPlayer *		player,
						 const char *		url,
						 SwfdecLoaderRequest	request,
						 SwfdecBuffer *		buffer);
SwfdecAsObject *swfdec_player_get_export_class	(SwfdecPlayer *		player,
						 const char *		name);
void		swfdec_player_set_export_class	(SwfdecPlayer *		player,
						 const char *		name,
						 SwfdecAsObject *	object);

SwfdecSocket *	swfdec_player_create_socket	(SwfdecPlayer *		player,
						 const char *		hostname,
						 guint			port);

void		swfdec_player_invalidate_focusrect (SwfdecPlayer *	player);
void		swfdec_player_grab_focus	(SwfdecPlayer *		player,
						 SwfdecActor *		actor);
#define swfdec_player_has_focus(player,actor) ((player)->priv->focus == (actor))
#define swfdec_player_is_key_pressed(player,key) ((player)->priv->key_pressed[(key) / 8] & (1 << ((key) % 8)))
#define swfdec_player_is_mouse_pressed(player) ((player)->priv->mouse_button & 1)
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
						 SwfdecActor *		actor,
						 SwfdecEventType	type,
						 guint			importance);
void		swfdec_player_add_action_script	(SwfdecPlayer *		player,
						 SwfdecActor *		actor,
						 SwfdecScript *		script,
						 guint			importance);
void		swfdec_player_remove_all_actions (SwfdecPlayer *      	player,
						 SwfdecActor *		actor);

void		swfdec_player_set_fullscreen	(SwfdecPlayer *		player,
						 gboolean		fullscreen);
void		swfdec_player_set_drag_movie	(SwfdecPlayer *		player,
						 SwfdecActor *		drag,
						 gboolean		center,
						 SwfdecRect *		rect);
void		swfdec_player_set_align_flags	(SwfdecPlayer *		player,
						 guint			flags);
void		swfdec_player_stop_sounds	(SwfdecPlayer *		player,
						 SwfdecAudioRemoveFunc	func,
						 gpointer		data);
void		swfdec_player_stop_all_sounds	(SwfdecPlayer *		player);
gboolean	swfdec_player_get_level		(SwfdecPlayer *		player,
						 const char *		name);
SwfdecSpriteMovie *
		swfdec_player_get_movie_at_level(SwfdecPlayer *		player,
						 int			level);
SwfdecSpriteMovie *
		swfdec_player_create_movie_at_level 
						(SwfdecPlayer *		player,
						 SwfdecResource *	resource,
						 int			level);
void		swfdec_player_launch		(SwfdecPlayer *         player,
						 SwfdecLoaderRequest	request,
						 const char *		url,
						 const char *		target,
						 SwfdecBuffer *		data);
void		swfdec_player_stage_to_global	(SwfdecPlayer *		player,
						 double *		x,
						 double *		y);
void		swfdec_player_global_to_stage	(SwfdecPlayer *		player,
						 double *		x,
						 double *		y);
void		swfdec_player_update_scale	(SwfdecPlayer *		player);
void		swfdec_player_add_missing_plugin(SwfdecPlayer *		player,
						 const char *		detail);

/* in swfdec_policy_file.c */
gboolean	swfdec_player_allow_now		(SwfdecPlayer *		player,
						 const SwfdecURL *	url);
void	      	swfdec_player_allow_or_load	(SwfdecPlayer *		player,
						 const SwfdecURL *	url,
						 const SwfdecURL *	load_url,
						 SwfdecPolicyFunc	func,
						 gpointer		data);
/* in swfdec_as_interpret.c */
SwfdecMovie *	swfdec_player_get_movie_from_value 
						(SwfdecPlayer *		player,
						 SwfdecAsValue *	val);
SwfdecMovie *	swfdec_player_get_movie_from_string
						(SwfdecPlayer *		player,
						 const char *		s);


G_END_DECLS
#endif
