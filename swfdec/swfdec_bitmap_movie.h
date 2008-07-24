/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_BITMAP_MOVIE_H_
#define _SWFDEC_BITMAP_MOVIE_H_

#include <glib-object.h>
#include <swfdec/swfdec_bitmap_data.h>
#include <swfdec/swfdec_movie.h>

G_BEGIN_DECLS


typedef struct _SwfdecBitmapMovie SwfdecBitmapMovie;
typedef struct _SwfdecBitmapMovieClass SwfdecBitmapMovieClass;

#define SWFDEC_TYPE_BITMAP_MOVIE                    (swfdec_bitmap_movie_get_type())
#define SWFDEC_IS_BITMAP_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_BITMAP_MOVIE))
#define SWFDEC_IS_BITMAP_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_BITMAP_MOVIE))
#define SWFDEC_BITMAP_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_BITMAP_MOVIE, SwfdecBitmapMovie))
#define SWFDEC_BITMAP_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_BITMAP_MOVIE, SwfdecBitmapMovieClass))

struct _SwfdecBitmapMovie {
  SwfdecMovie		movie;

  SwfdecBitmapData *	bitmap;		/* the bitmap we are attached to */
};

struct _SwfdecBitmapMovieClass {
  SwfdecMovieClass	movie_class;
};

GType		swfdec_bitmap_movie_get_type		(void);

SwfdecMovie *	swfdec_bitmap_movie_new			(SwfdecMovie *		parent,
							 SwfdecBitmapData *	bitmap,
							 int			depth);


G_END_DECLS
#endif
