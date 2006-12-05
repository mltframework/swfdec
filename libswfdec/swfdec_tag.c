/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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

#include <zlib.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "swfdec_tag.h"
#include "swfdec_bits.h"
#include "swfdec_button.h"
#include "swfdec_compiler.h"
#include "swfdec_debug.h"
#include "swfdec_edittext.h"
#include "swfdec_font.h"
#include "swfdec_image.h"
#include "swfdec_morphshape.h"
#include "swfdec_movie.h" /* for SwfdecContent */
#include "swfdec_pattern.h"
#include "swfdec_shape.h"
#include "swfdec_sound.h"
#include "swfdec_sprite.h"
#include "swfdec_text.h"

int
tag_func_end (SwfdecSwfDecoder * s)
{
  return SWFDEC_STATUS_OK;
}

int
tag_func_protect (SwfdecSwfDecoder * s)
{
  if (s->protection) {
    SWFDEC_INFO ("This file is really protected.");
    g_free (s->password);
    s->password = NULL;
  }
  s->protection = TRUE;
  if (swfdec_bits_left (&s->b)) {
    /* FIXME: What's this for? */
    swfdec_bits_get_u16 (&s->b);
    s->password = swfdec_bits_get_string (&s->b);
  }
  return SWFDEC_STATUS_OK;
}

int
tag_func_frame_label (SwfdecSwfDecoder * s)
{
  SwfdecSpriteFrame *frame = &s->parse_sprite->frames[s->parse_sprite->parse_frame];
  
  if (frame->label) {
    SWFDEC_WARNING ("frame %d already has a label (%s)", s->parse_sprite->parse_frame, frame->label);
    g_free (frame->label);
  }
  frame->label = swfdec_bits_get_string (&s->b);
  SWFDEC_LOG ("frame %d named %s", s->parse_sprite->parse_frame, frame->label);

  return SWFDEC_STATUS_OK;
}


/* text */

static int
define_text (SwfdecSwfDecoder * s, int rgba)
{
  SwfdecBits *bits = &s->b;
  int id;
  int n_glyph_bits;
  int n_advance_bits;
  SwfdecText *text = NULL;
  SwfdecTextGlyph glyph = { 0 };

  id = swfdec_bits_get_u16 (bits);
  text = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_TEXT);
  if (!text)
    return SWFDEC_STATUS_OK;

  glyph.color = 0xffffffff;

  swfdec_bits_get_rect (bits, &SWFDEC_GRAPHIC (text)->extents);
  swfdec_bits_get_matrix (bits, &text->transform);
  swfdec_bits_syncbits (bits);
  n_glyph_bits = swfdec_bits_get_u8 (bits);
  n_advance_bits = swfdec_bits_get_u8 (bits);

  //printf("  n_glyph_bits = %d\n", n_glyph_bits);
  //printf("  n_advance_bits = %d\n", n_advance_bits);

  while (swfdec_bits_peekbits (bits, 8) != 0) {
    int type;

    type = swfdec_bits_getbit (bits);
    if (type == 0) {
      /* glyph record */
      int n_glyphs;
      int i;

      n_glyphs = swfdec_bits_getbits (bits, 7);
      if (glyph.font == NULL)
	SWFDEC_ERROR ("no font for %d glyphs", n_glyphs);
      for (i = 0; i < n_glyphs; i++) {
        glyph.glyph = swfdec_bits_getbits (bits, n_glyph_bits);

	if (glyph.font != NULL)
	  g_array_append_val (text->glyphs, glyph);
        glyph.x += swfdec_bits_getsbits (bits, n_advance_bits);
      }
    } else {
      /* state change */
      int reserved;
      int has_font;
      int has_color;
      int has_y_offset;
      int has_x_offset;

      reserved = swfdec_bits_getbits (bits, 3);
      has_font = swfdec_bits_getbit (bits);
      has_color = swfdec_bits_getbit (bits);
      has_y_offset = swfdec_bits_getbit (bits);
      has_x_offset = swfdec_bits_getbit (bits);
      if (has_font) {
        glyph.font = swfdec_swf_decoder_get_character (s, swfdec_bits_get_u16 (bits));
        //printf("  font = %d\n",font);
      }
      if (has_color) {
        if (rgba) {
          glyph.color = swfdec_bits_get_rgba (bits);
        } else {
          glyph.color = swfdec_bits_get_color (bits);
        }
        //printf("  color = %08x\n",glyph.color);
      }
      if (has_x_offset) {
        glyph.x = swfdec_bits_get_s16 (bits);
      }
      if (has_y_offset) {
        glyph.y = swfdec_bits_get_s16 (bits);
      }
      if (has_font) {
        glyph.height = swfdec_bits_get_u16 (bits);
      }
    }
    swfdec_bits_syncbits (bits);
  }
  swfdec_bits_get_u8 (bits);

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_text (SwfdecSwfDecoder * s)
{
  return define_text (s, 0);
}

