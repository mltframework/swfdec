/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_MOVIE_H_
#define _SWFDEC_MOVIE_H_

#include <glib-object.h>
#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_as_movie_value.h>
#include <swfdec/swfdec_color.h>
#include <swfdec/swfdec.h>
#include <swfdec/swfdec_event.h>
#include <swfdec/swfdec_rect.h>
#include <swfdec/swfdec_types.h>

G_BEGIN_DECLS


typedef struct _SwfdecMovieClass SwfdecMovieClass;

/* descriptions taken from  http://www.kirupa.com/developer/actionscript/depths2.htm */
typedef enum {
  SWFDEC_DEPTH_CLASS_EMPTY,
  SWFDEC_DEPTH_CLASS_TIMELINE,
  SWFDEC_DEPTH_CLASS_DYNAMIC,
  SWFDEC_DEPTH_CLASS_RESERVED
} SwfdecDepthClass;

typedef enum {
  SWFDEC_MOVIE_STATE_RUNNING = 0,	/* the movie has been created */
  SWFDEC_MOVIE_STATE_REMOVED,		/* swfdec_movie_remove has been called */
  SWFDEC_MOVIE_STATE_DESTROYED		/* swfdec_movie_destroy has been called */
} SwfdecMovieState;

typedef enum {
  SWFDEC_FLASH_MAYBE = 0,
  SWFDEC_FLASH_YES,
  SWFDEC_FLASH_NO
} SwfdecFlashBool;

typedef enum {
  SWFDEC_MOVIE_PROPERTY_X = 0,
  SWFDEC_MOVIE_PROPERTY_Y = 1,
  SWFDEC_MOVIE_PROPERTY_XSCALE = 2,
  SWFDEC_MOVIE_PROPERTY_YSCALE = 3,
  SWFDEC_MOVIE_PROPERTY_CURRENTFRAME = 4,
  SWFDEC_MOVIE_PROPERTY_TOTALFRAMES = 5,
  SWFDEC_MOVIE_PROPERTY_ALPHA = 6,
  SWFDEC_MOVIE_PROPERTY_VISIBLE = 7,
  SWFDEC_MOVIE_PROPERTY_WIDTH = 8,
  SWFDEC_MOVIE_PROPERTY_HEIGHT = 9,
  SWFDEC_MOVIE_PROPERTY_ROTATION = 10,
  SWFDEC_MOVIE_PROPERTY_TARGET = 11,
  SWFDEC_MOVIE_PROPERTY_FRAMESLOADED = 12,
  SWFDEC_MOVIE_PROPERTY_NAME = 13,
  SWFDEC_MOVIE_PROPERTY_DROPTARGET = 14,
  SWFDEC_MOVIE_PROPERTY_URL = 15,
  SWFDEC_MOVIE_PROPERTY_HIGHQUALITY = 16,
  SWFDEC_MOVIE_PROPERTY_FOCUSRECT = 17,
  SWFDEC_MOVIE_PROPERTY_SOUNDBUFTIME = 18,
  SWFDEC_MOVIE_PROPERTY_QUALITY = 19,
  SWFDEC_MOVIE_PROPERTY_XMOUSE = 20,
  SWFDEC_MOVIE_PROPERTY_YMOUSE = 21
} SwfdecMovieProperty;

#define SWFDEC_BLEND_MODE_NONE		0
#define SWFDEC_BLEND_MODE_NORMAL	1
#define SWFDEC_BLEND_MODE_LAYER		2
#define SWFDEC_BLEND_MODE_MULTIPLY	3
#define SWFDEC_BLEND_MODE_SCREEN	4
#define SWFDEC_BLEND_MODE_LIGHTEN	5
#define SWFDEC_BLEND_MODE_DARKEN	6
#define SWFDEC_BLEND_MODE_DIFFERENCE	7
#define SWFDEC_BLEND_MODE_ADD		8
#define SWFDEC_BLEND_MODE_SUBTRACT	9
#define SWFDEC_BLEND_MODE_INVERT	10
#define SWFDEC_BLEND_MODE_ALPHA		11
#define SWFDEC_BLEND_MODE_ERASE		12
#define SWFDEC_BLEND_MODE_OVERLAY	13
#define SWFDEC_BLEND_MODE_HARDLIGHT	14

#define SWFDEC_TYPE_MOVIE                    (swfdec_movie_get_type())
#define SWFDEC_IS_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_MOVIE))
#define SWFDEC_IS_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_MOVIE))
#define SWFDEC_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_MOVIE, SwfdecMovie))
#define SWFDEC_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_MOVIE, SwfdecMovieClass))
#define SWFDEC_MOVIE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_MOVIE, SwfdecMovieClass))

/* NB: each following state includes the previous */
typedef enum {
  SWFDEC_MOVIE_UP_TO_DATE = 0,		/* everything OK */
  SWFDEC_MOVIE_INVALID_CHILDREN,	/* call update on children */
  SWFDEC_MOVIE_INVALID_EXTENTS,		/* recalculate extents */
} SwfdecMovieCacheState;

