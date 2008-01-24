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
#include <swfdec/swfdec_movie.h>
#include <swfdec/swfdec_player_internal.h>
#include "vivi_movie_list.h"

/*** GTK_TREE_MODEL ***/

#if 0
#  define REPORT g_print ("%s\n", G_STRFUNC)
#else
#  define REPORT 
#endif
static GtkTreeModelFlags 
vivi_movie_list_get_flags (GtkTreeModel *tree_model)
{
  REPORT;
  return 0;
}

static gint
vivi_movie_list_get_n_columns (GtkTreeModel *tree_model)
{
  REPORT;
  return VIVI_MOVIE_LIST_N_COLUMNS;
}

static GType
vivi_movie_list_get_column_type (GtkTreeModel *tree_model, gint index_)
{
  REPORT;
  switch (index_) {
    case VIVI_MOVIE_LIST_COLUMN_MOVIE:
      return G_TYPE_POINTER;
    case VIVI_MOVIE_LIST_COLUMN_NAME:
      return G_TYPE_STRING;
    case VIVI_MOVIE_LIST_COLUMN_DEPTH:
      return G_TYPE_INT;
    case VIVI_MOVIE_LIST_COLUMN_TYPE:
      return G_TYPE_STRING;
    default:
      break;
  }
  g_assert_not_reached ();
  return G_TYPE_NONE;
}

static gboolean
vivi_movie_list_get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
  ViviMovieList *movies = VIVI_MOVIE_LIST (tree_model);
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
vivi_movie_list_node_to_path (GNode *node)
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
vivi_movie_list_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  REPORT;
  return vivi_movie_list_node_to_path (iter->user_data);
}

static void 
vivi_movie_list_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter,
    gint column, GValue *value)
{
  SwfdecMovie *movie = ((GNode *) iter->user_data)->data;

  REPORT;
  switch (column) {
    case VIVI_MOVIE_LIST_COLUMN_MOVIE:
      g_value_init (value, G_TYPE_POINTER);
      g_value_set_pointer (value, movie);
      return;
    case VIVI_MOVIE_LIST_COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      if (movie->name[0])
	g_value_set_string (value, movie->name);
      else
	g_value_set_string (value, movie->original_name);
      return;
    case VIVI_MOVIE_LIST_COLUMN_DEPTH:
      g_value_init (value, G_TYPE_INT);
      g_value_set_int (value, movie->depth);
      return;
    case VIVI_MOVIE_LIST_COLUMN_TYPE:
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
vivi_movie_list_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
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
vivi_movie_list_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
  GNode *node;

  REPORT;
  if (parent) {
    node = parent->user_data;
  } else {
    node = VIVI_MOVIE_LIST (tree_model)->root;
  }
  if (node->children == NULL)
    return FALSE;
  iter->user_data = node->children;
  return TRUE;
}

static gboolean
vivi_movie_list_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  GtkTreeIter unused;

  REPORT;
  return vivi_movie_list_iter_children (tree_model, &unused, iter);
}

static gint
vivi_movie_list_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  GNode *node;

  REPORT;
  if (iter) {
    node = iter->user_data;
  } else {
    node = VIVI_MOVIE_LIST (tree_model)->root;
  }
  return g_node_n_children (node);
}

static gboolean
vivi_movie_list_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *parent, gint n)
{
  GNode *node;

  REPORT;
  if (parent) {
    node = parent->user_data;
  } else {
    node = VIVI_MOVIE_LIST (tree_model)->root;
  }
  node = g_node_nth_child (node, n);
  if (node == NULL)
    return FALSE;
  iter->user_data = node;
  return TRUE;
}

static gboolean
vivi_movie_list_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
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
vivi_movie_list_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = vivi_movie_list_get_flags;
  iface->get_n_columns = vivi_movie_list_get_n_columns;
  iface->get_column_type = vivi_movie_list_get_column_type;
  iface->get_iter = vivi_movie_list_get_iter;
  iface->get_path = vivi_movie_list_get_path;
  iface->get_value = vivi_movie_list_get_value;
  iface->iter_next = vivi_movie_list_iter_next;
  iface->iter_children = vivi_movie_list_iter_children;
  iface->iter_has_child = vivi_movie_list_iter_has_child;
  iface->iter_n_children = vivi_movie_list_iter_n_children;
  iface->iter_nth_child = vivi_movie_list_iter_nth_child;
  iface->iter_parent = vivi_movie_list_iter_parent;
}

/*** VIVI_MOVIE_LIST ***/

G_DEFINE_TYPE_WITH_CODE (ViviMovieList, vivi_movie_list, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, vivi_movie_list_tree_model_init))

static int
vivi_movie_list_get_index (GNode *parent, GNode *new)
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
vivi_movie_list_movie_notify (SwfdecMovie *movie, GParamSpec *pspec, ViviMovieList *movies)
{
  GtkTreeIter iter;
  GtkTreePath *path;
  GNode *node;

  node = g_hash_table_lookup (movies->nodes, movie);
  if (g_str_equal (pspec->name, "depth")) {
    guint old, new;
    GNode *parent;

    parent = node->parent;
    old = g_node_child_position (parent, node);
    new = vivi_movie_list_get_index (parent, node);
    if (old != new) {
      /* reorder */
      guint min = MIN (old, new);
      guint max = MAX (old, new);
      guint i;
      guint count = g_node_n_children (parent);
      int *positions = g_new (int, count);
      for (i = 0; i < min; i++) {
	positions[i] = i;
      }
      if (old < new) {
	for (i = min; i < max; i++) {
	  positions[i] = i + 1;
	}
      } else {
	for (i = min + 1; i <= max; i++) {
	  positions[i] = i - 1;
	}
      }
      positions[new] = old;
      for (i = max + 1; i < count; i++) {
	positions[i] = i;
      }
      g_node_unlink (node);
      g_node_insert (parent, new, node);
      iter.stamp = movies->stamp;
      iter.user_data = parent;
      path = vivi_movie_list_node_to_path (parent);
      gtk_tree_model_rows_reordered (GTK_TREE_MODEL (movies), path, &iter, positions);
      gtk_tree_path_free (path);
      g_free (positions);
    }
  }
  iter.stamp = movies->stamp;
  iter.user_data = node;
  path = vivi_movie_list_node_to_path (node);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (movies), path, &iter);
  gtk_tree_path_free (path);
}

