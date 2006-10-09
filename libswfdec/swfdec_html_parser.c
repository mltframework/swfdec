#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <pango/pango.h>
#include "swfdec_edittext.h"
#include "swfdec_font.h"
#include "swfdec_debug.h"

struct _SwfdecParagraph {
  char *		text;		/* the text to display */
  PangoAttrList *	attrs;		/* attributes for the text */
  PangoAlignment	align;		/* alignment of this paragraph */

  /* used when parsing */
  guint			start;		/* start index of text */
  guint			end;		/* end index of text */
};

typedef struct {
  SwfdecEditText *	text;		/* element we're parsing for */
  GArray *		out;     	/* all the paragraphs we've created so far, will be the out value */
  GString *		str;		/* the text we've parsed so far */
  GList *		attributes;	/* PangoAttribute list we're currently in */
  GList *		attributes_completed;	/* completed attributes, oldest last */
  GList *		paragraphs;	/* SwfdecParagraph list we're currrently in */
} ParserData;

static void
swfdec_paragraph_init (SwfdecEditText *text, SwfdecParagraph *paragraph)
{
  paragraph->align = text->align;
  paragraph->attrs = pango_attr_list_new ();
  paragraph->text = NULL;
}

static void
swfdec_paragraph_finish (SwfdecParagraph *paragraph)
{
  g_free (paragraph->text);
  pango_attr_list_unref (paragraph->attrs);
}

static SwfdecParagraph *
paragraph_start (ParserData *data)
{
  SwfdecParagraph *paragraph;

  g_array_set_size (data->out, data->out->len + 1);
  paragraph = &g_array_index (data->out, SwfdecParagraph, data->out->len - 1);
  swfdec_paragraph_init (data->text, paragraph);
  paragraph->start = data->str->len;

  data->paragraphs = g_list_prepend (data->paragraphs, GUINT_TO_POINTER (data->out->len - 1));
  return paragraph;
}

static void
paragraph_end (ParserData *data, GError **error)
{
  SwfdecParagraph *paragraph = &g_array_index (data->out, SwfdecParagraph, GPOINTER_TO_UINT (data->paragraphs->data));
  paragraph->end = data->str->len;
  data->paragraphs = g_list_remove (data->paragraphs, data->paragraphs->data);
}

static void
attribute_start (ParserData *data, PangoAttribute *attribute)
{
  attribute->start_index = data->str->len;
  data->attributes = g_list_prepend (data->attributes, attribute);
}

static void
attribute_end (ParserData *data, GError **error)
{
  PangoAttribute *attribute;

  attribute = data->attributes->data;
  g_assert (attribute);
  attribute->end_index = data->str->len;
  data->attributes = g_list_remove (data->attributes, data->attributes->data);
  data->attributes_completed = g_list_prepend (data->attributes_completed, attribute);
}

static void
font_start (ParserData *data, const gchar **names, const gchar **values, 
    GError **error)
{
  PangoAttribute *attr;
  guint i;

  data->attributes = g_list_prepend (data->attributes, NULL);

  for (i = 0; names[i]; i++) {
    if (g_ascii_strcasecmp (names[i], "face") == 0) {
      attr = pango_attr_family_new (values[i]);
      attribute_start (data, attr);
    } else if (g_ascii_strcasecmp (names[i], "size") == 0) {
      const char *parse = values[i];
      char *end;
      guint size;
      if (parse[0] == '+' || parse[0] == '-') {
	SWFDEC_ERROR ("FIXME: implement relative font sizes!");
	parse++;
      }
      size = strtoul (parse, &end, 10);
      if (*end != '\0') {
	g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
	    "value \"%s\" is not valid for \"size\" attribute", values[i]);
	return;
      }
      size *= 20 * PANGO_SCALE;
      attr = pango_attr_size_new_absolute (size);
      attribute_start (data, attr);
    } else if (g_ascii_strcasecmp (names[i], "color") == 0) {
      PangoColor color;
      if (!pango_color_parse (&color, values[i])) {
	g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
	    "value \"%s\" is not valid for \"color\" attribute", values[i]);
	return;
      }
      attr = pango_attr_foreground_new (color.red, color.green, color.blue);
      attribute_start (data, attr);
    } else {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
	  "unknown attribute %s", names[i]);
      return;
    }
  }
}

static void
font_end (ParserData *data, GError **error)
{
  while (data->attributes->data)
    attribute_end (data, error);
  data->attributes = g_list_remove (data->attributes, data->attributes->data);
}