typedef void (*SwfdecMovieVariableListenerFunction) (gpointer data,
    const char *name, const SwfdecAsValue *val);

typedef struct {
  gpointer				data;
  const char *				name;
  SwfdecMovieVariableListenerFunction	function;
} SwfdecMovieVariableListener;

struct _SwfdecMovie {
  SwfdecAsObject	object;

  SwfdecGraphic *	graphic;		/* graphic represented by this movie or NULL if script-created */
  const char *		name;		/* name of movie - GC'd */
  GList *		list;			/* our contained movie clips (ordered by depth) */
  int			depth;			/* depth of movie (equals content->depth unless explicitly set) */
  SwfdecMovieCacheState	cache_state;		/* whether we are up to date */
  SwfdecMovieState	state;			/* state the movie is in */
  GSList		*variable_listeners;	/* textfield's listening to changes in variables - SwfdecMovieVariableListener */

  /* static properties (set by PlaceObject tags) */
  cairo_matrix_t	original_transform;	/* initial transform used */
  guint			original_ratio;		/* ratio used in this movie */
  int			clip_depth;		/* up to which movie this movie clips */

  /* scripting stuff */
  SwfdecAsMovieValue *	as_value;		/* This movie's value in the script engine or %NULL if not accessible by scripts */

  /* parenting information */
  SwfdecMovie *		parent;			/* movie that contains us or NULL for root movies */
  gboolean		lockroot;		/* when looking for _root we should use this movie, even if it has a parent */
  SwfdecResource *	resource;     		/* the resource that created us */

  /* positioning - the values are applied in this order */
  SwfdecRect		extents;		/* the extents occupied after transform is applied */
  SwfdecRect		original_extents;	/* the extents from all children - unmodified */
  gboolean		modified;		/* TRUE if the transform has been modified by scripts */
  double		xscale;			/* x scale in percent */
  double		yscale;			/* y scale in percent */
  double		rotation;		/* rotation in degrees [-180, 180] */
  cairo_matrix_t	matrix;			/* cairo matrix computed from above and content->transform */
  cairo_matrix_t	inverse_matrix;		/* the inverse of the cairo matrix */
  SwfdecColorTransform	color_transform;	/* scripted color transformation */
  guint			blend_mode;		/* blend mode to use - see to-cairo conversion code for what they mean */

  /* iteration state */
  gboolean		visible;		/* whether we currently can be seen or iterate */

  /* drawing state */
  SwfdecMovie *		mask_of;		/* movie this movie is a mask of or NULL if none */
  SwfdecMovie *		masked_by;		/* movie we are masked by or NULL if none */
  GSList *		filters;		/* filters to apply to movie */
  gboolean		cache_as_bitmap;	/* the movie should be cached */
  /* FIXME: could it be that shape drawing (SwfdecGraphicMovie etc) uses these same objects? */
  SwfdecImage *		image;			/* image loaded via loadMovie */
  SwfdecRect		draw_extents;		/* extents of the items in the following list */
  GSList *		draws;			/* all the items to draw */
  SwfdecDraw *		draw_fill;	      	/* current fill style or NULL */
  SwfdecDraw *		draw_line;	      	/* current line style or NULL */
  int			draw_x;			/* current x position for drawing */
  int			draw_y;			/* current y position for drawing */
  
  /* invalidatation state */
  gboolean		invalidate_last;	/* TRUE if this movie's previous contents are already invalidated */
  gboolean		invalidate_next;	/* TRUE if this movie should be invalidated before unlocking */

  /* leftover unimplemented variables from the Actionscript spec */
#if 0
  int droptarget;
#endif
};

struct _SwfdecMovieClass {
  SwfdecAsObjectClass	object_class;

  /* general vfuncs */
  void			(* init_movie)		(SwfdecMovie *		movie);
  void			(* finish_movie)	(SwfdecMovie *		movie);
  void			(* property_get)	(SwfdecMovie *		movie,
						 guint			prop_id,
						 SwfdecAsValue *	value);
  void			(* property_set)	(SwfdecMovie *		movie,
						 guint			prop_id,
						 const SwfdecAsValue *	value);
  void			(* replace)		(SwfdecMovie *		movie,
						 SwfdecGraphic *	graphic);
  void			(* set_ratio)		(SwfdecMovie *		movie);
  void			(* update_extents)	(SwfdecMovie *		movie,
						 SwfdecRect *   	extents);
  void			(* render)		(SwfdecMovie *		movie, 
						 cairo_t *		cr,
						 const SwfdecColorTransform *trans);
  void			(* invalidate)		(SwfdecMovie *		movie,
						 const cairo_matrix_t *	movie_to_global,
						 gboolean		new_contents);

  SwfdecMovie *		(* contains)		(SwfdecMovie *		movie,
						 double			x,
						 double			y,
						 gboolean		events);
};

GType		swfdec_movie_get_type		(void);