static gboolean
vivi_movie_list_added (ViviDebugger *debugger, SwfdecAsObject *object, ViviMovieList *movies)
{
  SwfdecMovie *movie;
  GtkTreePath *path;
  GtkTreeIter iter;
  GNode *node, *new;
  int pos;

  if (!SWFDEC_IS_MOVIE (object))
    return FALSE;
  movie = SWFDEC_MOVIE (object);
  g_signal_connect (movie, "notify", G_CALLBACK (vivi_movie_list_movie_notify), movies);
  if (movie->parent) {
    node = g_hash_table_lookup (movies->nodes, movie->parent);
    g_assert (node);
  } else {
    node = movies->root;
  }
  new = g_node_new (movie);
  g_hash_table_insert (movies->nodes, movie, new);
  pos = vivi_movie_list_get_index (node, new);
  g_node_insert (node, pos, new);
  movies->stamp++;
  iter.stamp = movies->stamp;
  iter.user_data = new;
  path = vivi_movie_list_node_to_path (new);
  gtk_tree_model_row_inserted (GTK_TREE_MODEL (movies), path, &iter);
  gtk_tree_path_free (path);
  return FALSE;
}

static void
vivi_movie_list_remove_node (ViviMovieList *movies, GNode *node)
{
  GNode *walk;

  for (walk = node->children; walk; walk = walk->next) {
    vivi_movie_list_remove_node (movies, walk);
  }
  g_hash_table_remove (movies->nodes, node->data);
  g_signal_handlers_disconnect_by_func (node->data, vivi_movie_list_movie_notify, movies);
}

static gboolean
vivi_movie_list_removed (ViviDebugger *debugger, SwfdecAsObject *object, ViviMovieList *movies)
{
  GNode *node;
  GtkTreePath *path;

  if (!SWFDEC_IS_MOVIE (object))
    return FALSE;
  node = g_hash_table_lookup (movies->nodes, object);
  /* happens when parent was already removed */
  if (node == NULL)
    return FALSE;
  vivi_movie_list_remove_node (movies, node);
  path = vivi_movie_list_node_to_path (node);
  g_node_destroy (node);
  gtk_tree_model_row_deleted (GTK_TREE_MODEL (movies), path);
  gtk_tree_path_free (path);
  return FALSE;
}

static void
vivi_movie_list_reset (ViviApplication *app, GParamSpec *pspec, ViviMovieList *movies)
{
  GNode *walk;

  for (walk = movies->root->children; walk; walk = walk->next) {
    vivi_movie_list_removed (NULL, walk->data, movies);
  }
}

static void
vivi_movie_list_dispose (GObject *object)
{
  ViviMovieList *movies = VIVI_MOVIE_LIST (object);
  ViviDebugger *debugger;
  GNode *walk;

  debugger = movies->app->debugger;
  g_signal_handlers_disconnect_by_func (movies->app, vivi_movie_list_reset, movies);
  g_signal_handlers_disconnect_by_func (debugger, vivi_movie_list_removed, movies);
  g_signal_handlers_disconnect_by_func (debugger, vivi_movie_list_added, movies);
  g_object_unref (movies->app);
  for (walk = movies->root->children; walk; walk = walk->next) {
    vivi_movie_list_remove_node (movies, walk);
  }
  g_node_destroy (movies->root);
#ifndef G_DISABLE_ASSERT
  if (g_hash_table_size (movies->nodes) != 0) {
    g_error ("%u items left in hash table", g_hash_table_size (movies->nodes));
  }
#endif
  g_hash_table_destroy (movies->nodes);

  G_OBJECT_CLASS (vivi_movie_list_parent_class)->dispose (object);
}

static void
vivi_movie_list_class_init (ViviMovieListClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = vivi_movie_list_dispose;
}

static void
vivi_movie_list_init (ViviMovieList *movies)
{
  movies->root = g_node_new (NULL);
  movies->nodes = g_hash_table_new (g_direct_hash, g_direct_equal);
}

GtkTreeModel *
vivi_movie_list_new (ViviApplication *app)
{
  ViviMovieList *movies;
  ViviDebugger *debugger;

  movies = g_object_new (VIVI_TYPE_MOVIE_LIST, NULL);
  g_object_ref (app);
  movies->app = app;
  debugger = app->debugger;
  g_signal_connect (app, "notify::player", G_CALLBACK (vivi_movie_list_reset), movies);
  g_signal_connect (debugger, "add", G_CALLBACK (vivi_movie_list_added), movies);
  g_signal_connect (debugger, "remove", G_CALLBACK (vivi_movie_list_removed), movies);
  return GTK_TREE_MODEL (movies);
}

