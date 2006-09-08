
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zlib.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include <liboil/liboil.h>
#include "swfdec_render.h"

#include "swfdec_internal.h"


struct tag_func_struct
{
  char *name;
  int (*func) (SwfdecDecoder * s);
  int flag;
};
static struct tag_func_struct tag_funcs[] = {
  [ST_END] = {"End", tag_func_end, 0},
  [ST_SHOWFRAME] = {"ShowFrame", tag_show_frame, 0},
  [ST_DEFINESHAPE] = {"DefineShape", tag_define_shape, 0},
  [ST_FREECHARACTER] = {"FreeCharacter", NULL, 0},
  [ST_PLACEOBJECT] = {"PlaceObject", NULL, 0},
  [ST_REMOVEOBJECT] = {"RemoveObject", swfdec_spriteseg_remove_object, 0},
//      [ ST_DEFINEBITS         ] = { "DefineBits",     NULL,   0 },
  [ST_DEFINEBITSJPEG] = {"DefineBitsJPEG", tag_func_define_bits_jpeg, 0},
  [ST_DEFINEBUTTON] = {"DefineButton", tag_func_define_button, 0},
  [ST_JPEGTABLES] = {"JPEGTables", swfdec_image_jpegtables, 0},
  [ST_SETBACKGROUNDCOLOR] =
      {"SetBackgroundColor", tag_func_set_background_color, 0},
  [ST_DEFINEFONT] = {"DefineFont", tag_func_define_font, 0},
  [ST_DEFINETEXT] = {"DefineText", tag_func_define_text, 0},
  [ST_DOACTION] = {"DoAction", tag_func_do_action, 0},
  [ST_DEFINEFONTINFO] = {"DefineFontInfo", tag_func_ignore, 0},
  [ST_DEFINESOUND] = {"DefineSound", tag_func_define_sound, 0},
  [ST_STARTSOUND] = {"StartSound", tag_func_start_sound, 0},
  [ST_DEFINEBUTTONSOUND] =
      {"DefineButtonSound", tag_func_define_button_sound, 0},
  [ST_SOUNDSTREAMHEAD] = {"SoundStreamHead", tag_func_sound_stream_head, 0},
  [ST_SOUNDSTREAMBLOCK] = {"SoundStreamBlock", tag_func_sound_stream_block, 0},
  [ST_DEFINEBITSLOSSLESS] =
      {"DefineBitsLossless", tag_func_define_bits_lossless, 0},
  [ST_DEFINEBITSJPEG2] = {"DefineBitsJPEG2", tag_func_define_bits_jpeg_2, 0},
  [ST_DEFINESHAPE2] = {"DefineShape2", tag_define_shape_2, 0},
  [ST_DEFINEBUTTONCXFORM] = {"DefineButtonCXForm", NULL, 0},
  [ST_PROTECT] = {"Protect", tag_func_protect, 0},
  [ST_PLACEOBJECT2] = {"PlaceObject2", swfdec_spriteseg_place_object_2, 0},
  [ST_REMOVEOBJECT2] = {"RemoveObject2", swfdec_spriteseg_remove_object_2, 0},
  [ST_DEFINESHAPE3] = {"DefineShape3", tag_define_shape_3, 0},
  [ST_DEFINETEXT2] = {"DefineText2", tag_func_define_text_2, 0},
  [ST_DEFINEBUTTON2] = {"DefineButton2", tag_func_define_button_2, 0},
  [ST_DEFINEBITSJPEG3] = {"DefineBitsJPEG3", tag_func_define_bits_jpeg_3, 0},
  [ST_DEFINEBITSLOSSLESS2] =
      {"DefineBitsLossless2", tag_func_define_bits_lossless_2, 0},
  [ST_DEFINEEDITTEXT] = {"DefineEditText", NULL, 0},
  [ST_DEFINEMOVIE] = {"DefineMovie", NULL, 0},
  [ST_DEFINESPRITE] = {"DefineSprite", tag_func_define_sprite, 0},
  [ST_NAMECHARACTER] = {"NameCharacter", NULL, 0},
  [ST_SERIALNUMBER] = {"SerialNumber", NULL, 0},
  [ST_GENERATORTEXT] = {"GeneratorText", NULL, 0},
  [ST_FRAMELABEL] = {"FrameLabel", tag_func_frame_label, 0},
  [ST_SOUNDSTREAMHEAD2] = {"SoundStreamHead2", tag_func_sound_stream_head, 0},
  [ST_DEFINEMORPHSHAPE] =
      {"DefineMorphShape", NULL /* tag_define_morph_shape */ , 0},
  [ST_DEFINEFONT2] = {"DefineFont2", tag_func_define_font_2, 0},
  [ST_TEMPLATECOMMAND] = {"TemplateCommand", NULL, 0},
  [ST_GENERATOR3] = {"Generator3", NULL, 0},
  [ST_EXTERNALFONT] = {"ExternalFont", NULL, 0},
  [ST_EXPORTASSETS] = {"ExportAssets", tag_func_export_assets, 0},
  [ST_IMPORTASSETS] = {"ImportAssets", NULL, 0},
  [ST_ENABLEDEBUGGER] = {"EnableDebugger", NULL, 0},
  [ST_DOINITACTION] = {"DoInitAction", tag_func_do_init_action, 0},
  [ST_DEFINEVIDEOSTREAM] = {"DefineVideoStream", NULL, 0},
  [ST_VIDEOFRAME] = {"VideoFrame", NULL, 0},
  [ST_DEFINEFONTINFO2] = {"DefineFontInfo2", NULL, 0},
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
tag_func_end (SwfdecDecoder * s)
{
  return SWF_OK;
}

int
tag_func_protect (SwfdecDecoder * s)
{
  return SWF_OK;
}

int
tag_func_ignore (SwfdecDecoder * s)
{
  if (s->b.buffer) {
    s->b.ptr += s->b.buffer->length;
  }

  return SWF_OK;
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

  return SWF_OK;
}

int
tag_func_frame_label (SwfdecDecoder * s)
{
  g_free (swfdec_bits_get_string (&s->b));

  return SWF_OK;
}


/* text */

static int
define_text (SwfdecDecoder * s, int rgba)
{
  SwfdecBits *bits = &s->b;
  int id;
  int rect[4];
  int n_glyph_bits;
  int n_advance_bits;
  SwfdecText *text = NULL;
  SwfdecTextGlyph glyph = { 0 };

  if (swfdec_bits_needbits(bits,2)) return SWF_ERROR;

  id = swfdec_bits_get_u16 (bits);
  text = swfdec_object_new (SWFDEC_TYPE_TEXT);
  SWFDEC_OBJECT (text)->id = id;
  s->objects = g_list_append (s->objects, text);

  glyph.color = 0xffffffff;

  swfdec_bits_get_rect (bits, rect);
  swfdec_bits_get_transform (bits, &SWFDEC_OBJECT (text)->transform);
  swfdec_bits_syncbits (bits);
  n_glyph_bits = swfdec_bits_get_u8 (bits);
  n_advance_bits = swfdec_bits_get_u8 (bits);

  //printf("  id = %d\n", id);
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
      for (i = 0; i < n_glyphs; i++) {
        glyph.glyph = swfdec_bits_getbits (bits, n_glyph_bits);

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
        glyph.font = swfdec_bits_get_u16 (bits);
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
        glyph.x = swfdec_bits_get_u16 (bits);
        //printf("  x = %d\n",x);
      }
      if (has_y_offset) {
        glyph.y = swfdec_bits_get_u16 (bits);
        //printf("  y = %d\n",y);
      }
      if (has_font) {
        glyph.height = swfdec_bits_get_u16 (bits);
        //printf("  height = %d\n",height);
      }
    }
    swfdec_bits_syncbits (bits);
  }
  swfdec_bits_get_u8 (bits);

  return SWF_OK;
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
  sprite = swfdec_object_new (SWFDEC_TYPE_SPRITE);
  SWFDEC_OBJECT (sprite)->id = id;
  s->objects = g_list_append (s->objects, sprite);

  SWFDEC_LOG ("  ID: %d", id);

  sprite->n_frames = swfdec_bits_get_u16 (bits);
  SWFDEC_LOG ("n_frames = %d", sprite->n_frames);

  sprite->frames = g_malloc0 (sizeof (SwfdecSpriteFrame) * sprite->n_frames);

  memcpy (&parse, bits, sizeof (SwfdecBits));

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
    } else {
      endptr = parse.ptr + tag_len;
      s->parse_sprite = sprite;
      ret = func (s);
      s->parse_sprite = NULL;

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

  return SWF_OK;
}