int
tag_func_define_text_2 (SwfdecSwfDecoder * s)
{
  return define_text (s, 1);
}



int
tag_func_define_sprite (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecBits parse;
  int id;
  SwfdecSprite *sprite;
  int ret;
  SwfdecBits save_bits;

  save_bits = s->b;

  id = swfdec_bits_get_u16 (bits);
  sprite = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_SPRITE);
  if (!sprite)
    return SWFDEC_STATUS_OK;

  SWFDEC_LOG ("  ID: %d", id);

  swfdec_sprite_set_n_frames (sprite, swfdec_bits_get_u16 (bits), SWFDEC_DECODER (s)->rate);

  parse = *bits;

  s->parse_sprite = sprite;
  while (1) {
    unsigned char *endptr;
    int x;
    int tag;
    int tag_len;
    SwfdecBuffer *buffer;
    SwfdecTagFunc *func;

    //SWFDEC_INFO ("sprite parsing at %d", parse.ptr - parse.buffer->data);
    x = swfdec_bits_get_u16 (&parse);
    tag = (x >> 6) & 0x3ff;
    tag_len = x & 0x3f;
    if (tag_len == 0x3f) {
      tag_len = swfdec_bits_get_u32 (&parse);
    }
    SWFDEC_INFO ("sprite parsing at %d, tag %d %s, length %d",
        parse.ptr - parse.buffer->data, tag,
        swfdec_swf_decoder_get_tag_name (tag), tag_len);
    //SWFDEC_DEBUG ("tag %d %s", tag, swfdec_decoder_get_tag_name (tag));

    if (tag_len > 0) {
      buffer = swfdec_buffer_new_subbuffer (parse.buffer,
          parse.ptr - parse.buffer->data, tag_len);
      s->b.buffer = buffer;
      s->b.ptr = buffer->data;
      s->b.idx = 0;
      s->b.end = buffer->data + buffer->length;
    } else {
      buffer = NULL;
      s->b.buffer = NULL;
      s->b.ptr = NULL;
      s->b.idx = 0;
      s->b.end = NULL;
    }

    func = swfdec_swf_decoder_get_tag_func (tag);
    if (func == NULL) {
      SWFDEC_WARNING ("tag function not implemented for %d %s",
          tag, swfdec_swf_decoder_get_tag_name (tag));
    } else if ((swfdec_swf_decoder_get_tag_flag (tag) & 1) == 0) {
      SWFDEC_ERROR ("invalid tag %d %s during DefineSprite",
          tag, swfdec_swf_decoder_get_tag_name (tag));
    } else {
      endptr = parse.ptr + tag_len;
      ret = func (s);

      swfdec_bits_syncbits (bits);
      if (tag_len > 0) {
        if (s->b.ptr < endptr) {
          SWFDEC_WARNING ("early parse finish (%d bytes)", endptr - s->b.ptr);
        }
        if (s->b.ptr > endptr) {
          SWFDEC_WARNING ("parse overrun (%d bytes)", s->b.ptr - endptr);
        }
      }

      parse.ptr = endptr;
    }

    if (buffer)
      swfdec_buffer_unref (buffer);

    if (tag == 0)
      break;
  }

  s->b = save_bits;
  s->b.ptr += s->b.buffer->length;
  /* this assumes that no recursive DefineSprite happens and the spec says it doesn't */
  s->parse_sprite = s->main_sprite;
  SWFDEC_LOG ("done parsing this sprite");

  return SWFDEC_STATUS_OK;
}

