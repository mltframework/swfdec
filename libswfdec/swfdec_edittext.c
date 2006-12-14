#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pango/pangocairo.h>
#include <string.h>
#include <js/jsapi.h>
#include "swfdec_edittext.h"
#include "swfdec_debug.h"
#include "swfdec_edittext_movie.h"
#include "swfdec_font.h"
#include "swfdec_js.h"
#include "swfdec_player_internal.h"
#include "swfdec_swf_decoder.h"

G_DEFINE_TYPE (SwfdecEditText, swfdec_edit_text, SWFDEC_TYPE_GRAPHIC)

static gboolean
swfdec_edit_text_mouse_in (SwfdecGraphic *graphic, double x, double y)
{
  return swfdec_rect_contains (&graphic->extents, x, y);
}

static SwfdecMovie *
swfdec_edit_text_create_movie (SwfdecGraphic *graphic)
{
  SwfdecEditText *text = SWFDEC_EDIT_TEXT (graphic);
  SwfdecEditTextMovie *ret = g_object_new (SWFDEC_TYPE_EDIT_TEXT_MOVIE, NULL);

  ret->text = text;
  if (text->text)
    swfdec_edit_text_movie_set_text (ret, text->text);

  return SWFDEC_MOVIE (ret);
}

static void
swfdec_edit_text_dispose (GObject *object)
{
  SwfdecEditText *text = SWFDEC_EDIT_TEXT (object);

  g_free (text->text);
  g_free (text->variable);
  text->variable = NULL;
  g_free (text->variable_prefix);
  text->variable_prefix = NULL;
  
  G_OBJECT_CLASS (swfdec_edit_text_parent_class)->dispose (object);
}

static void
swfdec_edit_text_class_init (SwfdecEditTextClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);

  object_class->dispose = swfdec_edit_text_dispose;
  graphic_class->create_movie = swfdec_edit_text_create_movie;
  graphic_class->mouse_in = swfdec_edit_text_mouse_in;
}

static void
swfdec_edit_text_init (SwfdecEditText * text)
{
  text->max_length = G_MAXUINT;
}

static void
swfdec_edit_text_parse_variable (SwfdecEditText *text)
{
  char *s;

  if (text->variable && text->variable[0] == '\0') {
    g_free (text->variable);
    text->variable = NULL;
    return;
  }
  /* FIXME: check the variable for valid identifiers */
  if (strchr (text->variable, '/')) {
    char *ret = swfdec_js_slash_to_dot (text->variable);
    g_free (text->variable);
    text->variable = ret;
  }
  if (!text->variable)
    return;
  s = strrchr (text->variable, '.');
  if (s) {
    guint len = s - text->variable;
    text->variable_prefix = g_strndup (text->variable, len);
    text->variable_name = s + 1;
  } else {
    text->variable_name = text->variable;
  }
}

int
tag_func_define_edit_text (SwfdecSwfDecoder * s)
{
  SwfdecEditText *text;
  unsigned int id;
  int reserved, use_outlines;
  gboolean has_font, has_color, has_max_length, has_layout, has_text;
  SwfdecBits *b = &s->b;
  
  id = swfdec_bits_get_u16 (b);
  SWFDEC_LOG ("  id = %u", id);
  text = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_EDIT_TEXT);
  if (text == NULL)
    return SWFDEC_STATUS_OK;

  swfdec_bits_get_rect (b, &SWFDEC_GRAPHIC (text)->extents);
  SWFDEC_LOG ("  extents: %d %d  %d %d", 
      SWFDEC_GRAPHIC (text)->extents.x0, SWFDEC_GRAPHIC (text)->extents.y0,
      SWFDEC_GRAPHIC (text)->extents.x1, SWFDEC_GRAPHIC (text)->extents.y1);
  swfdec_bits_syncbits (b);
  has_text = swfdec_bits_getbit (b);
  text->wrap = swfdec_bits_getbit (b);
  text->multiline = swfdec_bits_getbit (b);
  text->password = swfdec_bits_getbit (b);
  text->readonly = swfdec_bits_getbit (b);
  has_color = swfdec_bits_getbit (b);
  has_max_length = swfdec_bits_getbit (b);
  has_font = swfdec_bits_getbit (b);
  reserved = swfdec_bits_getbit (b);
  text->autosize = swfdec_bits_getbit (b);
  has_layout = swfdec_bits_getbit (b);
  text->selectable = !swfdec_bits_getbit (b);
  text->border = swfdec_bits_getbit (b);
  reserved = swfdec_bits_getbit (b);
  text->html = swfdec_bits_getbit (b);
  use_outlines = swfdec_bits_getbit (b); /* FIXME: what's this? */
  if (has_font) {
    SwfdecCharacter *font;

    id = swfdec_bits_get_u16 (b);
    font = swfdec_swf_decoder_get_character (s, id);
    if (SWFDEC_IS_FONT (font)) {
      SWFDEC_LOG ("  font = %u", id);
      text->font = SWFDEC_FONT (font);
    } else {
      SWFDEC_ERROR ("id %u does not specify a font", id);
    }
    text->height = swfdec_bits_get_u16 (b);
    SWFDEC_LOG ("  height = %u", text->height);
  }
  if (has_color) {
    text->color = swfdec_bits_get_rgba (b);
    SWFDEC_LOG ("  color = %u", text->color);
  } else {
    SWFDEC_WARNING ("FIXME: figure out default color");
    text->color = SWF_COLOR_COMBINE (255, 255, 255, 255);
  }
  if (has_max_length) {
    text->max_length = swfdec_bits_get_u16 (b);
  }
  if (has_layout) {
    unsigned int align = swfdec_bits_get_u8 (b);
    switch (align) {
      case 0:
	text->align = PANGO_ALIGN_LEFT;
	break;
      case 1:
	text->align = PANGO_ALIGN_RIGHT;
	break;
      case 2:
	text->align = PANGO_ALIGN_CENTER;
	break;
      case 3:
	text->justify = TRUE;
	break;
      default:
	SWFDEC_ERROR ("undefined align value %u", align);
	break;
    }
    text->left_margin = swfdec_bits_get_u16 (b);
    text->right_margin = swfdec_bits_get_u16 (b);
    text->indent = swfdec_bits_get_u16 (b);
    text->spacing = swfdec_bits_get_s16 (b);
  }
  text->variable = swfdec_bits_get_string (b);
  swfdec_edit_text_parse_variable (text);
  if (has_text)
    text->text = swfdec_bits_get_string (b);

  return SWFDEC_STATUS_OK;
}
