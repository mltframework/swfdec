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
    case SWFDEC_DEBUG_MOVIES_COLUMN_DEPTH:
      return G_TYPE_INT;
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
  GNode *node;
  guint i, depth;
  int *indices;

  REPORT;
  depth = gtk_tree_path_get_depth (path);
  indices = gtk_tree_path_get_indices (path);
  if (indices == NULL)
    return FALSE;
  node = movies->root;
  for (i = 0; i < depth; i++) {
    node = g_node_nth_child (node, indices[i]);
    if (node == NULL)
      return FALSE;
  }
  iter->user_data = node;
  iter->stamp = movies->stamp;
  return TRUE;
}

static GtkTreePath *
swfdec_debug_movies_node_to_path (GNode *node)
{
  GtkTreePath *path;

  path = gtk_tree_path_new ();
  while (node->parent != NULL) {
    gtk_tree_path_prepend_index (path, g_node_child_position (node->parent, node));
    node = node->parent;
  }
  return path;
}

static GtkTreePath *
swfdec_debug_movies_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  REPORT;
  return swfdec_debug_movies_node_to_path (iter->user_data);
}

static void 
swfdec_debug_movies_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter,
    gint column, GValue *value)
{
  SwfdecMovie *movie = ((GNode *) iter->user_data)->data;

  REPORT;
  switch (column) {
    case SWFDEC_DEBUG_MOVIES_COLUMN_MOVIE:
      g_value_init (value, G_TYPE_POINTER);
      g_value_set_pointer (value, movie);
      return;
    case SWFDEC_DEBUG_MOVIES_COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      if (movie->name[0])
	g_value_set_string (value, movie->name);
      else
	g_value_set_string (value, movie->original_name);
      return;
    case SWFDEC_DEBUG_MOVIES_COLUMN_DEPTH:
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, movie->depth);
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
  GNode *node;

  REPORT;
  node = iter->user_data;
  node = node->next;
  if (node == NULL)
    return FALSE;
  iter->user_data = node;
  return TRUE;
}

static gboolean
swfdec_debug_movies_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
  GNode *node;

  REPORT;
  if (parent) {
    node = parent->user_data;
  } else {
    node = SWFDEC_DEBUG_MOVIES (tree_model)->root;
  }
  if (node->children == NULL)
    return FALSE;
  iter->user_data = node->children;
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
  GNode *node;

  REPORT;
  if (iter) {
    node = iter->user_data;
  } else {
    node = SWFDEC_DEBUG_MOVIES (tree_model)->root;
  }
  return g_node_n_children (node);
}

static gboolean
swfdec_debug_movies_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *parent, gint n)
{
  GNode *node;

  REPORT;
  if (parent) {
    node = parent->user_data;
  } else {
    node = SWFDEC_DEBUG_MOVIES (tree_model)->root;
  }
  node = g_node_nth_child (node, n);
  if (node == NULL)
    return FALSE;
  iter->user_data = node;
  return TRUE;
}

static gboolean
swfdec_debug_movies_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
  GNode *node;

  REPORT;
  node = child->user_data;
  node = node->parent;
  if (node->parent == NULL)
    return FALSE;
  iter->user_data = node;
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

static int
swfdec_debug_movies_get_index (GNode *parent, GNode *new)
{
  GNode *walk;
  int i = 0;

  for (walk = parent->children; walk; walk = walk->next) {
    if (walk == new)
      continue;
    if (swfdec_movie_compare_depths (walk->data, new->data) < 0) {
      i++;
      continue;
    }
    break;
  }
  return i;
}

static void
swfdec_debug_movies_added (SwfdecPlayer *player, SwfdecMovie *movie, SwfdecDebugMovies *movies)
{
  GtkTreePath *path;
  GtkTreeIter iter;
  GNode *node, *new;
  int pos;

  if (movie->parent) {
    node = g_hash_table_lookup (movies->nodes, movie->parent);
    g_assert (node);
  } else {
    node = movies->root;
  }
  new = g_node_new (movie);
  g_hash_table_insert (movies->nodes, movie, new);
  pos = swfdec_debug_movies_get_index (node, new);
  g_node_insert (node, pos, new);
  movies->stamp++;
  iter.stamp = movies->stamp;
  iter.user_data = new;
  path = swfdec_debug_movies_node_to_path (new);
  gtk_tree_model_row_inserted (GTK_TREE_MODEL (movies), path, &iter);
  gtk_tree_path_free (path);
}

static void
swfdec_debug_movies_movie_notify (SwfdecMovie *movie, GParamSpec *pspec, SwfdecDebugMovies *movies)
{
  GtkTreeIter iter;
  GtkTreePath *path;
  GNode *node;

  node = g_hash_table_lookup (movies->nodes, movie);
  if (g_str_equal (pspec->name, "depth")) {
    /* reorder when depth changes */
    g_printerr ("FIXME: implement depth changes\n");
  }
  iter.stamp = movies->stamp;
  iter.user_data = node;
  path = swfdec_debug_movies_node_to_path (node);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (movies), path, &iter);
  gtk_tree_path_free (path);
}

static void
swfdec_debug_movies_removed (SwfdecPlayer *player, SwfdecMovie *movie, SwfdecDebugMovies *movies)
{
  GNode *node;
  GtkTreePath *path;

  node = g_hash_table_lookup (movies->nodes, movie);
  g_hash_table_remove (movies->nodes, movie);
  g_signal_handlers_disconnect_by_func (movie, swfdec_debug_movies_movie_notify, movies);
  path = swfdec_debug_movies_node_to_path (node);
  g_assert (g_node_n_children (node) == 0);
  g_node_destroy (node);
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
  g_assert (g_node_n_children (movies->root) == 0);
  g_node_destroy (movies->root);
  g_hash_table_destroy (movies->nodes);

  G_OBJECT_CLASS (swfdec_debug_movies_parent_class)->dispose (object);
}

static void
swfdec_debug_movies_class_init (SwfdecDebugMoviesClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = swfdec_debug_movies_dispose;
}

static void
swfdec_debug_movies_init (SwfdecDebugMovies *movies)
{
  movies->root = g_node_new (NULL);
  movies->nodes = g_hash_table_new (g_direct_hash, g_direct_equal);
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