int
tag_func_do_action (SwfdecSwfDecoder * s)
{
  JSScript *script;
  char *name;

  name = g_strdup_printf ("Sprite%u.Frame%u", SWFDEC_CHARACTER (s->parse_sprite)->id,
      s->parse_sprite->parse_frame);
  script = swfdec_compile (SWFDEC_DECODER (s)->player, &s->b, s->version, name);
  g_free (name);
  if (script)
    swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, 
	SWFDEC_SPRITE_ACTION_SCRIPT, script);

  return SWFDEC_STATUS_OK;
}

int
tag_func_do_init_action (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int len;
  SwfdecBuffer *buffer;
  SwfdecCharacter *character;
  unsigned char *endptr;
  //int retcode = SWF_ERROR;

  endptr = bits->ptr + bits->buffer->length;

  character = swfdec_swf_decoder_get_character (s, swfdec_bits_get_u16 (bits));

  len = bits->end - bits->ptr;

  buffer = swfdec_buffer_new_subbuffer (bits->buffer,
    bits->ptr - bits->buffer->data, len);

  bits->ptr += len;

  if (SWFDEC_IS_SPRITE (character)) {
    SWFDEC_WARNING ("init actions not implemented yet");
#if 0
    SwfdecSprite *save_parse_sprite = s->parse_sprite;
    s->parse_sprite = SWFDEC_SPRITE(obj);
    retcode = swfdec_action_script_execute (s, buffer);
    s->parse_sprite = save_parse_sprite;
#endif
  }
  swfdec_buffer_unref (buffer);

  //return retcode;
  return SWFDEC_STATUS_OK;
}

static void
swfdec_button_append_content (SwfdecButton *button, guint states, SwfdecContent *content)
{
  guint i;
  SwfdecContent *cur = NULL;

  for (i = 0; i < 4; i++) {
    if (!cur && (states & 1)) {
      cur = content;
      if (content->end != G_MAXUINT) {
	/* need a copy here */
	cur = swfdec_content_new (content->depth);
	*cur = *content;
	cur->sequence = cur;
      }
      cur->start = i;
      button->records = g_list_append (button->records, content);
    }
    if (cur && !(states & 1)) {
      cur->end = i;
      cur = NULL;
    }
    states >>= 1;
  }
  if (cur) {
    SwfdecRect rect;
    cur->end = 4;
    swfdec_rect_transform (&rect, &content->graphic->extents, &cur->transform);
    swfdec_rect_union (&SWFDEC_GRAPHIC (button)->extents,
	&SWFDEC_GRAPHIC (button)->extents, &rect);
  }
  if (content->end == G_MAXUINT) {
    SWFDEC_ERROR ("button record for graphic %u is not used in any state, discarding", 
	SWFDEC_CHARACTER (content->graphic)->id);
    swfdec_content_free (content);
  }
}

