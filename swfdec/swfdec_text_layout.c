/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
 *                    2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include <pango/pango.h>
#include <pango/pangocairo.h>
#include <string.h>

#include "swfdec_text_layout.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"

#define BULLET_MARGIN 36

enum {
  CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

static void
swfdec_text_attribute_apply_bold (SwfdecTextLayout *layout, const SwfdecTextAttributes *attr,
    PangoFontDescription *desc)
{
  pango_font_description_set_weight (desc, attr->bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
}

static PangoAttribute *
swfdec_text_attribute_create_color (SwfdecTextLayout *layout, const SwfdecTextAttributes *attr)
{
  return pango_attr_foreground_new (SWFDEC_COLOR_R (attr->color) * 255,
      SWFDEC_COLOR_G (attr->color) * 255, SWFDEC_COLOR_B (attr->color) * 255);
}

static void
swfdec_text_attribute_apply_font (SwfdecTextLayout *layout, const SwfdecTextAttributes *attr,
    PangoFontDescription *desc)
{
  /* FIXME: resolve _sans, _serif and friends */
  pango_font_description_set_family (desc, attr->font);
}

static void
swfdec_text_attribute_apply_italic (SwfdecTextLayout *layout, const SwfdecTextAttributes *attr,
    PangoFontDescription *desc)
{
  pango_font_description_set_style (desc, attr->italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
}

static PangoAttribute *
swfdec_text_attribute_create_letter_spacing (SwfdecTextLayout *layout, const SwfdecTextAttributes *attr)
{
  return pango_attr_letter_spacing_new (attr->letter_spacing * PANGO_SCALE);
}

static void
swfdec_text_attribute_apply_size (SwfdecTextLayout *layout, const SwfdecTextAttributes *attr,
    PangoFontDescription *desc)
{
  guint size = attr->size * layout->scale;
  size = MAX (size, 1);
  pango_font_description_set_absolute_size (desc, size * PANGO_SCALE);
}

static PangoAttribute *
swfdec_text_attribute_create_underline (SwfdecTextLayout *layout, const SwfdecTextAttributes *attr)
{
  return pango_attr_underline_new (attr->underline ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE);
}

/*** THE BIG OPTIONS TABLE ***/

typedef enum {
  FORMAT_APPLY_UNKNOWN,
  FORMAT_APPLY_BLOCK,
  FORMAT_APPLY_LINE,
  FORMAT_APPLY_GLYPH
} FormatApplication;

struct {
  FormatApplication   application;
  PangoAttribute *    (* create_attribute) (SwfdecTextLayout *layout, const SwfdecTextAttributes *attr);
  void		      (* apply_attribute) (SwfdecTextLayout *layout, const SwfdecTextAttributes *attr, PangoFontDescription *desc);
} format_table[] = {
  /* unknown or unhandled properties */
  [SWFDEC_TEXT_ATTRIBUTE_DISPLAY] = { FORMAT_APPLY_UNKNOWN, NULL, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_KERNING] = { FORMAT_APPLY_UNKNOWN, NULL, NULL },
  /* per-paragraph options */
  [SWFDEC_TEXT_ATTRIBUTE_BULLET] = { FORMAT_APPLY_BLOCK, NULL, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_INDENT] = { FORMAT_APPLY_BLOCK, NULL, NULL },
  /* per-line options */
  [SWFDEC_TEXT_ATTRIBUTE_ALIGN] = { FORMAT_APPLY_LINE, NULL, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_BLOCK_INDENT] = { FORMAT_APPLY_LINE, NULL, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_LEADING] = { FORMAT_APPLY_LINE, NULL, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_LEFT_MARGIN] = { FORMAT_APPLY_LINE, NULL, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_RIGHT_MARGIN] = { FORMAT_APPLY_LINE, NULL, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS] = { FORMAT_APPLY_LINE, NULL, NULL },
  /* per-glyph options */
  [SWFDEC_TEXT_ATTRIBUTE_BOLD] = { FORMAT_APPLY_GLYPH, NULL, swfdec_text_attribute_apply_bold },
  [SWFDEC_TEXT_ATTRIBUTE_COLOR] = { FORMAT_APPLY_GLYPH, swfdec_text_attribute_create_color, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_FONT] = { FORMAT_APPLY_GLYPH, NULL, swfdec_text_attribute_apply_font },
  [SWFDEC_TEXT_ATTRIBUTE_ITALIC] = { FORMAT_APPLY_GLYPH, NULL, swfdec_text_attribute_apply_italic },
  [SWFDEC_TEXT_ATTRIBUTE_LETTER_SPACING] = { FORMAT_APPLY_GLYPH, swfdec_text_attribute_create_letter_spacing, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_SIZE] = { FORMAT_APPLY_GLYPH, NULL, swfdec_text_attribute_apply_size },
  [SWFDEC_TEXT_ATTRIBUTE_TARGET] = { FORMAT_APPLY_GLYPH, NULL, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_UNDERLINE] = { FORMAT_APPLY_GLYPH, swfdec_text_attribute_create_underline, NULL },
  [SWFDEC_TEXT_ATTRIBUTE_URL] = { FORMAT_APPLY_GLYPH, NULL }
};

#define SWFDEC_TEXT_ATTRIBUTES_MASK_NEW_BLOCK (\
    (1 << SWFDEC_TEXT_ATTRIBUTE_ALIGN) | \
    (1 << SWFDEC_TEXT_ATTRIBUTE_BLOCK_INDENT) | \
    (1 << SWFDEC_TEXT_ATTRIBUTE_LEADING) | \
    (1 << SWFDEC_TEXT_ATTRIBUTE_LEFT_MARGIN) | \
    (1 << SWFDEC_TEXT_ATTRIBUTE_RIGHT_MARGIN) | \
    (1 << SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS))

/*** BLOCK ***/

/* A block is one vertical part of the layout that can be represented by exactly 
 * one PangoLayout. */

typedef struct _SwfdecTextBlock SwfdecTextBlock;

struct _SwfdecTextBlock {
  PangoLayout *		layout;		/* the layout we render */
  SwfdecRectangle	rect;		/* the area occupied by this layout */
  guint			row;		/* index of start row */
  gsize			start;		/* first character drawn */
  gsize			end;		/* first character not drawn anymore */
  gboolean		bullet;		/* TRUE if we need to draw a bullet in front of this layout */
};

static void
swfdec_text_block_free (gpointer blockp)
{
  SwfdecTextBlock *block = blockp;

  g_object_unref (block->layout);

  g_slice_free (SwfdecTextBlock, block);
}

static SwfdecTextBlock *
swfdec_text_block_new (PangoContext *context)
{
  SwfdecTextBlock *block;

  block = g_slice_new0 (SwfdecTextBlock);
  block->layout = pango_layout_new (context);
  pango_layout_set_wrap (block->layout, PANGO_WRAP_WORD_CHAR);

  return block;
}

/*** MAINTAINING THE LAYOUT ***/

static void
swfdec_text_layout_apply_paragraph_attributes (SwfdecTextLayout *layout,
    SwfdecTextBlock *block, const SwfdecTextAttributes *attr)
{
  int indent;

  /* SWFDEC_TEXT_ATTRIBUTE_BULLET */
  block->bullet = attr->bullet;
  if (block->bullet) {
    block->rect.x += BULLET_MARGIN;
    if (block->rect.width <= BULLET_MARGIN) {
      SWFDEC_FIXME ("out of horizontal space, what now?");
      block->rect.width = 0;
    } else {
      block->rect.width -= BULLET_MARGIN;
    }
  }

  /* SWFDEC_TEXT_ATTRIBUTE_INDENT */
  indent = attr->indent;
  indent = MAX (indent, - attr->left_margin - attr->block_indent);
  pango_layout_set_indent (block->layout, indent * PANGO_SCALE);
}

static void
swfdec_text_layout_apply_line_attributes (SwfdecTextBlock *block,
    const SwfdecTextAttributes *attr)
{
  /* SWFDEC_TEXT_ATTRIBUTE_ALIGN */
  switch (attr->align) {
    case SWFDEC_TEXT_ALIGN_LEFT:
      pango_layout_set_alignment (block->layout, PANGO_ALIGN_LEFT);
      pango_layout_set_justify (block->layout, FALSE);
      break;
    case SWFDEC_TEXT_ALIGN_RIGHT:
      pango_layout_set_alignment (block->layout, PANGO_ALIGN_RIGHT);
      pango_layout_set_justify (block->layout, FALSE);
      break;
    case SWFDEC_TEXT_ALIGN_CENTER:
      pango_layout_set_alignment (block->layout, PANGO_ALIGN_CENTER);
      pango_layout_set_justify (block->layout, FALSE);
      break;
    case SWFDEC_TEXT_ALIGN_JUSTIFY:
      pango_layout_set_alignment (block->layout, PANGO_ALIGN_LEFT);
      pango_layout_set_justify (block->layout, TRUE);
      break;
    default:
      g_assert_not_reached ();
  }

  /* SWFDEC_TEXT_ATTRIBUTE_BLOCK_INDENT */
  /* SWFDEC_TEXT_ATTRIBUTE_LEFT_MARGIN */
  /* SWFDEC_TEXT_ATTRIBUTE_RIGHT_MARGIN */
  block->rect.x += attr->left_margin + attr->block_indent;
  if (block->rect.width <= attr->left_margin + attr->block_indent + attr->right_margin) {
    SWFDEC_FIXME ("out of horizontal space, what now?");
    block->rect.width = 0;
  } else {
    block->rect.width -= attr->left_margin + attr->block_indent + attr->right_margin;
  }

  /* SWFDEC_TEXT_ATTRIBUTE_LEADING */
  if (attr->leading > 0)
    pango_layout_set_spacing (block->layout, attr->leading * PANGO_SCALE);
  else
    pango_layout_set_spacing (block->layout, (attr->leading - 1) * PANGO_SCALE);

  /* SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS */
  if (attr->n_tab_stops != 0) {
    PangoTabArray *tabs = pango_tab_array_new (attr->n_tab_stops, TRUE);
    guint i;
    for (i = 0; i < attr->n_tab_stops; i++) {
      pango_tab_array_set_tab (tabs, i, PANGO_TAB_LEFT, attr->tab_stops[i]);
    }
    pango_layout_set_tabs (block->layout, tabs);
    pango_tab_array_free (tabs);
  }
}

static void
swfdec_text_layout_apply_attributes_to_description (SwfdecTextLayout *layout, 
    const SwfdecTextAttributes *attr, PangoFontDescription *desc)
{
  guint i;

  for (i = 0; i < G_N_ELEMENTS (format_table); i++) {
    if (format_table[i].apply_attribute) {
      format_table[i].apply_attribute (layout, attr, desc);
    }
  }
}

static void
swfdec_text_layout_apply_attributes (SwfdecTextLayout *layout, PangoAttrList *list,
    const SwfdecTextAttributes *attr, guint start, guint end)
{
  PangoAttribute *attribute;
  PangoFontDescription *desc;
  guint i;

  for (i = 0; i < G_N_ELEMENTS (format_table); i++) {
    if (format_table[i].create_attribute) {
      attribute = format_table[i].create_attribute (layout, attr);
      attribute->start_index = start;
      attribute->end_index = end;
      pango_attr_list_change (list, attribute);
    }
  }
  desc = pango_font_description_new ();
  swfdec_text_layout_apply_attributes_to_description (layout, attr, desc);
  attribute = pango_attr_font_desc_new (desc);
  pango_font_description_free (desc);
  attribute->start_index = start;
  attribute->end_index = end;
  pango_attr_list_change (list, attribute);
}

static const SwfdecTextBlock *
swfdec_text_layout_create_paragraph (SwfdecTextLayout *layout, PangoContext *context,
    const SwfdecTextBlock *last, gsize start, gsize end)
{
  SwfdecTextBufferIter *iter;
  SwfdecTextBlock *block;
  PangoAttrList *list;
  const char *string;
  const SwfdecTextAttributes *attr, *attr_next;
  gsize new_block, start_next;
  PangoFontDescription *desc;
  gboolean first = TRUE;
  // TODO: kerning, display

  string = swfdec_text_buffer_get_text (layout->text);
  do {
    iter = swfdec_text_buffer_get_iter (layout->text, start);
    attr = swfdec_text_buffer_iter_get_attributes (layout->text, iter);
    new_block = end;

    block = swfdec_text_block_new (context);
    block->start = start;
    block->rect.x = 0;
    if (last) {
      block->rect.y = last->rect.y + last->rect.height;
      block->row = last->row + pango_layout_get_line_count (last->layout);
    } else {
      block->rect.y = 0;
      block->row = 0;
    }
    if (layout->wrap_width == -1) {
      block->rect.width = G_MAXINT;
    } else {
      block->rect.width = layout->wrap_width;
    }
    desc = pango_font_description_new ();
    swfdec_text_layout_apply_attributes_to_description (layout, attr, desc);
    pango_layout_set_font_description (block->layout, desc);
    pango_font_description_free (desc);
    g_sequence_append (layout->blocks, block);

    if (layout->password) {
      /* requires sane line breaking so we can't just replace the text with * chars.
       * A good idea might be using a custom font that replaces all chars upon 
       * rendering, so Pango still thinks it uses proper utf-8 */
      SWFDEC_FIXME ("implement password handling.");
    }
    pango_layout_set_text (block->layout, string + start, end - start);

    if (first) {
      swfdec_text_layout_apply_paragraph_attributes (layout, block, attr);
      first = FALSE;
    }
    swfdec_text_layout_apply_line_attributes (block, attr);
    if (layout->wrap_width != -1)
      pango_layout_set_width (block->layout, block->rect.width * PANGO_SCALE);

    iter = swfdec_text_buffer_get_iter (layout->text, start);
    attr = swfdec_text_buffer_iter_get_attributes (layout->text, iter);
    list = pango_attr_list_new ();

    while (start != end) {
      g_assert (attr);
      iter = swfdec_text_buffer_iter_next (layout->text, iter);
      attr_next = iter ? swfdec_text_buffer_iter_get_attributes (layout->text, iter) : NULL;
      start_next = iter ? swfdec_text_buffer_iter_get_start (layout->text, iter) :
	swfdec_text_buffer_get_length (layout->text);
      start_next = MIN (start_next, end);
      
      swfdec_text_layout_apply_attributes (layout, list, attr, start - block->start, start_next - block->start);

      if (attr_next && new_block > start_next &&
	  (swfdec_text_attributes_diff (attr, attr_next) & SWFDEC_TEXT_ATTRIBUTES_MASK_NEW_BLOCK))
	new_block = start_next;

      attr = attr_next;
      start = start_next;
    }
    pango_layout_set_attributes (block->layout, list);
    pango_attr_list_unref (list);
    
    if (new_block < end) {
      int i;

      for (i = 0; i < pango_layout_get_line_count (block->layout); i++) {
	PangoLayoutLine *line = pango_layout_get_line_readonly (block->layout, i);
	if ((gsize) line->start_index + line->length >= new_block) {
	  new_block = line->start_index + line->length;
	  /* I hope deleting lines actually works by removing text */
	  pango_layout_set_text (block->layout, string + start, new_block - start);
	  g_assert (i + 1 == pango_layout_get_line_count (block->layout));
	  break;
	}
      }
      /* check that we have found a line */
      g_assert (i < pango_layout_get_line_count (block->layout));
    }

    start = new_block;
    pango_layout_get_pixel_size (block->layout, &block->rect.width, &block->rect.height);
    /* add leading to last line, too */
    block->rect.height += pango_layout_get_spacing (block->layout) / PANGO_SCALE;

    last = block;
  } while (start != end);

  return last;
}

static void
swfdec_text_layout_create (SwfdecTextLayout *layout)
{
  const char *p, *end, *string;
  PangoContext *context;
  PangoFontMap *map;
  const SwfdecTextBlock *last = NULL;

  map = pango_cairo_font_map_get_default ();
  context = pango_cairo_font_map_create_context (PANGO_CAIRO_FONT_MAP (map));

  p = string = swfdec_text_buffer_get_text (layout->text);
  //g_print ("=====>\n %s\n<======\n", string);
  for (;;) {
    end = strpbrk (p, "\r\n");
    if (end == NULL) {
      end = string + swfdec_text_buffer_get_length (layout->text);
    }

    last = swfdec_text_layout_create_paragraph (layout, context, last, p - string, end - string);

    if (*end == '\0')
      break;
    p = end + 1;
  }

  g_object_unref (context);
}

static void
swfdec_text_layout_ensure (SwfdecTextLayout *layout)
{
  if (!g_sequence_iter_is_end (g_sequence_get_begin_iter (layout->blocks)))
    return;

  swfdec_text_layout_create (layout);
}

static void
swfdec_text_layout_invalidate (SwfdecTextLayout *layout)
{
  if (g_sequence_iter_is_end (g_sequence_get_begin_iter (layout->blocks)))
    return;

  g_sequence_remove_range (g_sequence_get_begin_iter (layout->blocks),
    g_sequence_get_end_iter (layout->blocks));
  g_signal_emit (layout, signals[CHANGED], 0);
}

/*** LAYOUT ***/

/* A layout represents the whole text of a TextFieldMovie, this includes the
 * invisible parts. It also does not care about transformations.
 * It's the job of the movie to take care of that. */
G_DEFINE_TYPE (SwfdecTextLayout, swfdec_text_layout, G_TYPE_OBJECT)

static void
swfdec_text_layout_dispose (GObject *object)
{
  SwfdecTextLayout *layout = SWFDEC_TEXT_LAYOUT (object);

  g_signal_handlers_disconnect_matched (layout->text, 
	G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, layout);
  g_object_unref (layout->text);
  layout->text = NULL;
  g_sequence_free (layout->blocks);
  layout->blocks = NULL;

  G_OBJECT_CLASS (swfdec_text_layout_parent_class)->dispose (object);
}

static void
swfdec_text_layout_class_init (SwfdecTextLayoutClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_text_layout_dispose;

  signals[CHANGED] = g_signal_new ("changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
}

static void
swfdec_text_layout_init (SwfdecTextLayout *layout)
{
  layout->wrap_width = -1;
  layout->scale = 1.0;
  layout->blocks = g_sequence_new (swfdec_text_block_free);
}

SwfdecTextLayout *
swfdec_text_layout_new (SwfdecTextBuffer *buffer)
{
  SwfdecTextLayout *layout;

  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), NULL);

  layout = g_object_new (SWFDEC_TYPE_TEXT_LAYOUT, NULL);
  layout->text = g_object_ref (buffer);
  g_signal_connect_swapped (buffer, "text-changed",
      G_CALLBACK (swfdec_text_layout_invalidate), layout);

  return layout;
}

void
swfdec_text_layout_set_wrap_width (SwfdecTextLayout *layout, int wrap_width)
{
  g_return_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (wrap_width >= -1);

  if (layout->wrap_width == wrap_width)
    return;

  layout->wrap_width = wrap_width;
  swfdec_text_layout_invalidate (layout);
}

int
swfdec_text_layout_get_wrap_width (SwfdecTextLayout *layout)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout), -1);

  return layout->wrap_width;
}

