/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_button.h"
#include "swfdec_button_movie.h"
#include "swfdec_debug.h"
#include "swfdec_filter.h"
#include "swfdec_sound.h"
#include "swfdec_sprite.h"


G_DEFINE_TYPE (SwfdecButton, swfdec_button, SWFDEC_TYPE_GRAPHIC)

static void
swfdec_button_init (SwfdecButton * button)
{
}

static void
swfdec_button_dispose (GObject *object)
{
  guint i;
  SwfdecButton *button = SWFDEC_BUTTON (object);

  g_slist_foreach (button->records, (GFunc) swfdec_buffer_unref, NULL);
  g_slist_free (button->records);
  button->records = NULL;
  if (button->events != NULL) {
    swfdec_event_list_free (button->events);
    button->events = NULL;
  }
  for (i = 0; i < 4; i++) {
    if (button->sounds[i]) {
      swfdec_sound_chunk_free (button->sounds[i]);
      button->sounds[i] = NULL;
    }
  }

  G_OBJECT_CLASS (swfdec_button_parent_class)->dispose (G_OBJECT (button));
}

static SwfdecMovie *
swfdec_button_create_movie (SwfdecGraphic *graphic, gsize *size)
{
  SwfdecButton *button = SWFDEC_BUTTON (graphic);
  SwfdecButtonMovie *movie = g_object_new (SWFDEC_TYPE_BUTTON_MOVIE, NULL);

  movie->button = g_object_ref (button);
  *size = sizeof (SwfdecButtonMovie);
  if (button->events)
    SWFDEC_ACTOR (movie)->events = swfdec_event_list_copy (button->events);

  return SWFDEC_MOVIE (movie);
}

static void
swfdec_button_class_init (SwfdecButtonClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);

  object_class->dispose = swfdec_button_dispose;

  graphic_class->create_movie = swfdec_button_create_movie;
}

static guint
swfdec_button_translate_conditions (guint conditions, gboolean menu)
{
  static const guint events[][2] = { 
    /* idle => over up */	{ 1 << SWFDEC_EVENT_ROLL_OVER, 1 << SWFDEC_EVENT_ROLL_OVER },
    /* over up => idle */	{ 1 << SWFDEC_EVENT_ROLL_OUT, 1 << SWFDEC_EVENT_ROLL_OUT },
    /* over up => over down */	{ 1 << SWFDEC_EVENT_PRESS, 1 << SWFDEC_EVENT_PRESS },
    /* over down => over up */	{ 1 << SWFDEC_EVENT_RELEASE, 1 << SWFDEC_EVENT_RELEASE },
    /* over down => out down */	{ 1 << SWFDEC_EVENT_DRAG_OUT, 0 },
    /* out down => over down */	{ 1 << SWFDEC_EVENT_DRAG_OVER, 0 },
    /* out down => idle */	{ 1 << SWFDEC_EVENT_RELEASE_OUTSIDE, 1 << SWFDEC_EVENT_RELEASE_OUTSIDE },
    /* idle => over down */	{ 0, 1 << SWFDEC_EVENT_DRAG_OVER },
    /* over down => idle */	{ 0, 1 << SWFDEC_EVENT_DRAG_OUT }
  };
  guint i, ret;

  ret = 0;
  for (i = 0; i <= G_N_ELEMENTS (events); i++) {
    if (conditions & (1 << i))
      ret |= events[i][menu ? 1 : 0];
  }
  return ret;
}

