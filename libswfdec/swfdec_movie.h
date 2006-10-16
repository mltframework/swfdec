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
#include <libswfdec/color.h>
#include <libswfdec/swfdec_event.h>
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

  JSObject *		jsobj;			/* our object in javascript */
  GList *		list;			/* our contained movie clips (ordered by depth) */
  const SwfdecContent *	content;           	/* the content we are displaying */
  SwfdecMovieState	cache_state;		/* wether we are up to date */

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

  /* iteration state */
  guint			frame;			/* current frame */
  guint			n_frames;		/* amount of frames */
  gboolean		stopped;		/* if we currently iterate */
  gboolean		visible;		/* whether we currently can be seen or iterate */

  /* mouse handling */
  SwfdecMovie *		mouse_grab;		/* child movie or self when mouse is grabbed */
  double		mouse_x;		/* x position of mouse */
  double		mouse_y;		/* y position of mouse */
  gboolean		mouse_in;		/* if the mouse is inside this widget */
  int			mouse_button;		/* 1 if button is pressed, 0 otherwise */

  /* leftover unimplemented variables from the Actionscript spec */
  int alpha;
  //droptarget
  char *target;
  char *url;
};

struct _SwfdecMovieClass {
  GObjectClass		object_class;

  /* vfuncs */
  void			(* set_parent)		(SwfdecMovie *		movie,
						 SwfdecMovie *		parent);
  void			(* content_changed)	(SwfdecMovie *		movie,
						 const SwfdecContent *	content);
  void			(* update_extents)	(SwfdecMovie *		movie,
						 SwfdecRect *   	extents);
  void			(* render)		(SwfdecMovie *		movie, 
						 cairo_t *		cr,
						 const SwfdecColorTransform *trans,
						 const SwfdecRect *	inval,
						 gboolean		fill);
  gboolean		(* handle_mouse)      	(SwfdecMovie *		movie,
						 double			x,
						 double			y,
						 int			button);
  void			(* goto_frame)		(SwfdecMovie *		movie,
						 guint			frame);
};

GType		swfdec_movie_get_type		(void);

SwfdecMovie *	swfdec_movie_new		(SwfdecMovie *		parent,
						 const SwfdecContent *	content);
void		swfdec_movie_set_parent		(SwfdecMovie *		movie,
						 SwfdecMovie *		parent);
void		swfdec_movie_remove		(SwfdecMovie *		movie);
void		swfdec_movie_set_content	(SwfdecMovie *		movie,
						 const SwfdecContent *	content);
void		swfdec_movie_invalidate		(SwfdecMovie *		movie);
void		swfdec_movie_queue_update	(SwfdecMovie *		movie,
						 SwfdecMovieState	state);
void		swfdec_movie_update		(SwfdecMovie *		movie);
gboolean      	swfdec_movie_handle_mouse	(SwfdecMovie *		movie,
						 double			x,
						 double			y,
						 int			button);
void		swfdec_movie_render		(SwfdecMovie *		movie,
						 cairo_t *		cr, 
						 const SwfdecColorTransform *trans,
						 const SwfdecRect *	inval,
						 gboolean		fill);
void		swfdec_movie_goto		(SwfdecMovie *		movie,
						 guint			frame);
void		swfdec_movie_execute		(SwfdecMovie *		movie,
						 SwfdecEventType	condition);
int		swfdec_movie_compare_depths	(gconstpointer		a,
						 gconstpointer		b);

G_END_DECLS
#endif