int
tag_func_set_background_color (SwfdecDecoder * s)
{
  SwfdecRect rect;

  s->bg_color = swfdec_bits_get_color (&s->b);

  rect.x0 = 0;
  rect.y0 = 0;
  rect.x1 = s->width;
  rect.y1 = s->height;

  swf_invalidate_irect (s, &rect);

  return SWF_OK;
}


int
tag_func_do_action (SwfdecDecoder * s)
{
  swfdec_sprite_add_action (s->parse_sprite, s->b.buffer,
      s->parse_sprite->parse_frame);

  s->b.ptr += s->b.buffer->length;

  return SWF_OK;
}


int
tag_func_do_init_action (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int len;
  SwfdecBuffer *buffer;
  SwfdecObject *obj;
  SwfdecSprite *save_parse_sprite;
  unsigned char *endptr;
  int retcode = SWF_ERROR;

  endptr = bits->ptr + bits->buffer->length;

  obj = swfdec_object_get (s, swfdec_bits_get_u16 (bits));

  len = bits->end - bits->ptr;

  buffer = swfdec_buffer_new_subbuffer (bits->buffer,
    bits->ptr - bits->buffer->data, len);

  bits->ptr += len;

  if (SWFDEC_IS_SPRITE(obj)) {
    save_parse_sprite = s->parse_sprite;
    s->parse_sprite = SWFDEC_SPRITE(obj);
    retcode = swfdec_action_script_execute (s, buffer);
    s->parse_sprite = save_parse_sprite;
  }
  swfdec_buffer_unref (buffer);

  return retcode;
}


