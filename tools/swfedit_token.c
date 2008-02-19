/* Swfedit
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

#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <swfdec/swfdec_buffer.h>
#include <swfdec/swfdec_color.h>
#include <swfdec/swfdec_script_internal.h>
#include "swfedit_token.h"

/*** CONVERTERS ***/

static gboolean
swfedit_parse_hex (const char *s, guint *result)
{
  guint byte;

  if (s[0] >= '0' && s[0] <= '9')
    byte = s[0] - '0';
  else if (s[0] >= 'a' && s[0] <= 'f')
    byte = s[0] + 10 - 'a';
  else if (s[0] >= 'A' && s[0] <= 'F')
    byte = s[0] + 10 - 'A';
  else
    return FALSE;
  s++;
  byte *= 16;
  if (s[0] >= '0' && s[0] <= '9')
    byte += s[0] - '0';
  else if (s[0] >= 'a' && s[0] <= 'f')
    byte += s[0] + 10 - 'a';
  else if (s[0] >= 'A' && s[0] <= 'F')
    byte += s[0] + 10 - 'A';
  else
    return FALSE;
  *result = byte;
  return TRUE;
}

static gpointer
swfedit_binary_new (void)
{
  return swfdec_buffer_new (0);
}

static gboolean
swfedit_binary_from_string (const char *s, gpointer* result)
{
  GByteArray *array = g_byte_array_new ();
  guint byte;
  guint8 add;

  while (g_ascii_isspace (*s)) s++;
  do {
    if (!swfedit_parse_hex (s, &byte))
      break;
    s += 2;
    add = byte;
    g_byte_array_append (array, &add, 1);
    while (g_ascii_isspace (*s)) s++;
  } while (*s != '\0');
  if (*s == '\0') {
    SwfdecBuffer *buffer = swfdec_buffer_new_for_data (array->data, array->len);
    g_byte_array_free (array, FALSE);
    *result = buffer;
    return TRUE;
  }
  g_byte_array_free (array, TRUE);
  return FALSE;
}

static char *
swfedit_binary_to_string (gconstpointer value)
{
  guint i;
  const SwfdecBuffer *buffer = value;
  GString *string = g_string_new ("");

  for (i = 0; i < buffer->length; i++) {
    if (i && i % 4 == 0)
      g_string_append_c (string, ' ');
    g_string_append_printf (string, "%02X", buffer->data[i]);
  }
  return g_string_free (string, FALSE);
}

static gboolean
swfedit_bit_from_string (const char *s, gpointer* result)
{
  if (s[0] == '1' && s[1] == '\0')
    *result = GUINT_TO_POINTER (1);
  else if (s[0] == '0' && s[1] == '\0')
    *result = GUINT_TO_POINTER (0);
  else
    return FALSE;
  return TRUE;
}

static char *
swfedit_bit_to_string (gconstpointer value)
{
  return g_strdup (value ? "1" : "0");
}

static gboolean
swfedit_from_string_unsigned (const char *s, gulong max, gpointer* result)
{
  char *end;
  gulong u;

  g_assert (max <= G_MAXUINT);
  u = strtoul (s, &end, 10);
  if (*end != '\0')
    return FALSE;
  if (u > max)
    return FALSE;
  *result = GUINT_TO_POINTER (u);
  return TRUE;
}

static gboolean
swfedit_uint8_from_string (const char *s, gpointer* result)
{
  return swfedit_from_string_unsigned (s, G_MAXUINT8, result);
}

static gboolean
swfedit_uint16_from_string (const char *s, gpointer* result)
{
  return swfedit_from_string_unsigned (s, G_MAXUINT16, result);
}

static gboolean
swfedit_uint32_from_string (const char *s, gpointer* result)
{
  return swfedit_from_string_unsigned (s, G_MAXUINT32, result);
}

static char *
swfedit_to_string_unsigned (gconstpointer value)
{
  return g_strdup_printf ("%u", GPOINTER_TO_UINT (value));
}

