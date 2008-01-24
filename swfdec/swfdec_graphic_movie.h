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

#ifndef _SWFDEC_GRAPHIC_MOVIE_H_
#define _SWFDEC_GRAPHIC_MOVIE_H_

#include <glib-object.h>
#include <swfdec/swfdec_movie.h>
#include <swfdec/swfdec_graphic.h>

G_BEGIN_DECLS


typedef struct _SwfdecGraphicMovie SwfdecGraphicMovie;
typedef struct _SwfdecGraphicMovieClass SwfdecGraphicMovieClass;

#define SWFDEC_TYPE_GRAPHIC_MOVIE                    (swfdec_graphic_movie_get_type())
#define SWFDEC_IS_GRAPHIC_MOVIE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_GRAPHIC_MOVIE))
#define SWFDEC_IS_GRAPHIC_MOVIE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_GRAPHIC_MOVIE))
#define SWFDEC_GRAPHIC_MOVIE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_GRAPHIC_MOVIE, SwfdecGraphicMovie))
#define SWFDEC_GRAPHIC_MOVIE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_GRAPHIC_MOVIE, SwfdecGraphicMovieClass))

struct _SwfdecGraphicMovie {
  SwfdecMovie		movie;
};

struct _SwfdecGraphicMovieClass {
  SwfdecMovieClass	movie_class;
};

GType		swfdec_graphic_movie_get_type		(void);


G_END_DECLS
#endif
