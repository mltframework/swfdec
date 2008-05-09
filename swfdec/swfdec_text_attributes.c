/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include <string.h>
#include "swfdec_text_attributes.h"
#include "swfdec_as_context.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"

void
swfdec_text_attributes_reset (SwfdecTextAttributes *attr)
{
  g_free (attr->tab_stops);

  attr->align = SWFDEC_TEXT_ALIGN_LEFT;
  attr->block_indent = 0;
  attr->bold = FALSE;
  attr->bullet = FALSE;
  attr->color = 0;
  attr->display = SWFDEC_TEXT_DISPLAY_BLOCK;
  attr->font = SWFDEC_AS_STR_Times_New_Roman;
  attr->indent = 0;
  attr->italic = FALSE;
  attr->kerning = FALSE;
  attr->leading = 0;
  attr->left_margin = 0;
  attr->letter_spacing = 0;
  attr->right_margin = 0;
  attr->size = 12;
  attr->tab_stops = NULL;
  attr->n_tab_stops = 0;
  attr->target = SWFDEC_AS_STR_EMPTY;
  attr->url = SWFDEC_AS_STR_EMPTY;
  attr->underline = FALSE;
}

void
swfdec_text_attributes_copy (SwfdecTextAttributes *attr, const SwfdecTextAttributes *from,
    guint flags)
{
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_ALIGN))
    attr->align = from->align;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_BLOCK_INDENT))
    attr->block_indent = from->block_indent;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_BOLD))
    attr->bold = from->bold;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_BULLET))
    attr->bullet = from->bullet;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_COLOR))
    attr->color = from->color;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_DISPLAY))
    attr->display = from->display;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_FONT))
    attr->font = from->font;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_INDENT))
    attr->indent = from->indent;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_ITALIC))
    attr->italic = from->italic;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_KERNING))
    attr->kerning = from->kerning;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_LEADING))
    attr->leading = from->leading;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_LEFT_MARGIN))
    attr->left_margin = from->left_margin;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_LETTER_SPACING))
    attr->letter_spacing = from->letter_spacing;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_RIGHT_MARGIN))
    attr->right_margin = from->right_margin;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_SIZE))
    attr->size = from->size;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS)) {
    g_free (attr->tab_stops);
    attr->tab_stops = g_memdup (from->tab_stops, sizeof (guint) * from->n_tab_stops);
    attr->n_tab_stops = from->n_tab_stops;
  }
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_TARGET))
    attr->target = from->target;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_UNDERLINE))
    attr->underline = from->underline;
  if (SWFDEC_TEXT_ATTRIBUTE_IS_SET (flags, SWFDEC_TEXT_ATTRIBUTE_URL))
    attr->url = from->url;
}

void
swfdec_text_attributes_mark (SwfdecTextAttributes *attr)
{
  swfdec_as_string_mark (attr->font);
  swfdec_as_string_mark (attr->target);
  swfdec_as_string_mark (attr->url);
}

guint
swfdec_text_attributes_diff (const SwfdecTextAttributes *a, const SwfdecTextAttributes *b)
{
  guint result = 0;

  if (a->align != b->align)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_ALIGN);
  if (a->block_indent != b->block_indent)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_BLOCK_INDENT);
  if (a->bold != b->bold)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_BOLD);
  if (a->bullet != b->bullet)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_BULLET);
  if (a->color != b->color)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_COLOR);
  if (a->display != b->display)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_DISPLAY);
  if (a->font != b->font)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_FONT);
  if (a->indent != b->indent)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_INDENT);
  if (a->italic != b->italic)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_ITALIC);
  if (a->kerning != b->kerning)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_KERNING);
  if (a->leading != b->leading)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_LEADING);
  if (a->left_margin != b->left_margin)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_LEFT_MARGIN);
  if (a->letter_spacing != b->letter_spacing)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_LETTER_SPACING);
  if (a->right_margin != b->right_margin)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_RIGHT_MARGIN);
  if (a->size != b->size)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_SIZE);
  if (a->n_tab_stops != b->n_tab_stops)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS);
  else if (a->n_tab_stops != 0 && 
      memcmp (a->tab_stops, b->tab_stops, sizeof (guint) & a->n_tab_stops) != 0)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_TAB_STOPS);
  if (a->target != b->target)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_TARGET);
  if (a->underline != b->underline)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_UNDERLINE);
  if (a->url != b->url)
    SWFDEC_TEXT_ATTRIBUTE_SET (result, SWFDEC_TEXT_ATTRIBUTE_URL);

  return result;
}
