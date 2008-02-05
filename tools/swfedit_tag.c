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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <gtk/gtk.h>

#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_debug.h>
#include <swfdec/swfdec_script_internal.h>
#include <swfdec/swfdec_tag.h>
#include "swfedit_tag.h"
#include "swfdec_out.h"
#include "swfedit_file.h"
#include "swfedit_list.h"

/*** LOAD/SAVE ***/

static void
swfedit_object_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  SwfdecBuffer *buffer;

  g_assert (SWFEDIT_IS_TOKEN (data));
  buffer = swfedit_tag_write (data);
  swfdec_out_put_buffer (out, buffer);
  swfdec_buffer_unref (buffer);
}

static gpointer
swfedit_object_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  return swfedit_list_new_read (token, bits, hint);
}

static void
swfedit_binary_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_buffer (out, data);
}

static gpointer
swfedit_binary_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  SwfdecBuffer *buffer = swfdec_bits_get_buffer (bits, -1);
  if (buffer == NULL)
    buffer = swfdec_buffer_new (0);
  return buffer;
}

static void
swfedit_bit_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_bit (out, data ? TRUE : FALSE);
}

static gpointer
swfedit_bit_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  return GUINT_TO_POINTER (swfdec_bits_getbit (bits) ? 1 : 0);
}

static void
swfedit_u8_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_u8 (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_u8_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  return GUINT_TO_POINTER ((gulong) swfdec_bits_get_u8 (bits));
}

static void
swfedit_u16_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_u16 (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_u16_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  return GUINT_TO_POINTER (swfdec_bits_get_u16 (bits));
}

static void
swfedit_u32_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_u32 (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_u32_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  return GUINT_TO_POINTER (swfdec_bits_get_u32 (bits));
}

static void
swfedit_rect_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_rect (out, data);
}

static gpointer
swfedit_rect_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  SwfdecRect *rect = g_new (SwfdecRect, 1);
  swfdec_bits_get_rect (bits, rect);
  swfdec_bits_syncbits (bits);
  return rect;
}

static void
swfedit_string_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_string (out, data);
}

static gpointer
swfedit_string_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  char *s;
  s = swfdec_bits_get_string (bits, 7);
  if (s == NULL)
    s = g_strdup ("");
  return s;
}

static void
swfedit_rgb_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_rgb (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_rgb_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  return GUINT_TO_POINTER (swfdec_bits_get_color (bits));
}

static void
swfedit_rgba_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_rgba (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_rgba_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  return GUINT_TO_POINTER (swfdec_bits_get_rgba (bits));
}

static void
swfedit_matrix_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_matrix (out, data);
}

static gpointer
swfedit_matrix_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  cairo_matrix_t *matrix = g_new (cairo_matrix_t, 1);

  swfdec_bits_get_matrix (bits, matrix, NULL);
  swfdec_bits_syncbits (bits);
  return matrix;
}

static void
swfedit_ctrans_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  swfdec_out_put_color_transform (out, data);
}

static gpointer
swfedit_ctrans_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  SwfdecColorTransform *ctrans = g_new (SwfdecColorTransform, 1);

  swfdec_bits_get_color_transform (bits, ctrans);
  swfdec_bits_syncbits (bits);
  return ctrans;
}

static void
swfedit_script_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  SwfdecScript *script = data;

  swfdec_out_put_buffer (out, script->buffer);
}

static gpointer
swfedit_script_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  while (token->parent)
    token = token->parent;
  if (!SWFEDIT_IS_FILE (token))
    return NULL;
  return swfdec_script_new_from_bits (bits, "original script", swfedit_file_get_version (SWFEDIT_FILE (token)));
}