gboolean
swfdec_text_layout_get_password (SwfdecTextLayout *layout)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout), FALSE);

  return layout->password;
}

void
swfdec_text_layout_set_password (SwfdecTextLayout *layout, gboolean password)
{
  g_return_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout));

  if (layout->password == password)
    return;

  layout->password = password;
  swfdec_text_layout_invalidate (layout);
}

double
swfdec_text_layout_get_scale (SwfdecTextLayout *layout)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout), 1.0);

  return layout->scale;
}

void
swfdec_text_layout_set_scale (SwfdecTextLayout *layout, double scale)
{
  g_return_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (scale > 0);

  if (layout->scale == scale)
    return;

  layout->scale = scale;
  swfdec_text_layout_invalidate (layout);
}

/**
 * swfdec_text_layout_get_width:
 * @layout: the layout
 *
 * Computes the width of the layout in pixels. Note that the width can still
 * exceed the width set with swfdec_text_layout_set_wrap_width() if some
 * words are too long. Computing the width takes a long time, so it might be
 * useful to cache the value.
 *
 * Returns: The width in pixels
 **/
guint
swfdec_text_layout_get_width (SwfdecTextLayout *layout)
{
  GSequenceIter *iter;
  SwfdecTextBlock *block;
  guint width = 0;

  g_return_val_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout), 0);

  swfdec_text_layout_ensure (layout);

  for (iter = g_sequence_get_begin_iter (layout->blocks);
       !g_sequence_iter_is_end (iter);
       iter = g_sequence_iter_next (iter)) {
    block = g_sequence_get (iter);
    width = MAX (width, (guint) block->rect.x + block->rect.width);
  }

  return width;
}