int
tag_func_define_button_2 (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  int flags;
  int offset;
  SwfdecButton *button;
  unsigned char *endptr;
  char *script_name;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  button = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_BUTTON);
  if (!button)
    return SWFDEC_STATUS_OK;

  SWFDEC_LOG ("  ID: %d", id);

  flags = swfdec_bits_get_u8 (bits);
  if (flags & 0x01)
    button->menubutton = TRUE;
  offset = swfdec_bits_get_u16 (bits);

  SWFDEC_LOG ("  flags = %d", flags);
  SWFDEC_LOG ("  offset = %d", offset);

  while (swfdec_bits_peek_u8 (bits)) {
    int reserved;
    unsigned int character;
    unsigned int depth;
    unsigned int states;
    SwfdecContent *content;

    swfdec_bits_syncbits (bits);
    reserved = swfdec_bits_getbits (bits, 4);
    states = swfdec_bits_getbits (bits, 4);
    character = swfdec_bits_get_u16 (bits);
    depth = swfdec_bits_get_u16 (bits);

    SWFDEC_LOG ("  reserved = %d", reserved);
    SWFDEC_LOG ("states: %s%s%s%scharacter=%u layer=%u",
        states & (1 << SWFDEC_BUTTON_HIT) ? "HIT " : "", 
	states & (1 << SWFDEC_BUTTON_DOWN) ? "DOWN " : "", 
        states & (1 << SWFDEC_BUTTON_OVER) ? "OVER " : "",
	states & (1 << SWFDEC_BUTTON_UP) ? "UP " : "", 
	character, depth);
    content = swfdec_content_new (depth);

    swfdec_bits_get_matrix (bits, &content->transform);
    SWFDEC_LOG ("matrix: %g %g  %g %g   %g %g",
	content->transform.xx, content->transform.yy, 
	content->transform.xy, content->transform.yx,
	content->transform.x0, content->transform.y0);
    swfdec_bits_get_color_transform (bits, &content->color_transform);

    content->graphic = swfdec_swf_decoder_get_character (s, character);
    if (!SWFDEC_IS_GRAPHIC (content->graphic)) {
      SWFDEC_ERROR ("id %u does not reference a graphic, ignoring", character);
      swfdec_content_free (content);
    } else {
      swfdec_button_append_content (button, states, content);
    }
  }
  swfdec_bits_get_u8 (bits);

  script_name = g_strdup_printf ("Button%u", SWFDEC_CHARACTER (button)->id);
  while (offset != 0) {
    guint condition, key;

    offset = swfdec_bits_get_u16 (bits);
    condition = swfdec_bits_get_u16 (bits);
    key = condition >> 9;
    condition &= 0x1FF;

    SWFDEC_LOG ("  offset = %d", offset);

    if (button->events == NULL)
      button->events = swfdec_event_list_new (SWFDEC_DECODER (s)->player);
    SWFDEC_LOG ("new event for condition %u (key %u)", condition, key);
    swfdec_event_list_parse (button->events, &s->b, s->version, condition, key,
	script_name);
  }
  g_free (script_name);

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_button (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  SwfdecButton *button;
  unsigned char *endptr;
  char *script_name;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  button = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_BUTTON);
  if (!button)
    return SWFDEC_STATUS_OK;

  SWFDEC_LOG ("  ID: %d", id);

  while (swfdec_bits_peek_u8 (bits)) {
    int reserved;
    unsigned int character;
    unsigned int depth;
    unsigned int states;
    SwfdecContent *content;

    swfdec_bits_syncbits (bits);
    reserved = swfdec_bits_getbits (bits, 4);
    states = swfdec_bits_getbits (bits, 4);
    character = swfdec_bits_get_u16 (bits);
    depth = swfdec_bits_get_u16 (bits);

    SWFDEC_LOG ("  reserved = %d", reserved);
    SWFDEC_LOG ("states: %s%s%s%scharacter=%u layer=%u",
        states & (1 << SWFDEC_BUTTON_HIT) ? "HIT " : "", 
	states & (1 << SWFDEC_BUTTON_DOWN) ? "DOWN " : "", 
        states & (1 << SWFDEC_BUTTON_OVER) ? "OVER " : "",
	states & (1 << SWFDEC_BUTTON_UP) ? "UP " : "", 
	character, depth);
    content = swfdec_content_new (depth);

    swfdec_bits_get_matrix (bits, &content->transform);
    SWFDEC_LOG ("matrix: %g %g  %g %g   %g %g",
	content->transform.xx, content->transform.yy, 
	content->transform.xy, content->transform.yx,
	content->transform.x0, content->transform.y0);

    content->graphic = swfdec_swf_decoder_get_character (s, character);
    if (!SWFDEC_IS_GRAPHIC (content->graphic)) {
      SWFDEC_ERROR ("id %u does not reference a graphic, ignoring", character);
      swfdec_content_free (content);
    } else {
      swfdec_button_append_content (button, states, content);
    }
  }
  swfdec_bits_get_u8 (bits);

  script_name = g_strdup_printf ("Button%u", SWFDEC_CHARACTER (button)->id);
  button->events = swfdec_event_list_new (SWFDEC_DECODER (s)->player);
  swfdec_event_list_parse (button->events, &s->b, s->version, 
      SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN, 0, script_name);
  g_free (script_name);

  return SWFDEC_STATUS_OK;
}

