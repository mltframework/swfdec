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

#include <libswfdec/swfdec_bits.h>
#include <libswfdec/swfdec_tag.h>
#include "swfedit_tag.h"
#include "swfdec_out.h"

/*** LOAD/SAVE ***/

static void
swfedit_binary_write (gpointer data, SwfdecOut *out)
{
  swfdec_out_put_buffer (out, data);
}

static gpointer
swfedit_binary_read (SwfdecBits *bits)
{
  SwfdecBuffer *buffer = swfdec_bits_get_buffer (bits, -1);
  if (buffer == NULL)
    buffer = swfdec_buffer_new ();
  return buffer;
}

static void
swfedit_u8_write (gpointer data, SwfdecOut *out)
{
  swfdec_out_put_u8 (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_u8_read (SwfdecBits *bits)
{
  return GUINT_TO_POINTER (swfdec_bits_get_u8 (bits));
}

static void
swfedit_u16_write (gpointer data, SwfdecOut *out)
{
  swfdec_out_put_u16 (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_u16_read (SwfdecBits *bits)
{
  return GUINT_TO_POINTER (swfdec_bits_get_u16 (bits));
}

static void
swfedit_u32_write (gpointer data, SwfdecOut *out)
{
  swfdec_out_put_u32 (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_u32_read (SwfdecBits *bits)
{
  return GUINT_TO_POINTER (swfdec_bits_get_u32 (bits));
}

static void
swfedit_rect_write (gpointer data, SwfdecOut *out)
{
  swfdec_out_put_rect (out, data);
}

static gpointer
swfedit_rect_read (SwfdecBits *bits)
{
  SwfdecRect *rect = g_new (SwfdecRect, 1);
  swfdec_bits_get_rect (bits, rect);
  return rect;
}

static void
swfedit_rgb_write (gpointer data, SwfdecOut *out)
{
  swfdec_out_put_rgb (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_rgb_read (SwfdecBits *bits)
{
  return GUINT_TO_POINTER (swfdec_bits_get_color (bits));
}

static void
swfedit_rgba_write (gpointer data, SwfdecOut *out)
{
  swfdec_out_put_rgba (out, GPOINTER_TO_UINT (data));
}

static gpointer
swfedit_rgba_read (SwfdecBits *bits)
{
  return GUINT_TO_POINTER (swfdec_bits_get_rgba (bits));
}

struct {
  void		(* write)	(gpointer data, SwfdecOut *out);
  gpointer	(* read)	(SwfdecBits *bits);
} operations[SWFEDIT_N_TOKENS] = {
  { NULL, NULL },
  { swfedit_binary_write, swfedit_binary_read },
  { swfedit_u8_write, swfedit_u8_read },
  { swfedit_u16_write, swfedit_u16_read },
  { swfedit_u32_write, swfedit_u32_read },
  { swfedit_rect_write, swfedit_rect_read },
  { swfedit_rgb_write, swfedit_rgb_read },
  { swfedit_rgba_write, swfedit_rgba_read },
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
  operations[entry->type].write (entry->value, out);
}

SwfdecBuffer *
swfedit_tag_write (SwfeditTag *tag)
{
  guint i;
  SwfdecOut *out;

  g_return_val_if_fail (SWFEDIT_IS_TAG (tag), NULL);

  out = swfdec_out_open ();
  for (i = 0; i < SWFEDIT_TOKEN (tag)->tokens->len; i++) {
    swfedit_tag_write_token (SWFEDIT_TOKEN (tag), out, i);
  }
  return swfdec_out_close (out);
}

void
swfedit_tag_read_token (SwfeditToken *token, SwfdecBits *bits,
    const char *name, SwfeditTokenType type)
{
  gpointer data;

  g_return_if_fail (SWFEDIT_IS_TOKEN (token));
  g_return_if_fail (name != NULL);
  
  g_assert (operations[type].read != NULL);
  data = operations[type].read (bits);
  swfedit_token_add (token, name, type, data);
}

/*** TAGS ***/

typedef struct {
  const char *	  	name;			/* name to use for this field */
  SwfeditTokenType     	type;			/* type of this field */
  guint			n_items;		/* field to look at for item count (or 0 to use 1 item) */
  guint			hint;			/* hint to pass to field when creating */
} SwfeditTagDefinition;

static const SwfeditTagDefinition ShowFrame[] = { { NULL, 0, 0, 0 } };
static const SwfeditTagDefinition SetBackgroundColor[] = { { "color", SWFEDIT_TOKEN_RGB, 0, 0 }, { NULL, 0, 0, 0 } };

static const SwfeditTagDefinition *tags[] = {
  [SWFDEC_TAG_SHOWFRAME] = ShowFrame,
  [SWFDEC_TAG_SETBACKGROUNDCOLOR] = SetBackgroundColor,
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
swfedit_tag_class_init (SwfeditTagClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = swfedit_tag_dispose;
}

static void
swfedit_tag_init (SwfeditTag *tag)
{
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
      swfedit_tag_read_token (SWFEDIT_TOKEN (item), &bits, def->name, def->type);
    }
  } else {
    swfedit_token_add (SWFEDIT_TOKEN (item), "contents", SWFEDIT_TOKEN_BINARY, buffer);
  }
  return item;
}