int
tag_func_define_button_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  int flags;
  int offset;
  SwfdecTransform trans;
  SwfdecColorTransform color_trans;
  SwfdecButton *button;
  unsigned char *endptr;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  button = swfdec_object_new (SWFDEC_TYPE_BUTTON);
  SWFDEC_OBJECT (button)->id = id;
  s->objects = g_list_append (s->objects, button);

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
    SWFDEC_LOG ("hit_test=%d down=%d over=%d up=%d character=%d layer=%d",
        record.states & SWFDEC_BUTTON_HIT, record.states & SWFDEC_BUTTON_DOWN, 
        record.states & SWFDEC_BUTTON_OVER, record.states & SWFDEC_BUTTON_UP, 
	character, layer);

    swfdec_bits_get_transform (bits, &trans);
    swfdec_bits_syncbits (bits);
    swfdec_bits_get_color_transform (bits, &color_trans);
    swfdec_bits_syncbits (bits);

    record.segment = swfdec_spriteseg_new ();
    record.segment->id = character;
    memcpy (&record.segment->transform, &trans, sizeof (SwfdecTransform));
    memcpy (&record.segment->color_transform, &color_trans,
        sizeof (SwfdecColorTransform));

    g_array_append_val (button->records, record);
  }
  swfdec_bits_get_u8 (bits);

  while (offset != 0) {
    SwfdecButtonAction action = { 0 };
    int len;

    offset = swfdec_bits_get_u16 (bits);
    action.condition = swfdec_bits_get_u16 (bits);
    action.key = action.condition >> 9;
    action.condition &= 0x1FF;

    if (offset) {
      len = offset - 4;
    } else {
      len = bits->end - bits->ptr;
    }

    SWFDEC_LOG ("  offset = %d", offset);

    action.buffer = swfdec_buffer_new_subbuffer (bits->buffer,
        bits->ptr - bits->buffer->data, len);

    bits->ptr += len;

    g_array_append_val (button->actions, action);
  }
  SWFDEC_DEBUG ("number of actions %d", button->actions->len);

  return SWF_OK;
}

int
tag_func_define_button (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  SwfdecTransform trans;
  SwfdecColorTransform color_trans;
  SwfdecButton *button;
  unsigned char *endptr;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  button = swfdec_object_new (SWFDEC_TYPE_BUTTON);
  SWFDEC_OBJECT (button)->id = id;
  s->objects = g_list_append (s->objects, button);

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
    SWFDEC_LOG ("hit_test=%d down=%d over=%d up=%d character=%d layer=%d",
        record.states & SWFDEC_BUTTON_HIT, record.states & SWFDEC_BUTTON_DOWN, 
        record.states & SWFDEC_BUTTON_OVER, record.states & SWFDEC_BUTTON_UP, 
	character, layer);

    swfdec_bits_get_transform (bits, &trans);
    swfdec_bits_syncbits (bits);
    swfdec_bits_get_color_transform (bits, &color_trans);
    swfdec_bits_syncbits (bits);

    record.segment = swfdec_spriteseg_new ();
    record.segment->id = character;
    memcpy (&record.segment->transform, &trans, sizeof (SwfdecTransform));
    memcpy (&record.segment->color_transform, &color_trans,
        sizeof (SwfdecColorTransform));

    g_array_append_val (button->records, record);
  }
  swfdec_bits_get_u8 (bits);

  {
    SwfdecButtonAction action = { 0 };
    int len;

    action.condition = 0x08; /* over to down */

    len = bits->end - bits->ptr;

    action.buffer = swfdec_buffer_new_subbuffer (bits->buffer,
        bits->ptr - bits->buffer->data, len);

    bits->ptr += len;

    g_array_append_val (button->actions, action);
  }
  SWFDEC_DEBUG ("number of actions %d", button->actions->len);

  return SWF_OK;
}