int
tag_func_define_font (SwfdecSwfDecoder * s)
{
  int id;
  int i;
  int n_glyphs;
  int offset;
  SwfdecShape *shape;
  SwfdecFont *font;

  id = swfdec_bits_get_u16 (&s->b);
  font = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_FONT);
  if (!font)
    return SWFDEC_STATUS_OK;

  offset = swfdec_bits_get_u16 (&s->b);
  n_glyphs = offset / 2;

  for (i = 1; i < n_glyphs; i++) {
    offset = swfdec_bits_get_u16 (&s->b);
  }

  g_array_set_size (font->glyphs, n_glyphs);
  for (i = 0; i < n_glyphs; i++) {
    SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
    shape = g_object_new (SWFDEC_TYPE_SHAPE, NULL);
    entry->shape = shape;

    g_ptr_array_add (shape->fills, swfdec_pattern_new_color (0xFFFFFFFF));
    g_ptr_array_add (shape->lines, swfdec_pattern_new_stroke (20, 0xFFFFFFFF));

    swfdec_bits_syncbits (&s->b);
    shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_fill_bits = %d", shape->n_fill_bits);
    shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_line_bits = %d", shape->n_line_bits);

    swfdec_shape_get_recs (s, shape);
  }

  return SWFDEC_STATUS_OK;
}

static void
get_kerning_record (SwfdecBits * bits, int wide_codes)
{
  if (wide_codes) {
    swfdec_bits_get_u16 (bits);
    swfdec_bits_get_u16 (bits);
  } else {
    swfdec_bits_get_u8 (bits);
    swfdec_bits_get_u8 (bits);
  }
  swfdec_bits_get_s16 (bits);
}

int
tag_func_define_font_2 (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  SwfdecShape *shape;
  SwfdecFont *font;
  SwfdecRect rect;

  int has_layout;
  int shift_jis;
  int reserved;
  int ansi;
  int wide_offsets;
  int wide_codes;
  int italic;
  int bold;
  int langcode;
  int font_name_len;
  int n_glyphs;
  int code_table_offset;
  int font_ascent;
  int font_descent;
  int font_leading;
  int kerning_count;
  int i;

  id = swfdec_bits_get_u16 (bits);
  font = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_FONT);
  if (!font)
    return SWFDEC_STATUS_OK;

  has_layout = swfdec_bits_getbit (bits);
  shift_jis = swfdec_bits_getbit (bits);
  reserved = swfdec_bits_getbit (bits);
  ansi = swfdec_bits_getbit (bits);
  wide_offsets = swfdec_bits_getbit (bits);
  wide_codes = swfdec_bits_getbit (bits);
  italic = swfdec_bits_getbit (bits);
  bold = swfdec_bits_getbit (bits);

  langcode = swfdec_bits_get_u8 (bits);
  SWFDEC_DEBUG("langcode %d", langcode);

  font_name_len = swfdec_bits_get_u8 (bits);
  //font_name = 
  bits->ptr += font_name_len;

  n_glyphs = swfdec_bits_get_u16 (bits);
  if (wide_offsets) {
    bits->ptr += 4 * n_glyphs;
    code_table_offset = swfdec_bits_get_u32 (bits);
  } else {
    bits->ptr += 2 * n_glyphs;
    code_table_offset = swfdec_bits_get_u16 (bits);
  }

  g_array_set_size (font->glyphs, n_glyphs);

  for (i = 0; i < n_glyphs; i++) {
    SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
    shape = g_object_new (SWFDEC_TYPE_SHAPE, NULL);
    entry->shape = shape;

    g_ptr_array_add (shape->fills, swfdec_pattern_new_color (0xFFFFFFFF));
    g_ptr_array_add (shape->lines, swfdec_pattern_new_stroke (20, 0xFFFFFFFF));

    swfdec_bits_syncbits (&s->b);
    shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_fill_bits = %d", shape->n_fill_bits);
    shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_line_bits = %d", shape->n_line_bits);

    swfdec_shape_get_recs (s, shape);
  }
  if (wide_codes) {
    bits->ptr += 2 * n_glyphs;
  } else {
    bits->ptr += 1 * n_glyphs;
  }
  if (has_layout) {
    font_ascent = swfdec_bits_get_s16 (bits);
    font_descent = swfdec_bits_get_s16 (bits);
    font_leading = swfdec_bits_get_s16 (bits);
    //font_advance_table = swfdec_bits_get_s16(bits);
    bits->ptr += 2 * n_glyphs;
    for (i = 0; i < n_glyphs; i++) {
      swfdec_bits_get_rect (bits, &rect);
    }
    kerning_count = swfdec_bits_get_u16 (bits);
    if (0) {
      for (i = 0; i < kerning_count; i++) {
        get_kerning_record (bits, wide_codes);
      }
    }
  }

  return SWFDEC_STATUS_OK;
}

