
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zlib.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "swfdec_internal.h"
#include "swfdec_compiler.h"
#include "swfdec_edittext.h"
#include "swfdec_pattern.h"

int
tag_func_end (SwfdecDecoder * s)
{
  return SWFDEC_OK;
}

int
tag_func_protect (SwfdecDecoder * s)
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
  return SWFDEC_OK;
}

int
tag_func_dumpbits (SwfdecDecoder * s)
{
  SwfdecBits *b = &s->b;

  SWFDEC_DEBUG ("%02x %02x %02x %02x %02x %02x %02x %02x",
      swfdec_bits_get_u8 (b), swfdec_bits_get_u8 (b),
      swfdec_bits_get_u8 (b), swfdec_bits_get_u8 (b),
      swfdec_bits_get_u8 (b), swfdec_bits_get_u8 (b),
      swfdec_bits_get_u8 (b), swfdec_bits_get_u8 (b));

  return SWFDEC_OK;
}

int
tag_func_frame_label (SwfdecDecoder * s)
{
  SwfdecSpriteFrame *frame = &s->parse_sprite->frames[s->parse_sprite->parse_frame];
  
  if (frame->name) {
    SWFDEC_WARNING ("frame %d already has a name (%s)", s->parse_sprite->parse_frame, frame->name);
    g_free (frame->name);
  }
  frame->name = swfdec_bits_get_string (&s->b);
  SWFDEC_LOG ("frame %d named %s", s->parse_sprite->parse_frame, frame->name);

  return SWFDEC_OK;
}


/* text */

static int
define_text (SwfdecDecoder * s, int rgba)
{
  SwfdecBits *bits = &s->b;
  int id;
  int n_glyph_bits;
  int n_advance_bits;
  SwfdecText *text = NULL;
  SwfdecTextGlyph glyph = { 0 };

  id = swfdec_bits_get_u16 (bits);
  text = swfdec_object_create (s, id, SWFDEC_TYPE_TEXT);
  if (!text)
    return SWFDEC_OK;

  glyph.color = 0xffffffff;

  swfdec_bits_get_rect (bits, &SWFDEC_OBJECT (text)->extents);
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
        glyph.x += swfdec_bits_getbits (bits, n_advance_bits);
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
        glyph.font = swfdec_object_get (s, swfdec_bits_get_u16 (bits));
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

  return SWFDEC_OK;
}

int
tag_func_define_text (SwfdecDecoder * s)
{
  return define_text (s, 0);
}

int
tag_func_define_text_2 (SwfdecDecoder * s)
{
  return define_text (s, 1);
}



int
tag_func_define_sprite (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecBits parse;
  int id;
  SwfdecSprite *sprite;
  int ret;
  SwfdecBits save_bits;

  save_bits = s->b;

  id = swfdec_bits_get_u16 (bits);
  sprite = swfdec_object_create (s, id, SWFDEC_TYPE_SPRITE);
  if (!sprite)
    return SWFDEC_OK;

  SWFDEC_LOG ("  ID: %d", id);

  swfdec_sprite_set_n_frames (sprite, swfdec_bits_get_u16 (bits));

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
        swfdec_decoder_get_tag_name (tag), tag_len);
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

    func = swfdec_decoder_get_tag_func (tag);
    if (func == NULL) {
      SWFDEC_WARNING ("tag function not implemented for %d %s",
          tag, swfdec_decoder_get_tag_name (tag));
    } else if ((swfdec_decoder_get_tag_flag (tag) & 1) == 0) {
      SWFDEC_ERROR ("invalid tag %d %s during DefineSprite",
          tag, swfdec_decoder_get_tag_name (tag));
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

  return SWFDEC_OK;
}

int
tag_func_do_action (SwfdecDecoder * s)
{
  JSScript *script;
  
  script = swfdec_compile (s);
  if (script)
    swfdec_sprite_add_action (s->parse_sprite, s->parse_sprite->parse_frame, 
	SWFDEC_SPRITE_ACTION_SCRIPT, script);

  return SWFDEC_OK;
}