/**
 * swfdec_text_layout_get_height:
 * @layout: the layout
 *
 * Computes the height of the layout in pixels. This is a fast operation.
 *
 * Returns: The height of the layout in pixels
 **/
guint
swfdec_text_layout_get_height (SwfdecTextLayout *layout)
{
  GSequenceIter *iter;
  SwfdecTextBlock *block;

  g_return_val_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout), 0);

  swfdec_text_layout_ensure (layout);

  iter = g_sequence_iter_prev (g_sequence_get_end_iter (layout->blocks));
  block = g_sequence_get (iter);
  return block->rect.y + block->rect.height;
}

guint
swfdec_text_layout_get_n_rows (SwfdecTextLayout *layout)
{
  GSequenceIter *iter;
  SwfdecTextBlock *block;

  g_return_val_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout), 0);

  swfdec_text_layout_ensure (layout);

  iter = g_sequence_iter_prev (g_sequence_get_end_iter (layout->blocks));
  block = g_sequence_get (iter);
  return block->row + pango_layout_get_line_count (block->layout);
}

static GSequenceIter *
swfdec_text_layout_find_row (SwfdecTextLayout *layout, guint row)
{
  GSequenceIter *begin, *end, *mid;
  SwfdecTextBlock *cur;

  begin = g_sequence_get_begin_iter (layout->blocks);
  end = g_sequence_iter_prev (g_sequence_get_end_iter (layout->blocks));
  while (begin != end) {
    mid = g_sequence_range_get_midpoint (begin, end); 
    if (mid == begin)
      mid = g_sequence_iter_next (mid);
    cur = g_sequence_get (mid);
    if (cur->row > row)
      end = g_sequence_iter_prev (mid);
    else
      begin = mid;
  }
  return begin;
}

