#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pango/pangocairo.h>
#include <string.h>
#include <js/jsapi.h>
#include "swfdec.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_edittext.h"
#include "swfdec_font.h"
#include "swfdec_movieclip.h"

static gboolean
swfdec_edit_text_mouse_in (SwfdecObject *object,
      double x, double y, int button)
{
  return swfdec_rect_contains (&object->extents, x, y);
}

SWFDEC_OBJECT_BOILERPLATE (SwfdecEditText, swfdec_edit_text)

static void swfdec_edit_text_base_init (gpointer g_class)
{

}

static void
swfdec_edit_text_class_init (SwfdecEditTextClass * g_class)
{
  SwfdecObjectClass *object_class = SWFDEC_OBJECT_CLASS (g_class);

  object_class->mouse_in = swfdec_edit_text_mouse_in;
}

static void
swfdec_edit_text_init (SwfdecEditText * text)
{
  text->max_length = G_MAXUINT;
}

static void
swfdec_edit_text_dispose (SwfdecEditText *text)
{
  g_free (text->text);
  if (text->path) {
    g_free (text->path);
  } else if (text->variable) {
    g_free (text->variable);
  }
  text->path = NULL;
  text->variable = NULL;
  
  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (text));
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
  s = strrchr (text->variable, '/');
  if (s == NULL)
    s = strrchr (text->variable, '.');
  if (s) {
    text->path = text->variable;
    text->variable = s + 1;
    *s = 0;
  }
  SWFDEC_LOG ("parsed variable name into path \"%s\" and variable \"%s\"",
      text->path ? text->path : "", text->variable);
}
int
tag_func_define_edit_text (SwfdecDecoder * s)
{
  SwfdecEditText *text;
  unsigned int id;
  int reserved, use_outlines;
  gboolean has_font, has_color, has_max_length, has_layout, has_text;
  SwfdecBits *b = &s->b;
  
  id = swfdec_bits_get_u16 (b);
  SWFDEC_LOG ("  id = %u", id);
  text = swfdec_object_create (s, id, SWFDEC_TYPE_EDIT_TEXT);
  if (text == NULL)
    return SWFDEC_OK;

  swfdec_bits_get_rect (b, &SWFDEC_OBJECT (text)->extents);
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
    SwfdecObject *object;

    id = swfdec_bits_get_u16 (b);
    object = swfdec_object_get (s, id);
    if (SWFDEC_IS_FONT (object)) {
      SWFDEC_LOG ("  font = %u", id);
      text->font = SWFDEC_FONT (object);
    } else {
      SWFDEC_ERROR ("id %u does not specify a font", id);
    }
    text->height = swfdec_bits_get_u16 (b);
    SWFDEC_LOG ("  color = %u", text->height);
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

  return SWFDEC_OK;
}