static void
p_start (ParserData *data, const gchar **names, const gchar **values, 
    GError **error)
{
  guint i;
  SwfdecParagraph *paragraph = paragraph_start (data);

  for (i = 0; names[i]; i++) {
    if (g_ascii_strcasecmp (names[i], "align") == 0) {
      if (g_ascii_strcasecmp (values[i], "left") == 0) {
	paragraph->align = PANGO_ALIGN_LEFT;
      } else if (g_ascii_strcasecmp (values[i], "right") == 0) {
	paragraph->align = PANGO_ALIGN_RIGHT;
      } else if (g_ascii_strcasecmp (values[i], "center") == 0) {
	paragraph->align = PANGO_ALIGN_CENTER;
      } else {
	g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_INVALID_CONTENT,
	    "value \"%s\" is not valid for \"align\" attribute", values[i]);
	return;
      }
    } else {
      g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ATTRIBUTE,
	  "unknown attribute %s", names[i]);
      return;
    }
  }
}

typedef struct {
  const char *		name;
  void			(* start)	(ParserData *		data,
					 const gchar **		attribute_names,
					 const gchar **		attribute_values, 
					 GError **		error);
  void			(* end)		(ParserData *		data,
					 GError **		error);
} ParserElement;

/* NB: must be sorted alphabetically */
ParserElement elements[] = {
  { "font", font_start, font_end },
  { "p", p_start, paragraph_end }
};

static int
element_compare (gconstpointer a, gconstpointer b)
{
  return g_ascii_strcasecmp (((const ParserElement *) a)->name, ((const ParserElement *) b)->name);
}

static ParserElement *
swfdec_html_parser_find_element (const char *name)
{
  ParserElement find = { name, NULL, NULL };

  return bsearch (&find, elements, G_N_ELEMENTS (elements), 
      sizeof (ParserElement), element_compare);
}

static void 
swfdec_html_parser_start_element (GMarkupParseContext *context, 
    const gchar *element_name, const gchar **attribute_names,
    const gchar **attribute_values, gpointer user_data, GError **error)
{
  ParserElement *element = swfdec_html_parser_find_element (element_name);

  if (element == NULL) {
    g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
	"unknown element %s", element_name);
    return;
  }
  element->start (user_data, attribute_names, attribute_values, error);
}

static void 
swfdec_html_parser_end_element (GMarkupParseContext *context, 
    const gchar *element_name, gpointer user_data, GError **error)
{
  ParserElement *element = swfdec_html_parser_find_element (element_name);

  if (element == NULL) {
    g_set_error (error, G_MARKUP_ERROR, G_MARKUP_ERROR_UNKNOWN_ELEMENT,
	"unknown element %s", element_name);
    return;
  }
  element->end (user_data, error);
}

static void 
swfdec_html_parser_text (GMarkupParseContext *context, const gchar *text, 
    gsize text_len, gpointer user_data, GError **error)
{
  ParserData *data = user_data;
  g_string_append_len (data->str, text, text_len);
}

static void 
swfdec_html_parser_error (GMarkupParseContext *context, GError *error, gpointer user_data)
{
  SWFDEC_ERROR ("error parsing html content: %s", error->message);
}

static const GMarkupParser parser = {
  swfdec_html_parser_start_element,
  swfdec_html_parser_end_element,
  swfdec_html_parser_text,
  NULL,
  swfdec_html_parser_error,
};

static void G_GNUC_UNUSED 
dump (ParserData *data)
{
  guint i;
  GList *walk;

  g_print ("paragraphs:\n");
  for (i = 0; i < data->out->len; i++) {
    SwfdecParagraph *paragraph = &g_array_index (data->out, SwfdecParagraph, i);
    g_print ("%3u -%3u %*s\n", paragraph->start, paragraph->end, 
	paragraph->end - paragraph->start, data->str->str + paragraph->start);
  }
  g_print ("attributes:\n");
  for (walk = g_list_last (data->attributes_completed); walk; walk = walk->prev) {
    PangoAttribute *attr = walk->data;
    g_print ("%3u -%3u %u\n", attr->start_index, attr->end_index, attr->klass->type);
  }
}