/**
 * swfdec_text_layout_get_visible_rows:
 * @layout: the layout
 * @row: the first visible row
 * @height: the height in pixels of visible text
 *
 * Queries the number of rows that would be rendered, if rendering were to start
 * with @row and had to fit into @height pixels.
 *
 * Returns: the number of rows that would be rendered
 **/
guint
swfdec_text_layout_get_visible_rows (SwfdecTextLayout *layout, guint row, guint height)
{
  GSequenceIter *iter;
  SwfdecTextBlock *block;
  PangoRectangle extents;
  guint count = 0;

  g_return_val_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout), 1);
  g_return_val_if_fail (row < swfdec_text_layout_get_n_rows (layout), 1);

  swfdec_text_layout_ensure (layout);

  iter = swfdec_text_layout_find_row (layout, row); 
  block = g_sequence_get (iter);
  row -= block->row;

  do {
    block = g_sequence_get (iter);
    for (;row < (guint) pango_layout_get_line_count (block->layout); row++) {
      PangoLayoutLine *line = pango_layout_get_line_readonly (block->layout, row);
      
      pango_layout_line_get_pixel_extents (line, NULL, &extents);
      if (extents.height > (int) height)
	goto out;
      height -= extents.height;
      count++;
    }
    row = 0;
    if ((int) height <= pango_layout_get_spacing (block->layout) / PANGO_SCALE)
      goto out;
    height -= pango_layout_get_spacing (block->layout) / PANGO_SCALE;
    iter = g_sequence_iter_next (iter);
  } while (!g_sequence_iter_is_end (iter));

