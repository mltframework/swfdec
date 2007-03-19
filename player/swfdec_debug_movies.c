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
 * License along with this library; if not, to_string to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <libswfdec/swfdec_debugger.h>
#include <libswfdec/swfdec_movie.h>
#include <libswfdec/swfdec_player_internal.h>
#include <libswfdec/swfdec_root_movie.h>
#include "swfdec_debug_movies.h"

/*** GTK_TREE_MODEL ***/

#if 0
#  define REPORT g_print ("%s\n", G_STRFUNC)
#else
#  define REPORT 
#endif
static GtkTreeModelFlags 
swfdec_debug_movies_get_flags (GtkTreeModel *tree_model)
{
  REPORT;
  return 0;
}

static gint
swfdec_debug_movies_get_n_columns (GtkTreeModel *tree_model)
{
  REPORT;
  return SWFDEC_DEBUG_MOVIES_N_COLUMNS;
}

static GType
swfdec_debug_movies_get_column_type (GtkTreeModel *tree_model, gint index_)
{
  REPORT;
  switch (index_) {
    case SWFDEC_DEBUG_MOVIES_COLUMN_MOVIE:
      return G_TYPE_POINTER;
    case SWFDEC_DEBUG_MOVIES_COLUMN_NAME:
      return G_TYPE_STRING;
    case SWFDEC_DEBUG_MOVIES_COLUMN_VISIBLE:
      return G_TYPE_BOOLEAN;
    case SWFDEC_DEBUG_MOVIES_COLUMN_TYPE:
      return G_TYPE_STRING;
    default:
      break;
  }
  g_assert_not_reached ();
  return G_TYPE_NONE;
}

static gboolean
swfdec_debug_movies_get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
  SwfdecDebugMovies *movies = SWFDEC_DEBUG_MOVIES (tree_model);
  guint depth;
  int *indices;
  GList *walk;
  SwfdecMovie *movie;

  REPORT;
  depth = gtk_tree_path_get_depth (path);
  indices = gtk_tree_path_get_indices (path);
  if (indices == NULL)
    return FALSE;
  walk = g_list_nth (movies->player->roots, *indices);
  if (!walk)
    return FALSE;
  movie = walk->data;
  indices++;
  depth--;
  for (; depth > 0; depth--) {
    walk = g_list_nth (movie->list, *indices);
    if (!walk)
      return FALSE;
    movie = walk->data;
    indices++;
  }
  iter->user_data = movie;
  return TRUE;
}

gint
my_g_list_is_nth (GList *list, gpointer data)
{
  gint count;

  count = 0;
  for (; list; list = list->next) {
    if (list->data == data)
      return count;
    count++;
  }
  return -1;
}

static GtkTreePath *
swfdec_debug_movies_movie_to_path (SwfdecMovie *movie)
{
  GtkTreePath *path;
  gint i;

  if (movie->parent) {
    i = my_g_list_is_nth (movie->parent->list, movie);
    g_assert (i >= 0);
    path = swfdec_debug_movies_movie_to_path (movie->parent);
    gtk_tree_path_append_index (path, i);
  } else {
    i = my_g_list_is_nth (SWFDEC_ROOT_MOVIE (movie)->player->roots, movie);
    g_assert (i >= 0);
    path = gtk_tree_path_new ();
    gtk_tree_path_append_index (path, i);
  }
  return path;
}

static GtkTreePath *
swfdec_debug_movies_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  REPORT;
  return swfdec_debug_movies_movie_to_path (iter->user_data);
}

static void 
swfdec_debug_movies_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter,
    gint column, GValue *value)
{
  SwfdecMovie *movie = iter->user_data;

  REPORT;
  switch (column) {
    case SWFDEC_DEBUG_MOVIES_COLUMN_MOVIE:
      g_value_init (value, G_TYPE_POINTER);
      g_value_set_pointer (value, movie);
      return;
    case SWFDEC_DEBUG_MOVIES_COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, movie->name);
      return;
    case SWFDEC_DEBUG_MOVIES_COLUMN_VISIBLE:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, movie->visible);
      return;
    case SWFDEC_DEBUG_MOVIES_COLUMN_TYPE:
      g_value_init (value, G_TYPE_STRING);
      /* big hack: we skip the "Swfdec" here */
      g_value_set_string (value, G_OBJECT_TYPE_NAME (movie) + 6);
      return;
    default:
      break;
  }
  g_assert_not_reached ();
}

static gboolean
swfdec_debug_movies_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  GList *list;
  SwfdecMovie *movie = iter->user_data;

  REPORT;
  if (movie->parent) {
    list = movie->parent->list;
  } else {
    list = SWFDEC_ROOT_MOVIE (movie)->player->roots;
  }
  list = g_list_find (list, movie);
  g_assert (list);
  list = list->next;
  if (list == NULL)
    return FALSE;
  iter->user_data = list->data;
  return TRUE;
}