int
tag_func_place_object_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int reserved;
  int has_unknown;
  int has_name;
  int has_ratio;
  int has_color_transform;
  int has_matrix;
  int has_character;
  int move;
  int depth;
  int character_id;
  int ratio;

  reserved = swfdec_bits_getbit (bits);
  has_unknown = swfdec_bits_getbit (bits);
  has_name = swfdec_bits_getbit (bits);
  has_ratio = swfdec_bits_getbit (bits);
  has_color_transform = swfdec_bits_getbit (bits);
  has_matrix = swfdec_bits_getbit (bits);
  has_character = swfdec_bits_getbit (bits);
  move = swfdec_bits_getbit (bits);
  depth = swfdec_bits_get_u16 (bits);

  SWFDEC_DEBUG ("  reserved = %d\n", reserved);
  SWFDEC_DEBUG ("  depth = %d\n", depth);

  if (has_character) {
    character_id = swfdec_bits_get_u16 (bits);
    SWFDEC_DEBUG ("  id = %d\n", character_id);
  }
  if (has_matrix) {
    SwfdecTransform trans;

    swfdec_bits_get_transform (bits, &trans);
  }
  if (has_color_transform) {
    SwfdecColorTransform ct;

    swfdec_bits_get_color_transform (bits, &ct);
    swfdec_bits_syncbits (bits);
  }
  if (has_ratio) {
    ratio = swfdec_bits_get_u16 (bits);
    SWFDEC_DEBUG ("  ratio = %d\n", ratio);
  }
  if (has_name) {
    g_free (swfdec_bits_get_string (bits));
  }

  return SWF_OK;
}

int
tag_func_remove_object (SwfdecDecoder * s)
{
  int id;
  int depth;

  id = swfdec_bits_get_u16 (&s->b);
  depth = swfdec_bits_get_u16 (&s->b);

  SWFDEC_DEBUG ("  id = %d\n", id);
  SWFDEC_DEBUG ("  depth = %d\n", depth);

  return SWF_OK;
}

int
tag_func_remove_object_2 (SwfdecDecoder * s)
{
  int depth;

  depth = swfdec_bits_get_u16 (&s->b);

  SWFDEC_DEBUG ("  depth = %d\n", depth);

  return SWF_OK;
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
  font = swfdec_object_new (SWFDEC_TYPE_FONT);
  SWFDEC_OBJECT (font)->id = id;
  s->objects = g_list_append (s->objects, font);

  offset = swfdec_bits_get_u16 (&s->b);
  n_glyphs = offset / 2;

  for (i = 1; i < n_glyphs; i++) {
    offset = swfdec_bits_get_u16 (&s->b);
  }

  font->glyphs = g_ptr_array_sized_new (n_glyphs);

  for (i = 0; i < n_glyphs; i++) {
    shape = swfdec_object_new (SWFDEC_TYPE_SHAPE);
    g_ptr_array_add (font->glyphs, shape);

    shape->fills = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills, shapevec);
    shape->fills2 = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills2, shapevec);
    shape->lines = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->lines, shapevec);

    //swf_shape_add_styles(s,shape,&s->b);
    swfdec_bits_syncbits (&s->b);
    shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_fill_bits = %d", shape->n_fill_bits);
    shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_line_bits = %d", shape->n_line_bits);

    swf_shape_get_recs (s, &s->b, shape);
  }

  return SWF_OK;
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
  int rect[4];

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
  font = swfdec_object_new (SWFDEC_TYPE_FONT);
  SWFDEC_OBJECT (font)->id = id;
  s->objects = g_list_append (s->objects, font);

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

  font->glyphs = g_ptr_array_sized_new (n_glyphs);

  for (i = 0; i < n_glyphs; i++) {
    shape = swfdec_object_new (SWFDEC_TYPE_SHAPE);
    g_ptr_array_add (font->glyphs, shape);

    shape->fills = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills, shapevec);
    shape->fills2 = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->fills2, shapevec);
    shape->lines = g_ptr_array_sized_new (1);
    shapevec = swf_shape_vec_new ();
    g_ptr_array_add (shape->lines, shapevec);

    //swf_shape_add_styles(s,shape,&s->b);
    swfdec_bits_syncbits (&s->b);
    shape->n_fill_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_fill_bits = %d", shape->n_fill_bits);
    shape->n_line_bits = swfdec_bits_getbits (&s->b, 4);
    SWFDEC_LOG ("n_line_bits = %d", shape->n_line_bits);

    swf_shape_get_recs (s, &s->b, shape);
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
    //font_bounds = swfdec_bits_get_s16(bits);
    for (i = 0; i < n_glyphs; i++) {
      swfdec_bits_get_rect (bits, rect);
    }
    kerning_count = swfdec_bits_get_u16 (bits);
    if (0) {
      for (i = 0; i < kerning_count; i++) {
        get_kerning_record (bits, wide_codes);
      }
    }
  }

  return SWF_OK;
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

  return SWF_OK;
}