out:
  return MAX (count, 1);
}

/**
 * swfdec_text_layout_get_visible_rows_end:
 * @layout: the layout
 * @height: the height in pixels
 *
 * Computes how many rows will be visible if only the last rows would be 
 * displayed. This is useful for computing maximal scroll offsets.
 *
 * Returns: The number of rows visible at the bottom.
 **/
guint
swfdec_text_layout_get_visible_rows_end (SwfdecTextLayout *layout, guint height)
{
  GSequenceIter *iter;
  SwfdecTextBlock *block;
  PangoRectangle extents;
  guint row, count = 0;

  g_return_val_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout), 1);

  swfdec_text_layout_ensure (layout);
  iter = g_sequence_get_end_iter (layout->blocks);

  do {
    iter = g_sequence_iter_prev (iter);
    block = g_sequence_get (iter);
    if ((int) height <= pango_layout_get_spacing (block->layout) / PANGO_SCALE)
      goto out;
    height -= pango_layout_get_spacing (block->layout) / PANGO_SCALE;
    for (row = pango_layout_get_line_count (block->layout); row > 0; row--) {
      PangoLayoutLine *line = pango_layout_get_line_readonly (block->layout, row - 1);
      
      pango_layout_line_get_pixel_extents (line, NULL, &extents);
      if (extents.height > (int) height) 
	goto out;
      height -= extents.height;
      count++;
    }
  } while (!g_sequence_iter_is_begin (iter));