static gboolean
swfdec_debug_movies_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
  GList *list;

  REPORT;
  if (parent) {
    SwfdecMovie *movie = parent->user_data;
    list = movie->list;
  } else {
    SwfdecPlayer *player = SWFDEC_DEBUG_MOVIES (tree_model)->player;
    list = player->roots;
  }
  if (list == NULL)
    return FALSE;
  iter->user_data = list->data;
  return TRUE;
}

static gboolean
swfdec_debug_movies_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  GtkTreeIter unused;

  REPORT;
  return swfdec_debug_movies_iter_children (tree_model, &unused, iter);
}

static gint
swfdec_debug_movies_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  GList *list;

  REPORT;
  if (iter) {
    SwfdecMovie *movie = iter->user_data;
    list = movie->list;
  } else {
    SwfdecPlayer *player = SWFDEC_DEBUG_MOVIES (tree_model)->player;
    list = player->roots;
  }
  return g_list_length (list);
}

static gboolean
swfdec_debug_movies_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *parent, gint n)
{
  GList *list;

  REPORT;
  if (parent) {
    SwfdecMovie *movie = parent->user_data;
    list = movie->list;
  } else {
    SwfdecPlayer *player = SWFDEC_DEBUG_MOVIES (tree_model)->player;
    list = player->roots;
  }
  list = g_list_nth (list, n);
  if (list == NULL)
    return FALSE;
  iter->user_data = list->data;
  return TRUE;
}

static gboolean
swfdec_debug_movies_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
  SwfdecMovie *movie = SWFDEC_MOVIE (child->user_data);

  REPORT;
  if (movie->parent == NULL)
    return FALSE;
  iter->user_data = movie->parent;
  return TRUE;
}

static void
swfdec_debug_movies_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = swfdec_debug_movies_get_flags;
  iface->get_n_columns = swfdec_debug_movies_get_n_columns;
  iface->get_column_type = swfdec_debug_movies_get_column_type;
  iface->get_iter = swfdec_debug_movies_get_iter;
  iface->get_path = swfdec_debug_movies_get_path;
  iface->get_value = swfdec_debug_movies_get_value;
  iface->iter_next = swfdec_debug_movies_iter_next;
  iface->iter_children = swfdec_debug_movies_iter_children;
  iface->iter_has_child = swfdec_debug_movies_iter_has_child;
  iface->iter_n_children = swfdec_debug_movies_iter_n_children;
  iface->iter_nth_child = swfdec_debug_movies_iter_nth_child;
  iface->iter_parent = swfdec_debug_movies_iter_parent;
}

/*** SWFDEC_DEBUG_MOVIES ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecDebugMovies, swfdec_debug_movies, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, swfdec_debug_movies_tree_model_init))

static void
swfdec_debug_movies_added (SwfdecPlayer *player, SwfdecMovie *movie, SwfdecDebugMovies *movies)
{
  GtkTreePath *path = swfdec_debug_movies_movie_to_path (movie);
  GtkTreeIter iter;

  iter.user_data = movie;
  gtk_tree_model_row_inserted (GTK_TREE_MODEL (movies), path, &iter);
  gtk_tree_path_free (path);
}

static void
swfdec_debug_movies_removed (SwfdecPlayer *player, SwfdecMovie *movie, SwfdecDebugMovies *movies)
{
  GList *list;
  GtkTreePath *path;
  int i = 0;

  if (movie->parent) {
    path = swfdec_debug_movies_movie_to_path (movie->parent);
    list = movie->parent->list;
  } else {
    path = gtk_tree_path_new ();
    list = player->roots;
  }
  for (;list; list = list->next) {
    if (swfdec_movie_compare_depths (list->data, movie) >= 0)
      break;
    i++;
  }
  gtk_tree_path_append_index (path, i);
  gtk_tree_model_row_deleted (GTK_TREE_MODEL (movies), path);
  gtk_tree_path_free (path);
}

static void
swfdec_debug_movies_dispose (GObject *object)
{
  SwfdecDebugMovies *movies = SWFDEC_DEBUG_MOVIES (object);

  g_signal_handlers_disconnect_by_func (movies->player, swfdec_debug_movies_removed, movies);
  g_signal_handlers_disconnect_by_func (movies->player, swfdec_debug_movies_added, movies);
  g_object_unref (movies->player);

  G_OBJECT_CLASS (swfdec_debug_movies_parent_class)->dispose (object);
}

static void
swfdec_debug_movies_class_init (SwfdecDebugMoviesClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = swfdec_debug_movies_dispose;
}

static void
swfdec_debug_movies_init (SwfdecDebugMovies *token)
{
}

SwfdecDebugMovies *
swfdec_debug_movies_new (SwfdecPlayer *player)
{
  SwfdecDebugMovies *movies;

  movies = g_object_new (SWFDEC_TYPE_DEBUG_MOVIES, NULL);
  movies->player = player;
  g_object_ref (player);
  if (SWFDEC_IS_DEBUGGER (player)) {
    g_signal_connect (player, "movie-added", G_CALLBACK (swfdec_debug_movies_added), movies);
    g_signal_connect (player, "movie-removed", G_CALLBACK (swfdec_debug_movies_removed), movies);
  }
  return movies;
}

