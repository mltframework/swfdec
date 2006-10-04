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
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  PangoFontDescription *desc;
  PangoLayout *layout;
  SwfdecEditText *text = SWFDEC_EDIT_TEXT (obj);
  unsigned int width;

  if (text->text == NULL)
    return;
  layout = pango_cairo_create_layout (cr);
  if (text->font == NULL) {
    SWFDEC_ERROR ("no font to render with");
    return;
  }
  if (text->font->desc == NULL) {
    desc = pango_font_description_new ();
    pango_font_description_set_family (desc, "Sans");
    SWFDEC_ERROR ("font %d has no cairo font description", SWFDEC_OBJECT (text->font)->id);
  } else {
    desc = pango_font_description_copy (text->font->desc);
  }
  pango_font_description_set_absolute_size (desc, text->height * PANGO_SCALE);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);
  pango_layout_set_text (layout, text->text, -1);
  width = obj->extents.x1 - obj->extents.x0 - text->left_margin - text->right_margin;
  pango_layout_set_width (layout, width);
  cairo_move_to (cr, obj->extents.x0 + text->left_margin, obj->extents.y0);
  cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
  {
    double x, y;
    cairo_get_current_point (cr, &x, &y);
    cairo_user_to_device (cr, &x, &y);
    g_print ("rendering \"%s\" to %g %g\n", text->text, x, y);
  }
  pango_cairo_show_layout (cr, layout);
  {
    double x, y;
    cairo_get_current_point (cr, &x, &y);
    cairo_user_to_device (cr, &x, &y);
    g_print ("done rendering \"%s\" to %g %g\n", text->text, x, y);
  }

  g_object_unref (layout);
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
swfdec_edit_text_dispose (SwfdecEditText * edit_text)
{
  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (edit_text));
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
  if (swfdec_bits_getbit (b))
    SWFDEC_WARNING ("FIXME: html text parsing not supported");
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
  if (has_text)
    text->text = swfdec_bits_get_string (b);

  return SWFDEC_OK;
}
