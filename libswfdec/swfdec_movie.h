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

#ifndef _SWFDEC_MOVIE_H_
#define _SWFDEC_MOVIE_H_

#include <glib-object.h>
#include <libswfdec/swfdec_as_object.h>
#include <libswfdec/swfdec_color.h>
#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_event.h>
#include <libswfdec/swfdec_rect.h>
#include <libswfdec/swfdec_types.h>

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

struct _SwfdecContent {
  SwfdecGraphic *	graphic;	/* object to display */
  int	         	depth;		/* at which depth to display */
  int			clip_depth;	/* clip depth of object */
  guint			ratio;
  cairo_matrix_t	transform;
  SwfdecColorTransform	color_transform;
  gboolean		has_transform : 1;
  gboolean		has_color_transform : 1;
  char *		name;
  SwfdecEventList *	events;
  guint			blend_mode;	/* operator to use when painting (aka blend mode) */   

  /* only used by buttons */
  SwfdecContent *	sequence;
  guint			start;
  guint			end;
};
#define SWFDEC_CONTENT_DEFAULT { NULL, -1, 0, 0, { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 }, \
  { 256, 0, 256, 0, 256, 0, 256, 0 }, NULL, NULL, CAIRO_OPERATOR_OVER, NULL, 0, G_MAXUINT, FALSE }

#define SWFDEC_TYPE_MOVIE                    (swfdec_movie_get_type())
#define SWFDEC_IS_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_MOVIE))
#define SWFDEC_IS_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_MOVIE))
#define SWFDEC_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_MOVIE, SwfdecMovie))
#define SWFDEC_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_MOVIE, SwfdecMovieClass))
#define SWFDEC_MOVIE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_MOVIE, SwfdecMovieClass))

typedef enum {
  SWFDEC_MOVIE_UP_TO_DATE = 0,
  SWFDEC_MOVIE_INVALID_CHILDREN,
  SWFDEC_MOVIE_INVALID_EXTENTS,
  SWFDEC_MOVIE_INVALID_MATRIX
} SwfdecMovieCacheState;

struct _SwfdecMovie {
  SwfdecAsObject	object;

  SwfdecGraphic *	graphic;		/* graphic represented by this movie or NULL if script-created */
  const char *		name;			/* name of movie - GC'd */
  GList *		list;			/* our contained movie clips (ordered by depth) */
  int			depth;			/* depth of movie (equals content->depth unless explicitly set) */
  SwfdecMovieCacheState	cache_state;		/* whether we are up to date */
  SwfdecMovieState	state;			/* state the movie is in */

  /* static properties (set by PlaceObject tags) */
  const char *		original_name;		/* the original name - GC'd and static */
  SwfdecColorTransform	original_ctrans;	/* color transform set on this movie */
  cairo_matrix_t	original_transform;	/* initial transform used */
  guint			original_ratio;		/* ratio used in this movie */
  int			clip_depth;		/* up to which movie this movie clips */
  SwfdecEventList *	events;			/* events queued on this movie */

  /* parenting information */
  SwfdecMovie *		parent;			/* movie that contains us or NULL for root movies */
  SwfdecSwfInstance *	swf;			/* the instance that created us */

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
  gboolean		will_be_removed;	/* it's known that this movie will not survive the next iteration */

  /* leftover unimplemented variables from the Actionscript spec */
#if 0
  int droptarget;
  char *target;
  char *url;
#endif
};

struct _SwfdecMovieClass {
  SwfdecAsObjectClass	object_class;

  /* general vfuncs */
  void			(* init_movie)		(SwfdecMovie *		movie);
  void			(* finish_movie)	(SwfdecMovie *		movie);
  void			(* replace)		(SwfdecMovie *		movie,
						 SwfdecGraphic *	graphic);
  void			(* update_extents)	(SwfdecMovie *		movie,
						 SwfdecRect *   	extents);
  void			(* render)		(SwfdecMovie *		movie, 
						 cairo_t *		cr,
						 const SwfdecColorTransform *trans,
						 const SwfdecRect *	inval,
						 gboolean		fill);