static char *
swfedit_string_to_string (gconstpointer value)
{
  return g_strdup (value);
}

static gboolean
swfedit_string_from_string (const char *s, gpointer* result)
{
  *result = g_strdup (s);
  return TRUE;
}

static gpointer
swfedit_rect_new (void)
{
  return g_new0 (SwfdecRect, 1);
}

static gboolean
swfedit_rect_from_string (const char *s, gpointer* result)
{
  return FALSE;
}

static char *
swfedit_rect_to_string (gconstpointer value)
{
  const SwfdecRect *rect = value;

  return g_strdup_printf ("%d, %d, %d, %d", (int) rect->x0, (int) rect->y0,
      (int) rect->x1, (int) rect->y1);
}

static gboolean
swfedit_rgb_from_string (const char *s, gpointer* result)
{
  guint r, g, b;
  if (strlen (s) != 6)
    return FALSE;
  if (!swfedit_parse_hex (s, &r))
    return FALSE;
  s += 2;
  if (!swfedit_parse_hex (s, &g))
    return FALSE;
  s += 2;
  if (!swfedit_parse_hex (s, &b))
    return FALSE;
  *result = GUINT_TO_POINTER (SWFDEC_COLOR_COMBINE (r, g, b, 0xFF));
  return TRUE;
}

static char *
swfedit_rgb_to_string (gconstpointer value)
{
  guint c = GPOINTER_TO_UINT (value);

  return g_strdup_printf ("%02X%02X%02X", SWFDEC_COLOR_R (c),
      SWFDEC_COLOR_G (c), SWFDEC_COLOR_B (c));
}

static gboolean
swfedit_rgba_from_string (const char *s, gpointer* result)
{
  guint r, g, b, a;
  if (strlen (s) != 8)
    return FALSE;
  if (!swfedit_parse_hex (s, &a))
    return FALSE;
  s += 2;
  if (!swfedit_parse_hex (s, &r))
    return FALSE;
  s += 2;
  if (!swfedit_parse_hex (s, &g))
    return FALSE;
  s += 2;
  if (!swfedit_parse_hex (s, &b))
    return FALSE;
  *result = GUINT_TO_POINTER (SWFDEC_COLOR_COMBINE (r, g, b, a));
  return TRUE;
}

static char *
swfedit_rgba_to_string (gconstpointer value)
{
  guint c = GPOINTER_TO_UINT (value);

  return g_strdup_printf ("%02X%02X%02X%02X", SWFDEC_COLOR_R (c),
      SWFDEC_COLOR_G (c), SWFDEC_COLOR_B (c), SWFDEC_COLOR_A (c));
}

static gpointer
swfedit_matrix_new (void)
{
  cairo_matrix_t *matrix = g_new (cairo_matrix_t, 1);

  cairo_matrix_init_identity (matrix);
  return matrix;
}

static gboolean
swfedit_matrix_from_string (const char *s, gpointer* result)
{
  return FALSE;
}

static char *
swfedit_matrix_to_string (gconstpointer value)
{
  const cairo_matrix_t *mat = value;

  return g_strdup_printf ("{%g %g,  %g %g} + {%g, %g}", 
      mat->xx, mat->xy, mat->yx, mat->yy, mat->x0, mat->y0);
}

static gpointer
swfedit_ctrans_new (void)
{
  SwfdecColorTransform *trans = g_new (SwfdecColorTransform, 1);

  swfdec_color_transform_init_identity (trans);
  return trans;
}

static gboolean
swfedit_ctrans_from_string (const char *s, gpointer* result)
{
  return FALSE;
}

static char *
swfedit_ctrans_to_string (gconstpointer value)
{
  const SwfdecColorTransform *trans = value;

  return g_strdup_printf ("{%d %d} {%d %d} {%d %d} {%d %d}", 
      trans->ra, trans->rb, trans->ga, trans->gb, 
      trans->ba, trans->bb, trans->aa, trans->ab);
}