static int
tag_func_export_assets (SwfdecSwfDecoder * s)
{
  SwfdecBits *bits = &s->b;
  unsigned int count, i;

  count = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("exporting %u assets", count);
  for (i = 0; i < count; i++) {
    guint id;
    SwfdecCharacter *object;
    char *name;
    id = swfdec_bits_get_u16 (bits);
    object = swfdec_swf_decoder_get_character (s, id);
    name = swfdec_bits_get_string (bits);
    if (object == NULL) {
      SWFDEC_ERROR ("cannot export id %u as %s, id wasn't found", id, name);
      g_free (name);
    } else {
      SWFDEC_LOG ("exporting %s %u as %s", G_OBJECT_TYPE_NAME (object), id, name);
      g_object_ref (object);
      g_hash_table_insert (s->exports, name, object);
    }
  }

  return SWFDEC_STATUS_OK;
}

static int
tag_func_define_font_info_1 (SwfdecSwfDecoder *s)
{
  return tag_func_define_font_info (s, 1);
}

static int
tag_func_define_font_info_2 (SwfdecSwfDecoder *s)
{
  return tag_func_define_font_info (s, 2);
}

static int
tag_func_file_attributes (SwfdecSwfDecoder *s)
{
  int has_metadata, use_network;

  if (swfdec_bits_getbits (&s->b, 3))
    SWFDEC_INFO ("reserved bits (1) aren't 0");
  has_metadata = swfdec_bits_getbit (&s->b);
  if (swfdec_bits_getbits (&s->b, 3))
    SWFDEC_INFO ("reserved bits (2) aren't 0");
  use_network = swfdec_bits_getbit (&s->b);
  /* FIXME: do something useful with this data */
  if (swfdec_bits_getbits (&s->b, 24))
    SWFDEC_INFO ("reserved bits (3) aren't 0");

  return SWFDEC_STATUS_OK;
}