int
tag_func_do_init_action (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int len;
  SwfdecBuffer *buffer;
  SwfdecObject *obj;
  unsigned char *endptr;
  //int retcode = SWF_ERROR;

  endptr = bits->ptr + bits->buffer->length;

  obj = swfdec_object_get (s, swfdec_bits_get_u16 (bits));

  len = bits->end - bits->ptr;

  buffer = swfdec_buffer_new_subbuffer (bits->buffer,
    bits->ptr - bits->buffer->data, len);

  bits->ptr += len;

  if (SWFDEC_IS_SPRITE(obj)) {
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
  return SWFDEC_OK;
}


int
tag_func_define_button_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  int flags;
  int offset;
  cairo_matrix_t trans;
  SwfdecColorTransform color_trans;
  SwfdecButton *button;
  unsigned char *endptr;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  button = swfdec_object_create (s, id, SWFDEC_TYPE_BUTTON);
  if (!button)
    return SWFDEC_OK;

  SWFDEC_LOG ("  ID: %d", id);

  flags = swfdec_bits_get_u8 (bits);
  if (flags & 0x01)
    button->menubutton = TRUE;
  offset = swfdec_bits_get_u16 (bits);

  SWFDEC_LOG ("  flags = %d", flags);
  SWFDEC_LOG ("  offset = %d", offset);

  while (swfdec_bits_peek_u8 (bits)) {
    int reserved;
    int character;
    int layer;
    SwfdecButtonRecord record = { 0 };

    swfdec_bits_syncbits (bits);
    reserved = swfdec_bits_getbits (bits, 4);
    record.states = swfdec_bits_getbits (bits, 4);
    character = swfdec_bits_get_u16 (bits);
    layer = swfdec_bits_get_u16 (bits);

    SWFDEC_LOG ("  reserved = %d", reserved);
    if (reserved) {
      SWFDEC_WARNING ("reserved is supposed to be 0");
    }
    SWFDEC_LOG ("states: %s%s%s%scharacter=%d layer=%d",
        record.states & SWFDEC_BUTTON_HIT ? "HIT " : "", 
	record.states & SWFDEC_BUTTON_DOWN ? "DOWN " : "", 
        record.states & SWFDEC_BUTTON_OVER ? "OVER " : "",
	record.states & SWFDEC_BUTTON_UP ? "UP " : "", 
	character, layer);

    cairo_matrix_init_identity (&trans);
    swfdec_bits_get_matrix (bits, &trans);
    SWFDEC_LOG ("matrix: %g %g  %g %g   %g %g",
	trans.xx, trans.yy, trans.xy, trans.yx, trans.x0, trans.y0);
    swfdec_bits_syncbits (bits);
    swfdec_bits_get_color_transform (bits, &color_trans);
    swfdec_bits_syncbits (bits);

    record.object = swfdec_object_get (s, character);
    record.transform = trans;
    record.color_transform = color_trans;

    if (record.object) {
      SwfdecRect rect;
      swfdec_rect_transform (&rect, &record.object->extents, &record.transform);
      g_array_append_val (button->records, record);
      swfdec_rect_union (&SWFDEC_OBJECT (button)->extents,
	  &SWFDEC_OBJECT (button)->extents, &rect);
    } else {
      SWFDEC_ERROR ("no object with character %d\n", character);
    }
  }
  swfdec_bits_get_u8 (bits);

  while (offset != 0) {
    guint condition, key;

    offset = swfdec_bits_get_u16 (bits);
    condition = swfdec_bits_get_u16 (bits);
    key = condition >> 9;
    condition &= 0x1FF;

    SWFDEC_LOG ("  offset = %d", offset);

    if (button->events == NULL)
      button->events = swfdec_event_list_new (s);
    swfdec_event_list_parse (button->events, condition, key);
  }

  return SWFDEC_OK;
}