static gpointer
swfedit_script_new (void)
{
  return NULL;
}

static gboolean
swfedit_script_from_string (const char *s, gpointer* result)
{
  gpointer buffer;
  SwfdecScript *script;
  
  if (!swfedit_binary_from_string (s, &buffer)) {
    return FALSE;
  }

  script = swfdec_script_new (buffer, "unknown", 6 /* FIXME */);
  if (script != NULL) {
    *result = script;
    return TRUE;
  } else {
    return FALSE;
  }
}

static char *
swfedit_script_to_string (gconstpointer value)
{
  if (value == NULL)
    return g_strdup ("");
  else
    return swfedit_binary_to_string (((SwfdecScript *) value)->buffer);
}

static void
swfedit_script_free (gpointer script)
{
  if (script)
    swfdec_script_unref (script);
}

struct {
  gpointer	(* new)		(void);
  gboolean	(* from_string)	(const char *s, gpointer *);
  char *	(* to_string)	(gconstpointer value);
  void	  	(* free)	(gpointer value);
} converters[SWFEDIT_N_TOKENS] = {
  { NULL, NULL, NULL, g_object_unref },
  { swfedit_binary_new, swfedit_binary_from_string, swfedit_binary_to_string, (GDestroyNotify) swfdec_buffer_unref },
  { NULL, swfedit_bit_from_string, swfedit_bit_to_string, NULL },
  { NULL, swfedit_uint8_from_string, swfedit_to_string_unsigned, NULL },
  { NULL, swfedit_uint16_from_string, swfedit_to_string_unsigned, NULL },
  { NULL, swfedit_uint32_from_string, swfedit_to_string_unsigned, NULL },
  { NULL, swfedit_string_from_string, swfedit_string_to_string, g_free },
  { swfedit_rect_new, swfedit_rect_from_string, swfedit_rect_to_string, g_free },
  { NULL, swfedit_rgb_from_string, swfedit_rgb_to_string, NULL },
  { NULL, swfedit_rgba_from_string, swfedit_rgba_to_string, NULL },
  { swfedit_matrix_new, swfedit_matrix_from_string, swfedit_matrix_to_string, g_free },
  { swfedit_ctrans_new, swfedit_ctrans_from_string, swfedit_ctrans_to_string, g_free },
  { swfedit_script_new, swfedit_script_from_string, swfedit_script_to_string, swfedit_script_free },
  { NULL, swfedit_uint32_from_string, swfedit_to_string_unsigned, NULL },
};

gpointer
swfedit_token_new_token (SwfeditTokenType type)
{
  gpointer ret;

  g_assert (type < G_N_ELEMENTS (converters));

  if (!converters[type].new)
    return NULL;
  ret = converters[type].new ();
  return ret;
}

/*** GTK_TREE_MODEL ***/

#if 0
#  define REPORT g_print ("%s\n", G_STRFUNC)
#else
#  define REPORT 
#endif
static GtkTreeModelFlags 
swfedit_token_get_flags (GtkTreeModel *tree_model)
{
  REPORT;
  return 0;
}

static gint
swfedit_token_get_n_columns (GtkTreeModel *tree_model)
{
  SwfeditToken *token = SWFEDIT_TOKEN (tree_model);

  REPORT;
  return token->tokens->len;
}

static GType
swfedit_token_get_column_type (GtkTreeModel *tree_model, gint index_)
{
  REPORT;
  switch (index_) {
    case SWFEDIT_COLUMN_NAME:
      return G_TYPE_STRING;
    case SWFEDIT_COLUMN_VALUE_VISIBLE:
      return G_TYPE_BOOLEAN;
    case SWFEDIT_COLUMN_VALUE_EDITABLE:
      return G_TYPE_BOOLEAN;
    case SWFEDIT_COLUMN_VALUE:
      return G_TYPE_STRING;
    default:
      break;
  }
  g_assert_not_reached ();
  return G_TYPE_NONE;
}