  /* mouse handling */
  gboolean		(* mouse_in)		(SwfdecMovie *		movie,
						 double			x,
						 double			y);
  void			(* mouse_change)      	(SwfdecMovie *		movie,
						 double			x,
						 double			y,
						 gboolean		mouse_in,
						 int			button);

  /* iterating */
  void			(* iterate_start)     	(SwfdecMovie *		movie);
  gboolean		(* iterate_end)		(SwfdecMovie *		movie);
};

GType		swfdec_movie_get_type		(void);

SwfdecMovie *	swfdec_movie_new		(SwfdecPlayer *		player,
						 int			depth,
						 SwfdecMovie *		parent,
						 SwfdecGraphic *	graphic,
						 const char *		name);
SwfdecMovie *	swfdec_movie_new_for_content  	(SwfdecMovie *		parent,
						 const SwfdecContent *	content);
SwfdecMovie *	swfdec_movie_duplicate		(SwfdecMovie *		movie, 
						 const char *		name,
						 int			depth);
void		swfdec_movie_initialize		(SwfdecMovie *		movie);
SwfdecMovie *	swfdec_movie_find		(SwfdecMovie *		movie,
						 int			depth);
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
void		swfdec_movie_invalidate		(SwfdecMovie *		movie);
void		swfdec_movie_queue_update	(SwfdecMovie *		movie,
						 SwfdecMovieCacheState	state);
void		swfdec_movie_update		(SwfdecMovie *		movie);
void		swfdec_movie_local_to_global	(SwfdecMovie *		movie,
						 double *		x,
						 double *		y);
void		swfdec_movie_global_to_local	(SwfdecMovie *		movie,
						 double *		x,
						 double *		y);
void		swfdec_movie_rect_local_to_global (SwfdecMovie *	movie,
						 SwfdecRect *		rect);
void		swfdec_movie_rect_global_to_local (SwfdecMovie *	movie,
						 SwfdecRect *		rect);
void		swfdec_movie_set_depth		(SwfdecMovie *		movie,
						 int			depth);
void		swfdec_movie_get_mouse		(SwfdecMovie *		movie,
						 double *		x,
						 double *		y);
void		swfdec_movie_send_mouse_change	(SwfdecMovie *		movie,
						 gboolean		release);
gboolean	swfdec_movie_mouse_in		(SwfdecMovie *		movie,
						 double			x,
						 double			y);
SwfdecMovie *	swfdec_movie_get_movie_at	(SwfdecMovie *		movie,
						 double			x,
						 double			y);
char *		swfdec_movie_get_path		(SwfdecMovie *		movie);
void		swfdec_movie_render		(SwfdecMovie *		movie,
						 cairo_t *		cr, 
						 const SwfdecColorTransform *trans,
						 const SwfdecRect *	inval,
						 gboolean		fill);
void		swfdec_movie_execute_script	(SwfdecMovie *		movie,
						 SwfdecEventType	condition);
gboolean      	swfdec_movie_queue_script	(SwfdecMovie *		movie,
  						 SwfdecEventType	condition);
void		swfdec_movie_set_variables	(SwfdecMovie *		movie,
						 const char *		variables);
void		swfdec_movie_load		(SwfdecMovie *		movie,
						 const char *		url,
						 const char *		target,
						 SwfdecLoaderRequest	request,
						 const char *		data,
						 gsize			data_len);

int		swfdec_movie_compare_depths	(gconstpointer		a,
						 gconstpointer		b);
SwfdecDepthClass
		swfdec_depth_classify		(int			depth);

void		swfdec_movie_run_init		(SwfdecMovie *		movie);
void		swfdec_movie_run_construct	(SwfdecMovie *		movie);
/* in swfdec_movie_asprops.c */
gboolean	swfdec_movie_set_asprop		(SwfdecMovie *		movie,
						 const char *		name,
						 const SwfdecAsValue *	val);
gboolean	swfdec_movie_get_asprop		(SwfdecMovie *		movie,
						 const char *		name,
						 SwfdecAsValue *	val);

G_END_DECLS
#endif
