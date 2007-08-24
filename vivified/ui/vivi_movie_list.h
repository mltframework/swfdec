/* Vivi
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

#include <vivified/core/vivified-core.h>

#ifndef _VIVI_MOVIE_LIST_H_
#define _VIVI_MOVIE_LIST_H_

G_BEGIN_DECLS

enum {
  VIVI_MOVIE_LIST_COLUMN_MOVIE,
  VIVI_MOVIE_LIST_COLUMN_NAME,
  VIVI_MOVIE_LIST_COLUMN_DEPTH,
  VIVI_MOVIE_LIST_COLUMN_TYPE,
  /* add more */
  VIVI_MOVIE_LIST_N_COLUMNS
};

typedef struct _ViviMovieList ViviMovieList;
typedef struct _ViviMovieListClass ViviMovieListClass;

#define VIVI_TYPE_MOVIE_LIST                    (vivi_movie_list_get_type())
#define VIVI_IS_MOVIE_LIST(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_MOVIE_LIST))
#define VIVI_IS_MOVIE_LIST_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_MOVIE_LIST))
#define VIVI_MOVIE_LIST(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_MOVIE_LIST, ViviMovieList))
#define VIVI_MOVIE_LIST_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_MOVIE_LIST, ViviMovieListClass))

struct _ViviMovieList
{
  GObject		object;

  ViviApplication *	app;		/* the application we watch */
  GNode *		root;		/* the root node containing all the movies */
  int			stamp;		/* to validate tree iters */
  GHashTable *		nodes;		/* movies => node fast lookup table */
};

struct _ViviMovieListClass
{
  GObjectClass		object_class;
};

GType		vivi_movie_list_get_type		(void);

GtkTreeModel *	vivi_movie_list_new			(ViviApplication *	app);


G_END_DECLS
#endif