static gboolean
swfedit_token_get_iter (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path)
{
  SwfeditToken *token = SWFEDIT_TOKEN (tree_model);
  guint i = gtk_tree_path_get_indices (path)[0];
  SwfeditTokenEntry *entry;
  
  REPORT;
  if (i > token->tokens->len)
    return FALSE;
  entry = &g_array_index (token->tokens, SwfeditTokenEntry, i);
  if (gtk_tree_path_get_depth (path) > 1) {
    GtkTreePath *new;
    int j;
    int *indices;
    gboolean ret;

    if (entry->type != SWFEDIT_TOKEN_OBJECT)
      return FALSE;
    new = gtk_tree_path_new ();
    indices = gtk_tree_path_get_indices (path);
    for (j = 1; j < gtk_tree_path_get_depth (path); j++) {
      gtk_tree_path_append_index (new, indices[j]);
    }
    ret = swfedit_token_get_iter (GTK_TREE_MODEL (entry->value), iter, new);
    gtk_tree_path_free (new);
    return ret;
  } else {
    iter->stamp = 0; /* FIXME */
    iter->user_data = token;
    iter->user_data2 = GINT_TO_POINTER (i);
    return TRUE;
  }
}

static GtkTreePath *
swfedit_token_get_path (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);
  GtkTreePath *path = gtk_tree_path_new_from_indices (GPOINTER_TO_INT (iter->user_data2), -1);

  REPORT;
  while (token->parent) {
    guint i;
    SwfeditToken *parent = token->parent;
    for (i = 0; i < parent->tokens->len; i++) {
      SwfeditTokenEntry *entry = &g_array_index (parent->tokens, SwfeditTokenEntry, i);
      if (entry->type != SWFEDIT_TOKEN_OBJECT)
	continue;
      if (entry->value == token)
	break;
    }
    gtk_tree_path_prepend_index (path, i);
    token = parent;
  }
  return path;
}

static void 
swfedit_token_get_value (GtkTreeModel *tree_model, GtkTreeIter *iter,
    gint column, GValue *value)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);
  SwfeditTokenEntry *entry = &g_array_index (token->tokens, SwfeditTokenEntry, GPOINTER_TO_INT (iter->user_data2));

  REPORT;
  switch (column) {
    case SWFEDIT_COLUMN_NAME:
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, entry->name);
      return;
    case SWFEDIT_COLUMN_VALUE_VISIBLE:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, converters[entry->type].to_string != NULL);
      return;
    case SWFEDIT_COLUMN_VALUE_EDITABLE:
      g_value_init (value, G_TYPE_BOOLEAN);
      g_value_set_boolean (value, entry->visible);
      return;
    case SWFEDIT_COLUMN_VALUE:
      g_value_init (value, G_TYPE_STRING);
      if (converters[entry->type].to_string)
	g_value_take_string (value, converters[entry->type].to_string (entry->value));
      return;
    default:
      break;
  }
  g_assert_not_reached ();
}

static gboolean
swfedit_token_iter_next (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);
  int i;

  REPORT;
  i = GPOINTER_TO_INT (iter->user_data2) + 1;
  if ((guint) i >= token->tokens->len)
    return FALSE;

  iter->user_data2 = GINT_TO_POINTER (i);
  return TRUE;
}

static gboolean
swfedit_token_iter_children (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent)
{
  SwfeditToken *token;
  SwfeditTokenEntry *entry;

  REPORT;
  if (parent) {
    token = SWFEDIT_TOKEN (parent->user_data);
    entry = &g_array_index (token->tokens, SwfeditTokenEntry, GPOINTER_TO_INT (parent->user_data2));
    if (entry->type != SWFEDIT_TOKEN_OBJECT)
      return FALSE;
    token = entry->value;
  } else {
    token = SWFEDIT_TOKEN (tree_model);
  }
  if (token->tokens->len == 0)
    return FALSE;
  iter->stamp = 0; /* FIXME */
  iter->user_data = token;
  iter->user_data2 = GINT_TO_POINTER (0);
  return TRUE;
}

