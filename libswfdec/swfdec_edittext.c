#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <pango/pangocairo.h>
#include "swfdec_edittext.h"
#include "swfdec_internal.h"
#include "swfdec_font.h"

static gboolean
swfdec_edit_text_mouse_in (SwfdecObject *object,
      double x, double y, int button)
{
  return swfdec_rect_contains (&object->extents, x, y);
}

static void
swfdec_edit_text_render (SwfdecObject *obj, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  SwfdecEditText *text = SWFDEC_EDIT_TEXT (obj);

  if (text->paragraph == NULL)
    return;
  if (text->font == NULL) {
    SWFDEC_ERROR ("no font to render with");
    return;
  }

  swfdec_paragraph_render (text, cr, text->paragraph, fill);
}

SWFDEC_OBJECT_BOILERPLATE (SwfdecEditText, swfdec_edit_text)

static void swfdec_edit_text_base_init (gpointer g_class)
{

}

static void
swfdec_edit_text_class_init (SwfdecEditTextClass * g_class)
{
  SwfdecObjectClass *object_class = SWFDEC_OBJECT_CLASS (g_class);

  object_class->render = swfdec_edit_text_render;
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
  if (text->paragraph)
    swfdec_paragraph_free (text->paragraph);
  
  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (text));
}

static void
swfdec_edit_text_set_text (SwfdecEditText *text, const char *str)
{
  g_assert (str != NULL);

  if (text->paragraph)
    swfdec_paragraph_free (text->paragraph);
  g_free (text->text);
  text->text = g_strdup (str);
  if (text->html)
    text->paragraph = swfdec_paragraph_html_parse (text, str);
  else
    text->paragraph = swfdec_paragraph_text_parse (text, str);
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
  /* FIXME: needs a smarter verification of proper variable names */
  if (text->variable && text->variable[0] == '\0') {
    g_free (text->variable);
    text->variable = NULL;
  }
  if (has_text)
    swfdec_edit_text_set_text (text, swfdec_bits_skip_string (b));

  return SWFDEC_OK;
}
