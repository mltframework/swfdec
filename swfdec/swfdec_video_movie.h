/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_VIDEO_MOVIE_H_
#define _SWFDEC_VIDEO_MOVIE_H_

#include <glib-object.h>
#include <swfdec/swfdec_movie.h>
#include <swfdec/swfdec_video.h>

G_BEGIN_DECLS


typedef struct _SwfdecVideoMovie SwfdecVideoMovie;
typedef struct _SwfdecVideoMovieClass SwfdecVideoMovieClass;
typedef struct _SwfdecVideoMovieInput SwfdecVideoMovieInput;

#define SWFDEC_TYPE_VIDEO_MOVIE                    (swfdec_video_movie_get_type())
#define SWFDEC_IS_VIDEO_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_VIDEO_MOVIE))
#define SWFDEC_IS_VIDEO_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_VIDEO_MOVIE))
#define SWFDEC_VIDEO_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_VIDEO_MOVIE, SwfdecVideoMovie))
#define SWFDEC_VIDEO_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_VIDEO_MOVIE, SwfdecVideoMovieClass))

/* FIXME: make an interface? */
struct _SwfdecVideoMovieInput {
  /* connect to movie */
  void			(* connect)	(SwfdecVideoMovieInput *input,
					 SwfdecVideoMovie *	movie);
  /* called when input is unset */
  void			(* disconnect)	(SwfdecVideoMovieInput *input,
					 SwfdecVideoMovie *	movie);
  /* called when movie ratio changed */
  void			(* set_ratio)	(SwfdecVideoMovieInput *input,
					 SwfdecVideoMovie *	movie);
  /* called to request the current image */
  cairo_surface_t *	(* get_image)	(SwfdecVideoMovieInput *input);
};

struct _SwfdecVideoMovie {
  SwfdecMovie		movie;

  SwfdecVideo *		video;		/* video we play back */
  SwfdecVideoMovieInput *input;		/* where we take the input from */
  gboolean		needs_update;	/* TRUE if we should call set_ratio and get_image */
  cairo_surface_t *	image;	 	/* currently displayed image */
};

struct _SwfdecVideoMovieClass {
  SwfdecMovieClass	movie_class;
};

GType		swfdec_video_movie_get_type		(void);

void		swfdec_video_movie_set_input		(SwfdecVideoMovie *	movie,
							 SwfdecVideoMovieInput *input);
void		swfdec_video_movie_clear	      	(SwfdecVideoMovie *	movie);
/* API for SwfdecVideoMovieInput */
void		swfdec_video_movie_new_image		(SwfdecVideoMovie *	movie);

G_END_DECLS
#endif