out:
  return MAX (count, 1);
}

/**
 * swfdec_text_layout_render:
 * @layout: the layout to render
 * @cr: the Cairo context to render to. The context will be transformed so that
 *      (0, 0) points to where the @row should be rendered.
 * @ctrans: The color transform to apply.
 * @row: index of the first row to render.
 * @height: The height in pixels of the visible area.
 * @inval: The invalid area.
 *
 * Renders the contents of the layout into the given Cairo context.
 **/
void
swfdec_text_layout_render (SwfdecTextLayout *layout, cairo_t *cr, 
    const SwfdecColorTransform *ctrans, guint row, guint height,
    const SwfdecRectangle *inval)
{
  GSequenceIter *iter;
  SwfdecTextBlock *block;
  PangoRectangle extents;
  gboolean first_line = TRUE;

  g_return_if_fail (SWFDEC_IS_TEXT_LAYOUT (layout));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (ctrans != NULL);
  g_return_if_fail (row < swfdec_text_layout_get_n_rows (layout));
  g_return_if_fail (inval != NULL);
  
  swfdec_text_layout_ensure (layout);

  iter = swfdec_text_layout_find_row (layout, row); 
  block = g_sequence_get (iter);
  row -= block->row;
  do {
    block = g_sequence_get (iter);
    pango_cairo_update_layout (cr, block->layout);
    cairo_translate (cr, block->rect.x, 0);
    if (block->bullet && row == 0) {
      SWFDEC_FIXME ("render bullet");
    }
    for (;row < (guint) pango_layout_get_line_count (block->layout); row++) {
      PangoLayoutLine *line = pango_layout_get_line_readonly (block->layout, row);
      
      pango_layout_line_get_pixel_extents (line, NULL, &extents);
      if (extents.height > (int) height && !first_line)
	return;
      first_line = FALSE;
      cairo_translate (cr, 0, - extents.y);
      pango_cairo_show_layout_line (cr, line);
      height -= extents.height;
      cairo_translate (cr, 0, extents.height + extents.y);
    }
    if ((int) height <= pango_layout_get_spacing (block->layout) / PANGO_SCALE)
      return;
    height -= pango_layout_get_spacing (block->layout) / PANGO_SCALE;
    cairo_translate (cr, -block->rect.x, pango_layout_get_spacing (block->layout) / PANGO_SCALE);
    row = 0;
    iter = g_sequence_iter_next (iter);
  } while (!g_sequence_iter_is_end (iter));
}