static gboolean
swfedit_token_iter_has_child (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);
  SwfeditTokenEntry *entry = &g_array_index (token->tokens, SwfeditTokenEntry, GPOINTER_TO_INT (iter->user_data2));

  REPORT;
  return entry->type == SWFEDIT_TOKEN_OBJECT && SWFEDIT_TOKEN (entry->value)->tokens->len > 0;
}

static gint
swfedit_token_iter_n_children (GtkTreeModel *tree_model, GtkTreeIter *iter)
{
  SwfeditToken *token = SWFEDIT_TOKEN (iter->user_data);
  SwfeditTokenEntry *entry = &g_array_index (token->tokens, SwfeditTokenEntry, GPOINTER_TO_INT (iter->user_data2));

  REPORT;
  if (entry->type != SWFEDIT_TOKEN_OBJECT)
    return 0;

  token = entry->value;
  return token->tokens->len;
}

static gboolean
swfedit_token_iter_nth_child (GtkTreeModel *tree_model, GtkTreeIter *iter,
    GtkTreeIter *parent, gint n)
{
  SwfeditToken *token;
  SwfeditTokenEntry *entry;

  REPORT;
  if (parent) {
    token = SWFEDIT_TOKEN (parent->user_data);
    entry = &g_array_index (token->tokens, SwfeditTokenEntry, GPOINTER_TO_INT (parent->user_data2));

    if (entry->type != SWFEDIT_TOKEN_OBJECT)
      return FALSE;

    token = entry->value;
  } else {
    token = SWFEDIT_TOKEN (tree_model);
  }
  if ((guint) n >= token->tokens->len)
    return FALSE;
  iter->stamp = 0; /* FIXME */
  iter->user_data = token;
  iter->user_data2 = GINT_TO_POINTER (n);
  return TRUE;
}

static gboolean
swfedit_token_iter_parent (GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child)
{
  guint i;
  SwfeditToken *token = SWFEDIT_TOKEN (child->user_data);
  SwfeditToken *parent = token->parent;

  REPORT;
  if (parent == NULL)
    return FALSE;

  for (i = 0; i < parent->tokens->len; i++) {
    SwfeditTokenEntry *entry = &g_array_index (parent->tokens, SwfeditTokenEntry, i);
    if (entry->type != SWFEDIT_TOKEN_OBJECT)
      continue;
    if (entry->value == token)
      break;
  }
  iter->stamp = 0; /* FIXME */
  iter->user_data = parent;
  iter->user_data2 = GINT_TO_POINTER (i);
  return TRUE;
}

static void
swfedit_token_tree_model_init (GtkTreeModelIface *iface)
{
  iface->get_flags = swfedit_token_get_flags;
  iface->get_n_columns = swfedit_token_get_n_columns;
  iface->get_column_type = swfedit_token_get_column_type;
  iface->get_iter = swfedit_token_get_iter;
  iface->get_path = swfedit_token_get_path;
  iface->get_value = swfedit_token_get_value;
  iface->iter_next = swfedit_token_iter_next;
  iface->iter_children = swfedit_token_iter_children;
  iface->iter_has_child = swfedit_token_iter_has_child;
  iface->iter_n_children = swfedit_token_iter_n_children;
  iface->iter_nth_child = swfedit_token_iter_nth_child;
  iface->iter_parent = swfedit_token_iter_parent;
}

/*** SWFEDIT_TOKEN ***/

G_DEFINE_TYPE_WITH_CODE (SwfeditToken, swfedit_token, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL, swfedit_token_tree_model_init))

static void
swfedit_token_dispose (GObject *object)
{
  SwfeditToken *token = SWFEDIT_TOKEN (object);
  guint i;

  for (i = 0; i < token->tokens->len; i++) {
    SwfeditTokenEntry *entry = &g_array_index (token->tokens, SwfeditTokenEntry, i);
    g_free (entry->name);
    if (converters[entry->type].free)
      converters[entry->type].free (entry->value);
  }
  g_array_free (token->tokens, TRUE);

  G_OBJECT_CLASS (swfedit_token_parent_class)->dispose (object);
}

