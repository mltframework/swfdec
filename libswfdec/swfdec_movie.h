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

struct _SwfdecContent {
  SwfdecGraphic *	graphic;	/* object to display or NULL */
  int	         	depth;		/* at which depth to display */
  int			clip_depth;	/* clip depth of object */
  guint		ratio;
  cairo_matrix_t	transform;
  SwfdecColorTransform	color_transform;
  char *		name;
  SwfdecEventList *	events;

  SwfdecContent *	sequence;	/* first element in sequence this content belongs to */
  /* NB: the next two elements are only filled for the sequence leader */
  guint			start;		/* first frame that contains this sequence */
  guint			end;		/* first frame that does not contain this sequence anymore */
};
#define SWFDEC_CONTENT_DEFAULT { NULL, -1, 0, 0, { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 }, \
  { 256, 0, 256, 0, 256, 0, 256, 0 }, NULL, NULL, NULL, 0, G_MAXUINT }

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
  SWFDEC_MOVIE_INVALID_AREA,
  SWFDEC_MOVIE_INVALID_MATRIX,
} SwfdecMovieState;

struct _SwfdecMovie {
  SwfdecAsObject	object;

  char *		name;			/* name used in to_string */
  gboolean		has_name;		/* TRUE if name wasn't given automagically */
  GList *		list;			/* our contained movie clips (ordered by depth) */
  int			depth;			/* depth of movie (equals content->depth unless explicitly set) */
  const SwfdecContent *	content;           	/* the content we are displaying */
  SwfdecMovieState	cache_state;		/* whether we are up to date */

  /* parenting information */
  SwfdecMovie *		root;			/* root movie for this movie */
  SwfdecMovie *		parent;			/* movie that contains us or NULL for root */

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

  /* iteration state */
  guint			frame;			/* current frame */
  guint			n_frames;		/* amount of frames */
  gboolean		stopped;		/* if we currently iterate */
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
  void			(* content_changed)	(SwfdecMovie *		movie,
						 const SwfdecContent *	content);
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
  void			(* goto_frame)		(SwfdecMovie *		movie,
						 guint			frame);
  void			(* iterate_start)     	(SwfdecMovie *		movie);
  gboolean		(* iterate_end)		(SwfdecMovie *		movie);
};

GType		swfdec_movie_get_type		(void);

SwfdecMovie *	swfdec_movie_new		(SwfdecMovie *		parent,
						 const SwfdecContent *	content);
SwfdecMovie *	swfdec_movie_new_for_player	(SwfdecPlayer *		player,
						 guint			depth);
SwfdecMovie *	swfdec_movie_find		(SwfdecMovie *		movie,
						 int			depth);
void		swfdec_movie_remove		(SwfdecMovie *		movie);
void		swfdec_movie_destroy		(SwfdecMovie *		movie);
void		swfdec_movie_set_content	(SwfdecMovie *		movie,
						 const SwfdecContent *	content);
void		swfdec_movie_invalidate		(SwfdecMovie *		movie);
void		swfdec_movie_queue_update	(SwfdecMovie *		movie,
						 SwfdecMovieState	state);
void		swfdec_movie_update		(SwfdecMovie *		movie);
void		swfdec_movie_local_to_global	(SwfdecMovie *		movie,
						 double *		x,
						 double *		y);
void		swfdec_movie_global_to_local	(SwfdecMovie *		movie,
						 double *		x,
						 double *		y);
void		swfdec_movie_get_mouse		(SwfdecMovie *		movie,
						 double *		x,
						 double *		y);
void		swfdec_movie_send_mouse_change	(SwfdecMovie *		movie,
						 gboolean		release);
SwfdecMovie *	swfdec_movie_get_movie_at	(SwfdecMovie *		movie,
						 double			x,
						 double			y);
char *		swfdec_movie_get_path		(SwfdecMovie *		movie);
void		swfdec_movie_render		(SwfdecMovie *		movie,
						 cairo_t *		cr, 
						 const SwfdecColorTransform *trans,
						 const SwfdecRect *	inval,
						 gboolean		fill);
void		swfdec_movie_goto		(SwfdecMovie *		movie,
						 guint			frame);
void		swfdec_movie_execute_script	(SwfdecMovie *		movie,
						 SwfdecEventType	condition);
gboolean      	swfdec_movie_queue_script	(SwfdecMovie *		movie,
  						 SwfdecEventType	condition);
void		swfdec_movie_set_variables	(SwfdecMovie *		movie,
						 const char *		variables);
int		swfdec_movie_compare_depths	(gconstpointer		a,
						 gconstpointer		b);
SwfdecDepthClass
		swfdec_depth_classify		(int			depth);

/* in swfdec_js_movie.c */
void		swfdec_movie_run_init		(SwfdecMovie *		movie);
void		swfdec_movie_run_construct	(SwfdecMovie *		movie);

G_END_DECLS
#endif