SwfdecParagraph *
swfdec_paragraph_html_parse (SwfdecEditText *text, const char *str)
{
  ParserData data = { NULL, };
  GMarkupParseContext *context;
  GError *error = NULL;
  SwfdecParagraph *retval;

  g_return_val_if_fail (SWFDEC_IS_EDIT_TEXT (text), NULL);
  g_return_val_if_fail (str != NULL, NULL);

  data.text = text;
  data.out = g_array_new (TRUE, TRUE, sizeof (SwfdecParagraph));
  data.str = g_string_new ("");
  context = g_markup_parse_context_new (&parser, 0, &data, NULL);
  if (g_markup_parse_context_parse (context, str, strlen (str), &error) &&
      g_markup_parse_context_end_parse (context, &error)) {
    guint i;
    GList *walk;

    //dump (&data);
    data.attributes_completed = g_list_reverse (data.attributes_completed);
    for (i = 0; i < data.out->len; i++) {
      SwfdecParagraph *para = &g_array_index (data.out, SwfdecParagraph, i);
      para->text = g_strndup (data.str->str + para->start, 
	  para->end - para->start);
      for (walk = data.attributes_completed; walk; walk = walk->next) {
	PangoAttribute *tmp = walk->data;
	if (tmp->start_index >= para->end || 
	    tmp->end_index <= para->start)
	  continue;
	tmp = pango_attribute_copy (tmp);
	if (tmp->start_index > para->start)
	  tmp->start_index -= para->start;
	else
	  tmp->start_index = 0;
	tmp->end_index = MIN (tmp->end_index, para->end) - para->start;
	if (para->attrs == NULL)
	  para->attrs = pango_attr_list_new ();
	pango_attr_list_change (para->attrs, tmp);
      }
    }
    g_assert (data.attributes == NULL);
    retval = (SwfdecParagraph *) g_array_free (data.out, FALSE);
  } else {
    GList *walk;
    g_array_free (data.out, TRUE);
    retval = NULL;
    for (walk = data.attributes; walk; walk = walk->next) {
      if (walk->data)
	pango_attribute_destroy (walk->data);
    }
    g_list_free (data.attributes);
  }
  g_list_foreach (data.attributes_completed, (GFunc) pango_attribute_destroy, NULL);
  g_list_free (data.paragraphs);
  g_string_free (data.str, TRUE);
  g_markup_parse_context_free (context);
  return retval;
}

SwfdecParagraph *
swfdec_paragraph_text_parse (SwfdecEditText *text, const char *str)
{
  SwfdecParagraph *ret;
    
  g_return_val_if_fail (SWFDEC_IS_EDIT_TEXT (text), NULL);
  g_return_val_if_fail (str != NULL, NULL);

  ret = g_new0 (SwfdecParagraph, 2);
  swfdec_paragraph_init (text, ret);
  ret->text = g_strdup (str);
  return ret;
}

void
swfdec_paragraph_free (SwfdecParagraph *paragraphs)
{
  guint i;

  for (i = 0; paragraphs[i].text != NULL; i++) {
    swfdec_paragraph_finish (&paragraphs[i]);
  }
  g_free (paragraphs);
}

void
swfdec_edit_text_render (SwfdecEditText *text, cairo_t *cr, const SwfdecParagraph *paragraph,
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  guint i;
  PangoFontDescription *desc;
  PangoLayout *layout;
  unsigned int width;
  swf_color color;

  g_return_if_fail (SWFDEC_IS_EDIT_TEXT (text));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (paragraph != NULL);
  g_return_if_fail (trans != NULL);
  g_return_if_fail (inval != NULL);

  if (text->font == NULL) {
    SWFDEC_ERROR ("no font to render with");
    return;
  }
  if (text->font->desc == NULL) {
    desc = pango_font_description_new ();
    pango_font_description_set_family (desc, "Sans");
    SWFDEC_INFO ("font %d has no cairo font description", SWFDEC_OBJECT (text->font)->id);
  } else {
    desc = pango_font_description_copy (text->font->desc);
  }
  pango_font_description_set_absolute_size (desc, text->height * PANGO_SCALE);
  layout = pango_cairo_create_layout (cr);
  pango_layout_set_font_description (layout, desc);
  pango_font_description_free (desc);
  width = SWFDEC_OBJECT (text)->extents.x1 - SWFDEC_OBJECT (text)->extents.x0 - text->left_margin - text->right_margin;
  cairo_move_to (cr, SWFDEC_OBJECT (text)->extents.x0 + text->left_margin, SWFDEC_OBJECT (text)->extents.y0);
  pango_layout_set_width (layout, width * PANGO_SCALE);
  color = swfdec_color_apply_transform (text->color, trans);
  swfdec_color_set_source (cr, color);

  for (i = 0; paragraph[i].text != NULL; i++) {
    pango_layout_set_text (layout, paragraph[i].text, -1);
    pango_layout_set_attributes (layout, paragraph[i].attrs);
    pango_layout_set_alignment (layout, paragraph[i].align);
    if (fill)
      pango_cairo_show_layout (cr, layout);
    else
      pango_cairo_layout_path (cr, layout);
  }
  g_object_unref (layout);
}