int
tag_func_define_button (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  cairo_matrix_t trans;
  SwfdecColorTransform color_trans;
  SwfdecButton *button;
  unsigned char *endptr;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  button = swfdec_object_create (s, id, SWFDEC_TYPE_BUTTON);
  if (!button)
    return SWFDEC_OK;

  SWFDEC_LOG ("  ID: %d", id);

  while (swfdec_bits_peek_u8 (bits)) {
    int reserved;
    int character;
    int layer;
    SwfdecButtonRecord record = { 0 };

    swfdec_bits_syncbits (bits);
    reserved = swfdec_bits_getbits (bits, 4);
    record.states = swfdec_bits_getbits (bits, 4);
    character = swfdec_bits_get_u16 (bits);
    layer = swfdec_bits_get_u16 (bits);

    SWFDEC_LOG ("  reserved = %d", reserved);
    if (reserved) {
      SWFDEC_WARNING ("reserved is supposed to be 0");
    }
    SWFDEC_LOG ("states: %s%s%s%scharacter=%d layer=%d",
        record.states & SWFDEC_BUTTON_HIT ? "HIT " : "", 
	record.states & SWFDEC_BUTTON_DOWN ? "DOWN " : "", 
        record.states & SWFDEC_BUTTON_OVER ? "OVER " : "",
	record.states & SWFDEC_BUTTON_UP ? "UP " : "", 
	character, layer);

    cairo_matrix_init_identity (&trans);
    swfdec_bits_get_matrix (bits, &trans);
    swfdec_bits_syncbits (bits);
    swfdec_bits_get_color_transform (bits, &color_trans);
    swfdec_bits_syncbits (bits);

    record.object = swfdec_object_get (s, character);
    record.transform = trans;
    record.color_transform = color_trans;

    if (record.object) {
      SwfdecRect rect;
      swfdec_rect_transform (&rect, &record.object->extents, &record.transform);
      g_array_append_val (button->records, record);
      swfdec_rect_union (&SWFDEC_OBJECT (button)->extents,
	  &SWFDEC_OBJECT (button)->extents, &rect);
    } else {
      SWFDEC_ERROR ("no object with character %d\n", character);
    }
  }
  swfdec_bits_get_u8 (bits);

  button->events = swfdec_event_list_new (s);
  swfdec_event_list_parse (button->events, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN, 0);

  return SWFDEC_OK;
}

int
tag_func_define_font (SwfdecDecoder * s)
{
  int id;
  int i;
  int n_glyphs;
  int offset;
  SwfdecShapeVec *shapevec;
  SwfdecShape *shape;
  SwfdecFont *font;

  id = swfdec_bits_get_u16 (&s->b);
  font = swfdec_object_create (s, id, SWFDEC_TYPE_FONT);
  if (!font)
    return SWFDEC_OK;

  offset = swfdec_bits_get_u16 (&s->b);
  n_glyphs = offset / 2;

  for (i = 1; i < n_glyphs; i++) {
    offset = swfdec_bits_get_u16 (&s->b);
  }

  g_array_set_size (font->glyphs, n_glyphs);
  for (i = 0; i < n_glyphs; i++) {
    SwfdecFontEntry *entry = &g_array_index (font->glyphs, SwfdecFontEntry, i);
    shape = swfdec_object_new (s, SWFDEC_TYPE_SHAPE);
    entry->shape = shape;

    shapevec = swf_shape_vec_new ();
    shapevec->pattern = swfdec_pattern_new_color (0xFFFFFFFF);
    g_ptr_array_add (shape->fills, shapevec);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills2, shapevec);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->lines, shapevec);

    //swf_shape_add_styles(s,shape,&s->b);
    swfdec_bits_syncbits (&s->b);
    shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_fill_bits = %d", shape->n_fill_bits);
    shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_line_bits = %d", shape->n_line_bits);

    swf_shape_get_recs (s, &s->b, shape, FALSE);
  }

  return SWFDEC_OK;
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
tag_func_define_font_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  SwfdecShapeVec *shapevec;
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
  font = swfdec_object_create (s, id, SWFDEC_TYPE_FONT);
  if (!font)
    return SWFDEC_OK;

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
    shape = swfdec_object_new (s, SWFDEC_TYPE_SHAPE);
    entry->shape = shape;

    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills, shapevec);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->lines, shapevec);

    //swf_shape_add_styles(s,shape,&s->b);
    swfdec_bits_syncbits (&s->b);
    shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_fill_bits = %d", shape->n_fill_bits);
    shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_line_bits = %d", shape->n_line_bits);

    swf_shape_get_recs (s, &s->b, shape, FALSE);
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

  return SWFDEC_OK;
}