/* may appear inside DefineSprite */
#define SPRITE 1
struct tag_func_struct
{
  char *name;
  int (*func) (SwfdecSwfDecoder * s);
  int flag;
};
static struct tag_func_struct tag_funcs[] = {
  [SWFDEC_TAG_END] = {"End", tag_func_end, SPRITE},
  [SWFDEC_TAG_SHOWFRAME] = {"ShowFrame", tag_show_frame, SPRITE},
  [SWFDEC_TAG_DEFINESHAPE] = {"DefineShape", tag_define_shape, 0},
  [SWFDEC_TAG_FREECHARACTER] = {"FreeCharacter", NULL, 0},
  [SWFDEC_TAG_PLACEOBJECT] = {"PlaceObject", NULL, SPRITE},
  [SWFDEC_TAG_REMOVEOBJECT] = {"RemoveObject", swfdec_spriteseg_remove_object, SPRITE},
  [SWFDEC_TAG_DEFINEBITSJPEG] = {"DefineBitsJPEG", tag_func_define_bits_jpeg, 0},
  [SWFDEC_TAG_DEFINEBUTTON] = {"DefineButton", tag_func_define_button, 0},
  [SWFDEC_TAG_JPEGTABLES] = {"JPEGTables", swfdec_image_jpegtables, 0},
  [SWFDEC_TAG_SETBACKGROUNDCOLOR] =
      {"SetBackgroundColor", tag_func_set_background_color, 0},
  [SWFDEC_TAG_DEFINEFONT] = {"DefineFont", tag_func_define_font, 0},
  [SWFDEC_TAG_DEFINETEXT] = {"DefineText", tag_func_define_text, 0},
  [SWFDEC_TAG_DOACTION] = {"DoAction", tag_func_do_action, SPRITE},
  [SWFDEC_TAG_DEFINEFONTINFO] = {"DefineFontInfo", tag_func_define_font_info_1, 0},
  [SWFDEC_TAG_DEFINESOUND] = {"DefineSound", tag_func_define_sound, 0},
  [SWFDEC_TAG_STARTSOUND] = {"StartSound", tag_func_start_sound, SPRITE},
  [SWFDEC_TAG_DEFINEBUTTONSOUND] =
      {"DefineButtonSound", tag_func_define_button_sound, 0},
  [SWFDEC_TAG_SOUNDSTREAMHEAD] = {"SoundStreamHead", tag_func_sound_stream_head, SPRITE},
  [SWFDEC_TAG_SOUNDSTREAMBLOCK] = {"SoundStreamBlock", tag_func_sound_stream_block, SPRITE},
  [SWFDEC_TAG_DEFINEBITSLOSSLESS] =
      {"DefineBitsLossless", tag_func_define_bits_lossless, 0},
  [SWFDEC_TAG_DEFINEBITSJPEG2] = {"DefineBitsJPEG2", tag_func_define_bits_jpeg_2, 0},
  [SWFDEC_TAG_DEFINESHAPE2] = {"DefineShape2", tag_define_shape_2, 0},
  [SWFDEC_TAG_DEFINEBUTTONCXFORM] = {"DefineButtonCXForm", NULL, 0},
  [SWFDEC_TAG_PROTECT] = {"Protect", tag_func_protect, 0},
  [SWFDEC_TAG_PLACEOBJECT2] = {"PlaceObject2", swfdec_spriteseg_place_object_2, SPRITE},
  [SWFDEC_TAG_REMOVEOBJECT2] = {"RemoveObject2", swfdec_spriteseg_remove_object_2, SPRITE},
  [SWFDEC_TAG_DEFINESHAPE3] = {"DefineShape3", tag_define_shape_3, 0},
  [SWFDEC_TAG_DEFINETEXT2] = {"DefineText2", tag_func_define_text_2, 0},
  [SWFDEC_TAG_DEFINEBUTTON2] = {"DefineButton2", tag_func_define_button_2, 0},
  [SWFDEC_TAG_DEFINEBITSJPEG3] = {"DefineBitsJPEG3", tag_func_define_bits_jpeg_3, 0},
  [SWFDEC_TAG_DEFINEBITSLOSSLESS2] =
      {"DefineBitsLossless2", tag_func_define_bits_lossless_2, 0},
  [SWFDEC_TAG_DEFINEEDITTEXT] = {"DefineEditText", tag_func_define_edit_text, 0},
  [SWFDEC_TAG_DEFINEMOVIE] = {"DefineMovie", NULL, 0},
  [SWFDEC_TAG_DEFINESPRITE] = {"DefineSprite", tag_func_define_sprite, 0},
  [SWFDEC_TAG_NAMECHARACTER] = {"NameCharacter", NULL, 0},
  [SWFDEC_TAG_SERIALNUMBER] = {"SerialNumber", NULL, 0},
  [SWFDEC_TAG_GENERATORTEXT] = {"GeneratorText", NULL, 0},
  [SWFDEC_TAG_FRAMELABEL] = {"FrameLabel", tag_func_frame_label, SPRITE},
  [SWFDEC_TAG_SOUNDSTREAMHEAD2] = {"SoundStreamHead2", tag_func_sound_stream_head, SPRITE},
  [SWFDEC_TAG_DEFINEMORPHSHAPE] =
      {"DefineMorphShape", tag_define_morph_shape, 0},
  [SWFDEC_TAG_DEFINEFONT2] = {"DefineFont2", tag_func_define_font_2, 0},
  [SWFDEC_TAG_TEMPLATECOMMAND] = {"TemplateCommand", NULL, 0},
  [SWFDEC_TAG_GENERATOR3] = {"Generator3", NULL, 0},
  [SWFDEC_TAG_EXTERNALFONT] = {"ExternalFont", NULL, 0},
  [SWFDEC_TAG_EXPORTASSETS] = {"ExportAssets", tag_func_export_assets, 0},
  [SWFDEC_TAG_IMPORTASSETS] = {"ImportAssets", NULL, 0},
  [SWFDEC_TAG_ENABLEDEBUGGER] = {"EnableDebugger", NULL, 0},
  [SWFDEC_TAG_DOINITACTION] = {"DoInitAction", tag_func_do_init_action, SPRITE},
  [SWFDEC_TAG_DEFINEVIDEOSTREAM] = {"DefineVideoStream", NULL, 0},
  [SWFDEC_TAG_VIDEOFRAME] = {"VideoFrame", NULL, 0},
  [SWFDEC_TAG_DEFINEFONTINFO2] = {"DefineFontInfo2", tag_func_define_font_info_2, 0},
  [SWFDEC_TAG_MX4] = {"MX4", NULL, 0},
  [SWFDEC_TAG_ENABLEDEBUGGER2] = {"EnableDebugger2", NULL, 0},
  [SWFDEC_TAG_SCRIPTLIMITS] = {"ScriptLimits", NULL, 0},
  [SWFDEC_TAG_SETTABINDEX] = {"SetTabIndex", NULL, 0},
  [SWFDEC_TAG_FILEATTRIBUTES] = {"FileAttributes", tag_func_file_attributes, 0},
  [SWFDEC_TAG_PLACEOBJECT3] = {"PlaceObject3", NULL, 0},
  [SWFDEC_TAG_IMPORTASSETS2] = {"ImportAssets2", NULL, 0},
  [SWFDEC_TAG_DEFINEFONTALIGNZONES] = {"DefineFontAlignZones", NULL, 0},
  [SWFDEC_TAG_CSMTEXTSETTINGS] = {"CSMTextSettings", NULL, 0},
  [SWFDEC_TAG_DEFINEFONT3] = {"DefineFont3", NULL, 0},
  [SWFDEC_TAG_AVM2DECL] = {"AVM2Decl", NULL, 0},
  [SWFDEC_TAG_METADATA] = {"Metadata", NULL, 0},
  [SWFDEC_TAG_DEFINESCALINGGRID] = {"DefineScalingGrid", NULL, 0},
  [SWFDEC_TAG_AVM2ACTION] = {"AVM2Action", NULL, 0},
  [SWFDEC_TAG_DEFINESHAPE4] = {"DefineShape4", NULL, 0},
  [SWFDEC_TAG_DEFINEMORPHSHAPE2] = {"DefineMorphShape2", NULL, 0},
};

static const int n_tag_funcs = sizeof (tag_funcs) / sizeof (tag_funcs[0]);

const char *
swfdec_swf_decoder_get_tag_name (int tag)
{
  if (tag >= 0 && tag < n_tag_funcs) {
    if (tag_funcs[tag].name) {
      return tag_funcs[tag].name;
    }
  }
  
  return "unknown";
}

SwfdecTagFunc *
swfdec_swf_decoder_get_tag_func (int tag)
{ 
  if (tag >= 0 && tag < n_tag_funcs) {
    if (tag_funcs[tag].func) {
      return tag_funcs[tag].func;
    }
  }
  
  return NULL;
}

int
swfdec_swf_decoder_get_tag_flag (int tag)
{
  if (tag >= 0 && tag < n_tag_funcs) {
    return tag_funcs[tag].flag;
  }
  
  return 0;
}