static void
swfedit_clipeventflags_write (SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint)
{
  while (token->parent)
    token = token->parent;
  g_assert (SWFEDIT_IS_FILE (token));
  if (swfedit_file_get_version (SWFEDIT_FILE (token)) >= 6)
    swfdec_out_put_u32 (out, GPOINTER_TO_UINT (data));
  else
    swfdec_out_put_u16 (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_clipeventflags_read (SwfeditToken *token, SwfdecBits *bits, gconstpointer hint)
{
  while (token->parent)
    token = token->parent;
  g_assert (SWFEDIT_IS_FILE (token));
  if (swfedit_file_get_version (SWFEDIT_FILE (token)) >= 6)
    return GUINT_TO_POINTER (swfdec_bits_get_u32 (bits));
  else
    return GUINT_TO_POINTER (swfdec_bits_get_u16 (bits));
}

struct {
  void		(* write)	(SwfeditToken *token, gpointer data, SwfdecOut *out, gconstpointer hint);
  gpointer	(* read)	(SwfeditToken *token, SwfdecBits *bits, gconstpointer hint);
} operations[SWFEDIT_N_TOKENS] = {
  { swfedit_object_write, swfedit_object_read },
  { swfedit_binary_write, swfedit_binary_read },
  { swfedit_bit_write, swfedit_bit_read },
  { swfedit_u8_write, swfedit_u8_read },
  { swfedit_u16_write, swfedit_u16_read },
  { swfedit_u32_write, swfedit_u32_read },
  { swfedit_string_write, swfedit_string_read },
  { swfedit_rect_write, swfedit_rect_read },
  { swfedit_rgb_write, swfedit_rgb_read },
  { swfedit_rgba_write, swfedit_rgba_read },
  { swfedit_matrix_write, swfedit_matrix_read },
  { swfedit_ctrans_write, swfedit_ctrans_read },
  { swfedit_script_write, swfedit_script_read },
  { swfedit_clipeventflags_write, swfedit_clipeventflags_read },
};

void
swfedit_tag_write_token (SwfeditToken *token, SwfdecOut *out, guint i)
{
  SwfeditTokenEntry *entry;
  
  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (i < token->tokens->len);

  entry = &g_array_index (token->tokens, 
      SwfeditTokenEntry, i);
  g_assert (operations[entry->type].write != NULL);
  operations[entry->type].write (token, entry->value, out, NULL);
}

SwfdecBuffer *
swfedit_tag_write (SwfeditToken *token)
{
  guint i;
  SwfdecOut *out;

  g_return_val_if_fail (SWFEDIT_IS_TOKEN (token), NULL);

  out = swfdec_out_open ();
  for (i = 0; i < token->tokens->len; i++) {
    SwfeditTokenEntry *entry = &g_array_index (token->tokens, 
	SwfeditTokenEntry, i);
    if (entry->visible)
      swfedit_tag_write_token (token, out, i);
  }
  return swfdec_out_close (out);
}

void
swfedit_tag_read_token (SwfeditToken *token, SwfdecBits *bits,
    const char *name, SwfeditTokenType type, gconstpointer hint)
{
  gpointer data;

  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (name != NULL);
  
  g_assert (operations[type].read != NULL);
  data = operations[type].read (token, bits, hint);
  swfedit_token_add (token, name, type, data);
}

/*** TAGS ***/

static const SwfeditTagDefinition ShowFrame[] = { { NULL, 0, 0, NULL } };
static const SwfeditTagDefinition SetBackgroundColor[] = { { "color", SWFEDIT_TOKEN_RGB, 0, NULL }, { NULL, 0, 0, NULL } };
static const SwfeditTagDefinition PlaceObject2Action[] = {
  { "flags", SWFEDIT_TOKEN_CLIPEVENTFLAGS, 0, NULL },
  { "size", SWFEDIT_TOKEN_UINT32, 0, NULL },
  //{ "key code", SWFEDIT_TOKEN_UINT8, 0, NULL }, /* only if flag foo is set */
  { "script", SWFEDIT_TOKEN_SCRIPT, 2, NULL },
  { NULL, 0, 0, NULL }
};
static const SwfeditTagDefinition PlaceObject2[] = { 
  { "has clip actions", SWFEDIT_TOKEN_BIT, 0, NULL }, 
  { "has clip depth", SWFEDIT_TOKEN_BIT, 0, NULL }, 
  { "has name", SWFEDIT_TOKEN_BIT, 0, NULL }, 
  { "has ratio", SWFEDIT_TOKEN_BIT, 0, NULL }, 
  { "has color transform", SWFEDIT_TOKEN_BIT, 0, NULL }, 
  { "has matrix", SWFEDIT_TOKEN_BIT, 0, NULL }, 
  { "has character", SWFEDIT_TOKEN_BIT, 0, NULL }, 
  { "move", SWFEDIT_TOKEN_BIT, 0, NULL }, 
  { "depth", SWFEDIT_TOKEN_UINT16, 0, NULL }, 
  { "character", SWFEDIT_TOKEN_UINT16, 7, NULL }, 
  { "matrix", SWFEDIT_TOKEN_MATRIX, 6, NULL }, 
  { "color transform", SWFEDIT_TOKEN_CTRANS, 5, NULL }, 
  { "ratio", SWFEDIT_TOKEN_UINT16, 4, NULL }, 
  { "name", SWFEDIT_TOKEN_STRING, 3, NULL }, 
  { "clip depth", SWFEDIT_TOKEN_UINT16, 2, NULL }, 
  { "reserved", SWFEDIT_TOKEN_UINT16, 1, NULL },
  { "all flags", SWFEDIT_TOKEN_CLIPEVENTFLAGS, 1, NULL },
  { "actions", SWFEDIT_TOKEN_OBJECT, 1, PlaceObject2Action },
  { NULL, 0, 0, NULL }
};
static const SwfeditTagDefinition DoAction[] = {
  { "action", SWFEDIT_TOKEN_SCRIPT, 0, NULL },
  { NULL, 0, 0, NULL }
};
static const SwfeditTagDefinition DoInitAction[] = {
  { "character", SWFEDIT_TOKEN_UINT16, 0, NULL },
  { "action", SWFEDIT_TOKEN_SCRIPT, 0, NULL },
  { NULL, 0, 0, NULL }
};

static const SwfeditTagDefinition *tags[] = {
  [SWFDEC_TAG_SHOWFRAME] = ShowFrame,
  [SWFDEC_TAG_SETBACKGROUNDCOLOR] = SetBackgroundColor,
  [SWFDEC_TAG_PLACEOBJECT2] = PlaceObject2,
  [SWFDEC_TAG_DOACTION] = DoAction,
  [SWFDEC_TAG_DOINITACTION] = DoInitAction,
};

static const SwfeditTagDefinition *
swfedit_tag_get_definition (guint tag)
{
  if (tag >= G_N_ELEMENTS (tags))
    return NULL;
  return tags[tag];
}

/*** SWFEDIT_TAG ***/

G_DEFINE_TYPE (SwfeditTag, swfedit_tag, SWFEDIT_TYPE_TOKEN)

static void
swfedit_tag_dispose (GObject *object)
{
  //SwfeditTag *tag = SWFEDIT_TAG (object);

  G_OBJECT_CLASS (swfedit_tag_parent_class)->dispose (object);
}

static void
swfedit_tag_changed (SwfeditToken *token, guint i)
{
  guint j;
  SwfeditTag *tag = SWFEDIT_TAG (token);
  SwfeditTokenEntry *entry = &g_array_index (token->tokens, SwfeditTokenEntry, i);
  const SwfeditTagDefinition *def = swfedit_tag_get_definition (tag->tag);

  if (def == NULL)
    return;

  for (j = i + 1; def[j].name; j++) {
    if (def[j].n_items == i + 1) {
      swfedit_token_set_visible (token, j, entry->value != NULL);
    }
  }
  if (def[i].n_items != 0) {
    entry = &g_array_index (token->tokens, 
	SwfeditTokenEntry, def[i].n_items - 1);
    if (entry->type == SWFEDIT_TOKEN_UINT32) {
      SwfdecOut *out = swfdec_out_open ();
      SwfdecBuffer *buffer;
      swfedit_tag_write_token (token, out, def[i].n_items - 1);
      buffer = swfdec_out_close (out);
      if (entry->value != GUINT_TO_POINTER (buffer->length)) {
	swfedit_token_set (token, def[i].n_items - 1, 
	    GUINT_TO_POINTER (buffer->length));
      }
      swfdec_buffer_unref (buffer);
    }
  }
}

static void
swfedit_tag_class_init (SwfeditTagClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfeditTokenClass *token_class = SWFEDIT_TOKEN_CLASS (klass);

  object_class->dispose = swfedit_tag_dispose;

  token_class->changed = swfedit_tag_changed;
}

static void
swfedit_tag_init (SwfeditTag *tag)
{
}

void
swfedit_tag_add_token (SwfeditToken *token, const char *name, SwfeditTokenType type,
    gconstpointer hint)
{
  gpointer data;

  if (type == SWFEDIT_TOKEN_OBJECT) {
    data = swfedit_list_new (hint);
    SWFEDIT_TOKEN (data)->parent = token;
  } else {
    data = swfedit_token_new_token (type);
  }
  swfedit_token_add (token, name, type, data);
}

void
swfedit_tag_read_tag (SwfeditToken *token, SwfdecBits *bits, 
    const SwfeditTagDefinition *def)
{
  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (bits != NULL);
  g_return_if_fail (def != NULL);

  if (def->n_items != 0) {
    SwfeditTokenEntry *entry = &g_array_index (token->tokens, 
	SwfeditTokenEntry, def->n_items - 1);
    if (GPOINTER_TO_UINT (entry->value) == 0) {
      swfedit_tag_add_token (token, def->name, def->type, def->hint);
      swfedit_token_set_visible (token, token->tokens->len - 1, FALSE);
    } else if (entry->type == SWFEDIT_TOKEN_BIT) {
      swfedit_tag_read_token (token, bits, def->name, def->type, def->hint);
    } else {
      guint length = GPOINTER_TO_UINT (entry->value);
      SwfdecBuffer *buffer = swfdec_bits_get_buffer (bits, length);
      if (buffer == NULL) {
	swfedit_tag_add_token (token, def->name, def->type, def->hint);
      } else {
	SwfdecBits bits2;
	swfdec_bits_init (&bits2, buffer);
	swfedit_tag_read_token (token, &bits2, def->name, def->type, def->hint);
	swfdec_buffer_unref (buffer);
      }
    }
  } else {
    swfedit_tag_read_token (token, bits, def->name, def->type, def->hint);
  }
}

SwfeditTag *
swfedit_tag_new (SwfeditToken *parent, guint tag, SwfdecBuffer *buffer)
{
  SwfeditTag *item;
  const SwfeditTagDefinition *def;

  g_return_val_if_fail (SWFEDIT_IS_TOKEN (parent), NULL);

  item = g_object_new (SWFEDIT_TYPE_TAG, NULL);
  item->tag = tag;
  SWFEDIT_TOKEN (item)->parent = parent;
  def = swfedit_tag_get_definition (tag);
  if (def) {
    SwfdecBits bits;
    swfdec_bits_init (&bits, buffer);
    for (;def->name != NULL; def++) {
      swfedit_tag_read_tag (SWFEDIT_TOKEN (item), &bits, def);
    }
    if (swfdec_bits_left (&bits)) {
      SWFDEC_WARNING ("%u bytes %u bits left unparsed", 
	  swfdec_bits_left (&bits) / 8, swfdec_bits_left (&bits) % 8);
    }
  } else {
    swfedit_token_add (SWFEDIT_TOKEN (item), "contents", SWFEDIT_TOKEN_BINARY, buffer);
  }
  return item;
}