int
tag_func_export_assets (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  SwfdecExport *exp;
  int count, i;

  count = swfdec_bits_get_u16 (bits);
  for (i = 0; i < count; i++) {
    exp = g_malloc (sizeof(SwfdecExport));
    exp->id = swfdec_bits_get_u16 (bits);
    exp->name = swfdec_bits_get_string (bits);
    s->exports = g_list_append (s->exports, exp);
  }

  return SWFDEC_OK;
}

static int
tag_func_define_font_info_1 (SwfdecDecoder *s)
{
  return tag_func_define_font_info (s, 1);
}

static int
tag_func_define_font_info_2 (SwfdecDecoder *s)
{
  return tag_func_define_font_info (s, 2);
}

/* may appear inside DefineSprite */
#define SPRITE 1
struct tag_func_struct
{
  char *name;
  int (*func) (SwfdecDecoder * s);
  int flag;
};
static struct tag_func_struct tag_funcs[] = {
  [ST_END] = {"End", tag_func_end, SPRITE},
  [ST_SHOWFRAME] = {"ShowFrame", tag_show_frame, SPRITE},
  [ST_DEFINESHAPE] = {"DefineShape", tag_define_shape, 0},
  [ST_FREECHARACTER] = {"FreeCharacter", NULL, 0},
  [ST_PLACEOBJECT] = {"PlaceObject", NULL, SPRITE},
  [ST_REMOVEOBJECT] = {"RemoveObject", swfdec_spriteseg_remove_object, SPRITE},
//      [ ST_DEFINEBITS         ] = { "DefineBits",     NULL,   0 },
  [ST_DEFINEBITSJPEG] = {"DefineBitsJPEG", tag_func_define_bits_jpeg, 0},
  [ST_DEFINEBUTTON] = {"DefineButton", tag_func_define_button, 0},
  [ST_JPEGTABLES] = {"JPEGTables", swfdec_image_jpegtables, 0},
  [ST_SETBACKGROUNDCOLOR] =
      {"SetBackgroundColor", tag_func_set_background_color, 0},
  [ST_DEFINEFONT] = {"DefineFont", tag_func_define_font, 0},
  [ST_DEFINETEXT] = {"DefineText", tag_func_define_text, 0},
  [ST_DOACTION] = {"DoAction", tag_func_do_action, SPRITE},
  [ST_DEFINEFONTINFO] = {"DefineFontInfo", tag_func_define_font_info_1, 0},
  [ST_DEFINESOUND] = {"DefineSound", tag_func_define_sound, 0},
  [ST_STARTSOUND] = {"StartSound", tag_func_start_sound, SPRITE},
  [ST_DEFINEBUTTONSOUND] =
      {"DefineButtonSound", tag_func_define_button_sound, 0},
  [ST_SOUNDSTREAMHEAD] = {"SoundStreamHead", tag_func_sound_stream_head, SPRITE},
  [ST_SOUNDSTREAMBLOCK] = {"SoundStreamBlock", tag_func_sound_stream_block, SPRITE},
  [ST_DEFINEBITSLOSSLESS] =
      {"DefineBitsLossless", tag_func_define_bits_lossless, 0},
  [ST_DEFINEBITSJPEG2] = {"DefineBitsJPEG2", tag_func_define_bits_jpeg_2, 0},
  [ST_DEFINESHAPE2] = {"DefineShape2", tag_define_shape_2, 0},
  [ST_DEFINEBUTTONCXFORM] = {"DefineButtonCXForm", NULL, 0},
  [ST_PROTECT] = {"Protect", tag_func_protect, 0},
  [ST_PLACEOBJECT2] = {"PlaceObject2", swfdec_spriteseg_place_object_2, SPRITE},
  [ST_REMOVEOBJECT2] = {"RemoveObject2", swfdec_spriteseg_remove_object_2, SPRITE},
  [ST_DEFINESHAPE3] = {"DefineShape3", tag_define_shape_3, 0},
  [ST_DEFINETEXT2] = {"DefineText2", tag_func_define_text_2, 0},
  [ST_DEFINEBUTTON2] = {"DefineButton2", tag_func_define_button_2, 0},
  [ST_DEFINEBITSJPEG3] = {"DefineBitsJPEG3", tag_func_define_bits_jpeg_3, 0},
  [ST_DEFINEBITSLOSSLESS2] =
      {"DefineBitsLossless2", tag_func_define_bits_lossless_2, 0},
  [ST_DEFINEEDITTEXT] = {"DefineEditText", tag_func_define_edit_text, 0},
  [ST_DEFINEMOVIE] = {"DefineMovie", NULL, 0},
  [ST_DEFINESPRITE] = {"DefineSprite", tag_func_define_sprite, 0},
  [ST_NAMECHARACTER] = {"NameCharacter", NULL, 0},
  [ST_SERIALNUMBER] = {"SerialNumber", NULL, 0},
  [ST_GENERATORTEXT] = {"GeneratorText", NULL, 0},
  [ST_FRAMELABEL] = {"FrameLabel", tag_func_frame_label, SPRITE},
  [ST_SOUNDSTREAMHEAD2] = {"SoundStreamHead2", tag_func_sound_stream_head, SPRITE},
  [ST_DEFINEMORPHSHAPE] =
      {"DefineMorphShape", NULL /* tag_define_morph_shape */ , 0},
  [ST_DEFINEFONT2] = {"DefineFont2", tag_func_define_font_2, 0},
  [ST_TEMPLATECOMMAND] = {"TemplateCommand", NULL, 0},
  [ST_GENERATOR3] = {"Generator3", NULL, 0},
  [ST_EXTERNALFONT] = {"ExternalFont", NULL, 0},
  [ST_EXPORTASSETS] = {"ExportAssets", tag_func_export_assets, 0},
  [ST_IMPORTASSETS] = {"ImportAssets", NULL, 0},
  [ST_ENABLEDEBUGGER] = {"EnableDebugger", NULL, 0},
  [ST_DOINITACTION] = {"DoInitAction", tag_func_do_init_action, SPRITE},
  [ST_DEFINEVIDEOSTREAM] = {"DefineVideoStream", NULL, 0},
  [ST_VIDEOFRAME] = {"VideoFrame", NULL, 0},
  [ST_DEFINEFONTINFO2] = {"DefineFontInfo2", tag_func_define_font_info_2, 0},
  [ST_MX4] = {"MX4", NULL, 0},
  [ST_ENABLEDEBUGGER2] = {"EnableDebugger2", NULL, 0},
  [ST_SCRIPTLIMITS] = {"ScriptLimits", NULL, 0},
  [ST_SETTABINDEX] = {"SetTabIndex", NULL, 0},
//      [ ST_REFLEX             ] = { "Reflex", NULL,   0 },
};
static const int n_tag_funcs = sizeof (tag_funcs) / sizeof (tag_funcs[0]);

const char *
swfdec_decoder_get_tag_name (int tag)
{
  if (tag >= 0 && tag < n_tag_funcs) {
    if (tag_funcs[tag].name) {
      return tag_funcs[tag].name;
    }
  }
  
  return "unknown";
}

SwfdecTagFunc *
swfdec_decoder_get_tag_func (int tag)
{ 
  if (tag >= 0 && tag < n_tag_funcs) {
    if (tag_funcs[tag].func) {
      return tag_funcs[tag].func;
    }
  }
  
  return NULL;
}

int
swfdec_decoder_get_tag_flag (int tag)
{
  if (tag >= 0 && tag < n_tag_funcs) {
    return tag_funcs[tag].flag;
  }
  
  return 0;
}