int
tag_func_define_button_2 (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecBits bits;
  int id, reserved;
  guint length;
  SwfdecButton *button;
  char *script_name;

  id = swfdec_bits_get_u16 (&s->b);
  button = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_BUTTON);
  if (!button)
    return SWFDEC_STATUS_OK;

  SWFDEC_LOG ("  ID: %d", id);

  reserved = swfdec_bits_getbits (&s->b, 7);
  button->menubutton = swfdec_bits_getbit (&s->b) ? TRUE : FALSE;
  length = swfdec_bits_get_u16 (&s->b);

  SWFDEC_LOG ("  reserved = %d", reserved);
  SWFDEC_LOG ("  menu = %d", button->menubutton);
  SWFDEC_LOG ("  length of region = %d", length);

  if (length)
    swfdec_bits_init_bits (&bits, &s->b, length > 2 ? length - 2 : 0);
  else
    swfdec_bits_init_bits (&bits, &s->b, swfdec_bits_left (&s->b) / 8);
  while (swfdec_bits_peek_u8 (&bits)) {
    SwfdecBits tmp;
    SwfdecBuffer *buffer;
    cairo_matrix_t trans;
    SwfdecColorTransform ctrans;
    guint states, gid;
    gboolean has_blend_mode, has_filters;

    /* we parse the placement info into buffers each containing one palcement */
    tmp = bits;

    if (s->version >= 8) {
      reserved = swfdec_bits_getbits (&bits, 2);
      has_blend_mode = swfdec_bits_getbit (&bits);
      has_filters = swfdec_bits_getbit (&bits);
      SWFDEC_LOG ("  reserved = %d", reserved);
      SWFDEC_LOG ("  has_blend_mode = %d", has_blend_mode);
      SWFDEC_LOG ("  has_filters = %d", has_filters);
    } else {
      reserved = swfdec_bits_getbits (&bits, 4);
      has_blend_mode = 0;
      has_filters = 0;
      SWFDEC_LOG ("  reserved = %d", reserved);
    }
    states = swfdec_bits_getbits (&bits, 4);
    gid = swfdec_bits_get_u16 (&bits);
    swfdec_bits_get_u16 (&bits);

    SWFDEC_LOG ("  states: %s%s%s%s",
        states & (1 << SWFDEC_BUTTON_HIT) ? "HIT " : "", 
	states & (1 << SWFDEC_BUTTON_DOWN) ? "DOWN " : "", 
        states & (1 << SWFDEC_BUTTON_OVER) ? "OVER " : "",
	states & (1 << SWFDEC_BUTTON_UP) ? "UP " : "");

    swfdec_bits_get_matrix (&bits, &trans, NULL);
    SWFDEC_LOG ("matrix: %g %g  %g %g   %g %g",
	trans.xx, trans.yy, 
	trans.xy, trans.yx,
	trans.x0, trans.y0);
    swfdec_bits_get_color_transform (&bits, &ctrans);

    if (has_filters) {
      GSList *list = swfdec_filter_parse (&bits);
      g_slist_free (list);
    }
    if (has_blend_mode) {
      guint blend_mode = swfdec_bits_get_u8 (&bits);
      SWFDEC_LOG ("  blend mode = %u", blend_mode);
    }
    buffer = swfdec_bits_get_buffer (&tmp, (swfdec_bits_left (&tmp) - swfdec_bits_left (&bits)) / 8);
    if (buffer == NULL)
      break;
    button->records = g_slist_prepend (button->records, buffer);
    /* add hit state movies to button's extents */
    if (states & (1 << SWFDEC_BUTTON_HIT)) {
      SwfdecGraphic *graphic = swfdec_swf_decoder_get_character (s, gid);
      if (SWFDEC_IS_GRAPHIC (graphic)) {
	SwfdecRect rect;
	swfdec_rect_transform (&rect, &graphic->extents, &trans);
	swfdec_rect_union (&SWFDEC_GRAPHIC (button)->extents, &SWFDEC_GRAPHIC (button)->extents, &rect);
      } else {
	SWFDEC_ERROR ("graphic for id %u not found", gid);
      }
    }
  }
  swfdec_bits_get_u8 (&bits);
  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("%u bytes left when parsing button records", swfdec_bits_left (&bits) / 8);
  }
  button->records = g_slist_reverse (button->records);

  script_name = g_strdup_printf ("Button%u", SWFDEC_CHARACTER (button)->id);
  while (length != 0) {
    guint condition, key;

    length = swfdec_bits_get_u16 (&s->b);
    if (length)
      swfdec_bits_init_bits (&bits, &s->b, length > 2 ? length - 2 : 0);
    else
      swfdec_bits_init_bits (&bits, &s->b, swfdec_bits_left (&s->b) / 8);
    condition = swfdec_bits_get_u16 (&bits);
    key = condition >> 9;
    condition &= 0x1FF;
    condition = swfdec_button_translate_conditions (condition, button->menubutton);
    if (key)
      condition |= 1 << SWFDEC_EVENT_KEY_DOWN;

    SWFDEC_LOG (" length = %d", length);

    if (button->events == NULL)
      button->events = swfdec_event_list_new ();
    SWFDEC_LOG ("  new event for condition %u (key %u)", condition, key);
    swfdec_event_list_parse (button->events, &bits, s->version, condition, key,
	script_name);
    if (swfdec_bits_left (&bits)) {
      SWFDEC_WARNING ("%u bytes left after parsing script", swfdec_bits_left (&bits) / 8);
    }
  }
  g_free (script_name);

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_button (SwfdecSwfDecoder * s, guint tag)
{
  guint id, flags;
  SwfdecButton *button;

  id = swfdec_bits_get_u16 (&s->b);
  button = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_BUTTON);
  if (!button)
    return SWFDEC_STATUS_OK;

  SWFDEC_LOG ("  ID: %d", id);

  while (swfdec_bits_peek_u8 (&s->b)) {
    SwfdecBits tmp;
    SwfdecBuffer *buffer;
    cairo_matrix_t matrix;
    guint gid;

    tmp = s->b;
    flags = swfdec_bits_get_u8 (&tmp);
    gid = swfdec_bits_get_u16 (&tmp);
    swfdec_bits_get_u16 (&tmp);
    swfdec_bits_get_matrix (&tmp, &matrix, NULL);
    buffer = swfdec_bits_get_buffer (&s->b, (swfdec_bits_left (&s->b) - swfdec_bits_left (&tmp)) / 8);
    if (buffer == NULL)
      break;
    button->records = g_slist_prepend (button->records, buffer);
    /* add hit state movies to button's extents */
    if (flags & (1 << SWFDEC_BUTTON_HIT)) {
      SwfdecGraphic *graphic = swfdec_swf_decoder_get_character (s, gid);
      if (SWFDEC_IS_GRAPHIC (graphic)) {
	SwfdecRect rect;
	swfdec_rect_transform (&rect, &graphic->extents, &matrix);
	swfdec_rect_union (&SWFDEC_GRAPHIC (button)->extents, &SWFDEC_GRAPHIC (button)->extents, &rect);
      } else {
	SWFDEC_ERROR ("graphic for id %u not found", gid);
      }
    }
  }
  
  swfdec_bits_get_u8 (&s->b);
  button->records = g_slist_reverse (button->records);

  if (swfdec_bits_peek_u8 (&s->b)) {
    char *script_name = g_strdup_printf ("Button%u", id);
    button->events = swfdec_event_list_new ();
    SWFDEC_LOG ("  event for button press");
    swfdec_event_list_parse (button->events, &s->b, s->version, 1 << SWFDEC_EVENT_RELEASE, 
	0, script_name);
    g_free (script_name);
  } else {
    swfdec_bits_get_u8 (&s->b);
  }
  if (swfdec_bits_left (&s->b)) {
    SWFDEC_WARNING ("%u bytes left after parsing script", swfdec_bits_left (&s->b) / 8);
  }

  return SWFDEC_STATUS_OK;
}

