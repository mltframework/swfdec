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

#ifndef _SWFDEC_MOVIE_H_
#define _SWFDEC_MOVIE_H_

#include <glib-object.h>
#include <libswfdec/swfdec_color.h>
#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_rect.h>
#include <libswfdec/swfdec_types.h>
#include <libswfdec/js/jspubtd.h>

G_BEGIN_DECLS


typedef struct _SwfdecMovieClass SwfdecMovieClass;

struct _SwfdecContent {
  SwfdecGraphic *	graphic;	/* object to display or NULL */
  unsigned int	      	depth;		/* at which depth to display */
  unsigned int		clip_depth;	/* clip depth of object */
  unsigned int		ratio;
  cairo_matrix_t	transform;
  SwfdecColorTransform	color_transform;
  char *		name;
  SwfdecEventList *	events;

  SwfdecContent *	sequence;	/* first element in sequence this content belongs to */
  /* NB: the next two elements are only filled for the sequence leader */
  guint			start;		/* first frame that contains this sequence */
  guint			end;		/* first frame that does not contain this sequence anymore */
};

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
  GObject		object;

  char *		name;			/* name used in to_string */
  gboolean		has_name;		/* TRUE if name wasn't given automagically */
  char *		_name;			/* name for use by _name property */
  JSObject *		jsobj;			/* our object in javascript */
  GList *		list;			/* our contained movie clips (ordered by depth) */
  guint			depth;			/* depth of movie (equals content->depth unless explicitly set) */
  const SwfdecContent *	content;           	/* the content we are displaying */
  SwfdecMovieState	cache_state;		/* whether we are up to date */

  /* parenting information */
  SwfdecMovie *		root;			/* root movie for this movie */
  SwfdecMovie *		parent;			/* movie that contains us or NULL for root */

  /* positioning - the values are applied in this order */
  SwfdecRect		extents;		/* the extents occupied after variables are applied */
  SwfdecRect		original_extents;	/* the extents from all the children */
  double      		x;			/* x offset in twips */
  double		y;	      		/* y offset in twips */
  double      		xscale;			/* x scale factor */
  double      		yscale;			/* y scale factor */
  int			rotation;     		/* rotation in degrees */
  cairo_matrix_t	transform;		/* transformation matrix computed from above */
  cairo_matrix_t	inverse_transform;	/* the inverse of the transformation matrix */
  SwfdecColorTransform	color_transform;	/* scripted color transformation */

  /* iteration state */
  guint			frame;			/* current frame */
  guint			n_frames;		/* amount of frames */
  gboolean		stopped;		/* if we currently iterate */
  gboolean		visible;		/* whether we currently can be seen or iterate */
  gboolean		will_be_removed;	/* it's known that this movie will not survive the next iteration */

  /* leftover unimplemented variables from the Actionscript spec */
  //droptarget
  char *target;
  char *url;
};

struct _SwfdecMovieClass {
  GObjectClass		object_class;

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
						 guint			depth);
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
void		swfdec_movie_render		(SwfdecMovie *		movie,
						 cairo_t *		cr, 
						 const SwfdecColorTransform *trans,
						 const SwfdecRect *	inval,
						 gboolean		fill);
void		swfdec_movie_goto		(SwfdecMovie *		movie,
						 guint			frame);
gboolean      	swfdec_movie_queue_script	(SwfdecMovie *		movie,
  						 SwfdecEventType	condition);
int		swfdec_movie_compare_depths	(gconstpointer		a,
						 gconstpointer		b);

G_END_DECLS
#endif