static void
swfedit_token_class_init (SwfeditTokenClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = swfedit_token_dispose;
}

static void
swfedit_token_init (SwfeditToken *token)
{
  token->tokens = g_array_new (FALSE, FALSE, sizeof (SwfeditTokenEntry));
}

SwfeditToken *
swfedit_token_new (void)
{
  SwfeditToken *token;

  token = g_object_new (SWFEDIT_TYPE_TOKEN, NULL);
  return token;
}

void
swfedit_token_add (SwfeditToken *token, const char *name, SwfeditTokenType type, 
    gpointer value)
{
  SwfeditTokenEntry entry = { NULL, type, value, TRUE };

  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (name != NULL);
  g_return_if_fail (type < SWFEDIT_N_TOKENS);

  g_assert (type != SWFEDIT_TOKEN_OBJECT || value != NULL);
  entry.name = g_strdup (name);
  g_array_append_val (token->tokens, entry);
}

void
swfedit_token_set (SwfeditToken *token, guint i, gpointer value)
{
  SwfeditTokenClass *klass;
  SwfeditTokenEntry *entry;
  GtkTreePath *path;
  SwfeditToken *model;
  GtkTreeIter iter;

  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (i < token->tokens->len);

  entry = &g_array_index (token->tokens, SwfeditTokenEntry, i);
  if (converters[entry->type].free != NULL)
    converters[entry->type].free (entry->value);
  entry->value = value;
  klass = SWFEDIT_TOKEN_GET_CLASS (token);
  if (klass->changed)
    klass->changed (token, i);

  model = token;
  while (model->parent)
    model = model->parent;
  iter.user_data = token;
  iter.user_data2 = GUINT_TO_POINTER (i);
  path = gtk_tree_model_get_path (GTK_TREE_MODEL (model), &iter);
  gtk_tree_model_row_changed (GTK_TREE_MODEL (model), path, &iter);
  gtk_tree_path_free (path);
}

void
swfedit_token_set_iter (SwfeditToken *token, GtkTreeIter *iter, const char *value)
{
  SwfeditTokenClass *klass;
  GtkTreeModel *model;
  SwfeditTokenEntry *entry;
  guint i;
  gpointer new;
  GtkTreePath *path;

  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (value != NULL);

  model = GTK_TREE_MODEL (token);
  token = iter->user_data;
  i = GPOINTER_TO_UINT (iter->user_data2);
  entry = &g_array_index (token->tokens, SwfeditTokenEntry, i);
  if (converters[entry->type].from_string == NULL)
    return;
  if (!converters[entry->type].from_string (value, &new))
    return;
  if (converters[entry->type].free != NULL)
    converters[entry->type].free (entry->value);
  entry->value = new;
  klass = SWFEDIT_TOKEN_GET_CLASS (token);
  if (klass->changed)
    klass->changed (token, i);

  path = gtk_tree_model_get_path (model, iter);
  gtk_tree_model_row_changed (model, path, iter);
  gtk_tree_path_free (path);
}

void
swfedit_token_set_visible (SwfeditToken *token, guint i, gboolean visible)
{
  SwfeditTokenEntry *entry;
  GtkTreeModel *model;
  GtkTreePath *path;
  GtkTreeIter iter;

  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (i < token->tokens->len);

  entry = &g_array_index (token->tokens, SwfeditTokenEntry, i);
  if (entry->visible == visible)
    return;

  entry->visible = visible;
  iter.stamp = 0; /* FIXME */
  iter.user_data = token;
  iter.user_data2 = GINT_TO_POINTER (i);
  while (token->parent) 
    token = token->parent;
  model = GTK_TREE_MODEL (token);
  path = gtk_tree_model_get_path (model, &iter);
  gtk_tree_model_row_changed (model, path, &iter);
  gtk_tree_path_free (path);
}