SwfdecMovie *	swfdec_movie_new		(SwfdecPlayer *		player,
						 int			depth,
						 SwfdecMovie *		parent,
						 SwfdecResource *	resource,
						 SwfdecGraphic *	graphic,
						 const char *		name);
SwfdecMovie *	swfdec_movie_duplicate		(SwfdecMovie *		movie, 
						 const char *		name,
						 int			depth);
void		swfdec_movie_initialize		(SwfdecMovie *		movie);
SwfdecMovie *	swfdec_movie_find		(SwfdecMovie *		movie,
						 int			depth);
SwfdecMovie *	swfdec_movie_get_by_name	(SwfdecMovie *		movie,
						 const char *		name,
						 gboolean		unnamed);
SwfdecMovie *	swfdec_movie_get_root		(SwfdecMovie *		movie);
void		swfdec_movie_property_set	(SwfdecMovie *		movie,
						 guint			id, 
						 const SwfdecAsValue *	val);
void		swfdec_movie_property_get	(SwfdecMovie *		movie,
						 guint			id, 
						 SwfdecAsValue *	val);
void		swfdec_movie_call_variable_listeners 
						(SwfdecMovie *		movie,
						 const char *		name,
						 const SwfdecAsValue *	val);
void		swfdec_movie_remove		(SwfdecMovie *		movie);
void		swfdec_movie_destroy		(SwfdecMovie *		movie);
void		swfdec_movie_set_static_properties 
						(SwfdecMovie *		movie,
						 const cairo_matrix_t *	transform,
						 const SwfdecColorTransform *ctrans,
						 int			ratio,
						 int			clip_depth,
						 guint			blend_mode,
						 SwfdecEventList *	events);
void		swfdec_movie_invalidate_last	(SwfdecMovie *		movie);
void		swfdec_movie_invalidate_next	(SwfdecMovie *		movie);
void		swfdec_movie_invalidate		(SwfdecMovie *		movie,
						 const cairo_matrix_t *	parent_to_global,
						 gboolean		last);
void		swfdec_movie_queue_update	(SwfdecMovie *		movie,
						 SwfdecMovieCacheState	state);
void		swfdec_movie_update		(SwfdecMovie *		movie);
void		swfdec_movie_begin_update_matrix(SwfdecMovie *		movie);
void		swfdec_movie_end_update_matrix	(SwfdecMovie *		movie);
void		swfdec_movie_local_to_global	(SwfdecMovie *		movie,
						 double *		x,
						 double *		y);
void		swfdec_movie_global_to_local	(SwfdecMovie *		movie,
						 double *		x,
						 double *		y);
void		swfdec_movie_global_to_local_matrix 
						(SwfdecMovie *		movie,
						 cairo_matrix_t *	matrix);
void		swfdec_movie_local_to_global_matrix 
						(SwfdecMovie *		movie,
						 cairo_matrix_t *	matrix);
void		swfdec_movie_rect_local_to_global (SwfdecMovie *	movie,
						 SwfdecRect *		rect);
void		swfdec_movie_rect_global_to_local (SwfdecMovie *	movie,
						 SwfdecRect *		rect);
void		swfdec_movie_set_depth		(SwfdecMovie *		movie,
						 int			depth);

void		swfdec_movie_get_mouse		(SwfdecMovie *		movie,
						 double *		x,
						 double *		y);
#define swfdec_movie_contains(movie, x, y) \
  (swfdec_movie_get_movie_at ((movie), (x), (y), FALSE) != NULL)
SwfdecMovie *	swfdec_movie_get_movie_at	(SwfdecMovie *		movie,
						 double			x,
						 double			y,
						 gboolean		events);
char *		swfdec_movie_get_path		(SwfdecMovie *		movie,
						 gboolean		dot);
void		swfdec_movie_render		(SwfdecMovie *		movie,
						 cairo_t *		cr, 
						 const SwfdecColorTransform *trans);
gboolean	swfdec_movie_is_scriptable	(SwfdecMovie *		movie);
guint		swfdec_movie_get_version	(SwfdecMovie *		movie);

int		swfdec_movie_compare_depths	(gconstpointer		a,
						 gconstpointer		b);
SwfdecDepthClass
		swfdec_depth_classify		(int			depth);

/* in swfdec_movie_asprops.c */
guint		swfdec_movie_property_lookup	(const char *		name);
void		swfdec_movie_property_do_set	(SwfdecMovie *		movie,
						 guint			id, 
						 const SwfdecAsValue *	val);
void		swfdec_movie_property_do_get	(SwfdecMovie *		movie,
						 guint			id, 
						 SwfdecAsValue *	val);

void		swfdec_movie_add_variable_listener (SwfdecMovie *	movie,
						 gpointer		data,
						 const char *		name,
						 const SwfdecMovieVariableListenerFunction	function);
void		swfdec_movie_remove_variable_listener (SwfdecMovie *	movie,
						 gpointer		data,
						 const char *		name,
						 const SwfdecMovieVariableListenerFunction	function);
SwfdecResource *swfdec_movie_get_own_resource	(SwfdecMovie *		movie);

G_END_DECLS
#endif
