
#ifndef _SWFDEC_MOVIE_CLIP_H_
#define _SWFDEC_MOVIE_CLIP_H_

#include <swfdec_types.h>
#include <swfdec_object.h>
#include <color.h>
#include <swfdec_button.h>
#include <swfdec_edittext.h>

G_BEGIN_DECLS

typedef struct _SwfdecDisplayList SwfdecDisplayList;
//typedef struct _SwfdecMovieClip SwfdecMovieClip;
typedef struct _SwfdecMovieClipClass SwfdecMovieClipClass;

#define SWFDEC_TYPE_MOVIE_CLIP                    (swfdec_movie_clip_get_type())
#define SWFDEC_IS_MOVIE_CLIP(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_MOVIE_CLIP))
#define SWFDEC_IS_MOVIE_CLIP_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_MOVIE_CLIP))
#define SWFDEC_MOVIE_CLIP(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_MOVIE_CLIP, SwfdecMovieClip))
#define SWFDEC_MOVIE_CLIP_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_MOVIE_CLIP, SwfdecMovieClipClass))

typedef enum {
  SWFDEC_MOVIE_CLIP_UP_TO_DATE = 0,
  SWFDEC_MOVIE_CLIP_INVALID_CHILDREN,
  SWFDEC_MOVIE_CLIP_INVALID_EXTENTS,
  SWFDEC_MOVIE_CLIP_INVALID_AREA,
  SWFDEC_MOVIE_CLIP_INVALID_MATRIX,
} SwfdecMovieClipState;

struct _SwfdecMovieClip
{
  SwfdecObject		object;

  JSObject *		jsobj;			/* our object in javascript */
  SwfdecObject *	child;			/* object that we display (may be NULL) */
  GList *		list;			/* our contained movie clips (order by depth) */
  const SwfdecSpriteContent *	content;      	/* the content we are displaying */
  SwfdecMovieClipState	cache_state;		/* wether we are up to date */

  /* parenting information */
  SwfdecMovieClip *	parent;			/* the object that contains us */

  /* positioning - the values are applied in this order */
  SwfdecRect		original_extents;	/* the extents from all the children */
  double      		x;			/* x offset in twips */
  double		y;	      		/* y offset in twips */
  double      		xscale;			/* x scale factor */
  double      		yscale;			/* y scale factor */
  int			rotation;     		/* rotation in degrees */
  cairo_matrix_t	transform;		/* transformation matrix computed from above */
  cairo_matrix_t	inverse_transform;	/* the inverse of the transformation matrix */

  /* frame information */
  unsigned int		current_frame;		/* frame that is currently displayed (NB: indexed from 0) */
  unsigned int	      	next_frame;		/* frame that will be displayed next, the current frame to JS */
  gboolean		stopped;		/* if we currently iterate */
  gboolean		visible;		/* whether we currently can be seen or iterate */

  /* mouse handling */
  SwfdecMovieClip *	mouse_grab;		/* the child movie that has grabbed the mouse or self */
  double		mouse_x;		/* x position of mouse */
  double		mouse_y;		/* y position of mouse */
  gboolean		mouse_in;		/* if the mouse is inside this widget */
  int			mouse_button;		/* 1 if button is pressed, 0 otherwise */

  /* color information */
  swf_color		bg_color;		/* background color of main sprite */

  /* audio stream handling */
  unsigned int		sound_frame;		/* last frame we updated for */
  guint			sound_stream;		/* stream that currently plays */

  /* leftover unimplemented variables from the Actionscript spec */
  int alpha;
  //droptarget
  char *target;
  char *url;

  /* for buton handling (FIXME: want a subclass?) */
  SwfdecButtonState	button_state;		/* state of the displayed button (UP/OVER/DOWN) */

  /* for EditText handling */
  GHashTable *		text_variables;		/* all the text variables (by name) */
  char *		text;			/* current text for this textfield */
  SwfdecParagraph *	text_paragraph;		/* how that current text is rendered */
};

struct _SwfdecMovieClipClass
{
  SwfdecObjectClass object_class;
};

GType		swfdec_movie_clip_get_type		(void);

SwfdecMovieClip *swfdec_movie_clip_new			(SwfdecMovieClip *	parent,
							 SwfdecSpriteContent *	content);
void		swfdec_movie_clip_remove_root		(SwfdecMovieClip *	root);
unsigned int	swfdec_movie_clip_get_n_frames		(SwfdecMovieClip *      movie);
void		swfdec_movie_clip_set_child		(SwfdecMovieClip *	movie,
							 SwfdecObject *		child);
void		swfdec_movie_clip_set_content		(SwfdecMovieClip *	movie,
							 const SwfdecSpriteContent *content);
void		swfdec_movie_clip_set_text		(SwfdecMovieClip *	clip,
							 const char *		text);

void		swfdec_movie_clip_queue_update		(SwfdecMovieClip *	movie,
							 SwfdecMovieClipState	state);
void		swfdec_movie_clip_update		(SwfdecMovieClip *	movie);
void		swfdec_movie_clip_queue_iterate		(SwfdecMovieClip *	movie);
void		swfdec_movie_clip_iterate_audio		(SwfdecMovieClip *	movie);
void		swfdec_movie_clip_goto			(SwfdecMovieClip *	movie,
							 guint			frame,
							 gboolean		do_enter_frame);
gboolean	swfdec_decoder_do_goto			(SwfdecDecoder *	dec);
gboolean      	swfdec_movie_clip_handle_mouse		(SwfdecMovieClip *	movie,
							 double			x,
							 double			y,
							 int			button);


G_END_DECLS
#endif
