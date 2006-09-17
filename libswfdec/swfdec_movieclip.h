
#ifndef _SWFDEC_MOVIE_CLIP_H_
#define _SWFDEC_MOVIE_CLIP_H_

#include <swfdec_types.h>
#include <swfdec_object.h>
#include <color.h>
#include <swfdec_button.h>

G_BEGIN_DECLS

typedef struct _SwfdecDisplayList SwfdecDisplayList;
//typedef struct _SwfdecMovieClip SwfdecMovieClip;
typedef struct _SwfdecMovieClipClass SwfdecMovieClipClass;

struct _SwfdecDisplayList {
  GList *list;
};

#define SWFDEC_TYPE_MOVIE_CLIP                    (swfdec_movie_clip_get_type())
#define SWFDEC_IS_MOVIE_CLIP(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_MOVIE_CLIP))
#define SWFDEC_IS_MOVIE_CLIP_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_MOVIE_CLIP))
#define SWFDEC_MOVIE_CLIP(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_MOVIE_CLIP, SwfdecMovieClip))
#define SWFDEC_MOVIE_CLIP_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_MOVIE_CLIP, SwfdecMovieClipClass))

struct _SwfdecMovieClip
{
  SwfdecObject		object;

  SwfdecObject *	child;			/* object that we display (may be NULL) */
  SwfdecDisplayList	list;			/* our contained objects */

  /* parenting information */
  SwfdecMovieClip *	parent;			/* the object that contains us */
  char *		name;			/* the name that this clip is referenced in slash-notation */
  int			depth;			/* depth in parent's display list */
  int			clip_depth;	      	/* clipping depth (determines visibility) */

  /* positioning - the values are applied in this order */
  int			xscale;			/* x scaling in percent */
  int			yscale;			/* y scaling in percent */
  int			x;			/* x offset in pixels */
  int			y;	      		/* y offset in pixels */
  int			rotation;     		/* rotation in degrees */
  cairo_matrix_t	transform;		/* the transformation matrix computed from above */
  cairo_matrix_t	inverse_transform;	/* the inverse of the transformation matrix */
  int			width;			/* width of movie clip in pixels */
  int			height;		      	/* height of movie clip in pixels */

  /* frame information */
  int			ratio;			/* for morph shapes (FIXME: is this the same as current frame?) */
  int			current_frame;		/* the frame that is currently displayed */
  int			next_frame;		/* the frame that will be displayed next */
  gboolean		stopped;		/* if we currently iterate */
  gboolean		visible;		/* whether we currently can be seen or iterate */

  /* mouse handling */
  SwfdecMovieClip *	mouse_grab;		/* the child movie that has grabbed the mouse or self */
  double		mouse_x;		/* x position of mouse */
  double		mouse_y;		/* y position of mouse */
  gboolean		mouse_in;		/* if the mouse is inside this widget */
  int			mouse_button;		/* 1 if button is pressed, 0 otherwise */

  /* color information */
  swf_color		bg_color;		/* the background color for this movie (only used in root movie) */
  SwfdecColorTransform	color_transform;	/* color transform used in this movie */

  /* leftover unimplemented variables from the Actionscript spec */
  int alpha;
  //droptarget
  char *target;
  char *url;

  /* for buton handling (FIXME: want a subclass?) */
  SwfdecButtonState	button_state;		/* state of the displayed button (UP/OVER/DOWN) */
};

struct _SwfdecMovieClipClass
{
  SwfdecObjectClass object_class;
};

GType		swfdec_movie_clip_get_type		(void);

SwfdecMovieClip *swfdec_movie_clip_new			(SwfdecMovieClip *	parent,
							 unsigned int		id);
unsigned int	swfdec_movie_clip_get_n_frames		(SwfdecMovieClip *      movie);

void		swfdec_movie_clip_update_visuals	(SwfdecMovieClip *	movie);
void		swfdec_movie_clip_iterate		(SwfdecMovieClip *	movie);
gboolean      	swfdec_movie_clip_handle_mouse		(SwfdecMovieClip *	movie,
							 double			x,
							 double			y,
							 int			button);


G_END_DECLS
#endif
