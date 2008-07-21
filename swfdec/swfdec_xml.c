/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include <math.h>
#include <string.h>

#include "swfdec_xml.h"
#include "swfdec_xml_node.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_array.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecXml, swfdec_xml, SWFDEC_TYPE_XML_NODE)

static void
swfdec_xml_do_mark (SwfdecAsObject *object)
{
  SwfdecXml *xml = SWFDEC_XML (object);

  if (xml->xml_decl != NULL)
    swfdec_as_string_mark (xml->xml_decl);
  if (xml->doc_type_decl != NULL)
    swfdec_as_string_mark (xml->doc_type_decl);

  SWFDEC_AS_OBJECT_CLASS (swfdec_xml_parent_class)->mark (object);
}

static void
swfdec_xml_class_init (SwfdecXmlClass *klass)
{
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  asobject_class->mark = swfdec_xml_do_mark;
}

static void
swfdec_xml_init (SwfdecXml *xml)
{
}

typedef struct {
  const char	character;
  const char	*escaped;
} EntityConversion;

static EntityConversion xml_entities[] = {
  { '&', "&amp;" },
  { '"', "&quot;" },
  { '\'', "&apos;" },
  { '<', "&lt;" },
  { '>', "&gt;" },
  { '\xa0', "&nbsp;" },
  { '\0', NULL }
};

char *
swfdec_xml_escape_len (const char *orginal, gssize length)
{
  int i;
  const char *p, *start;
  GString *string;

  string = g_string_new ("");

  // Note: we don't escape non-breaking space to &nbsp;
  p = start = orginal;
  while (*(p += strcspn (p, "&<>\"'")) != '\0' && p - orginal < length) {
    string = g_string_append_len (string, start, p - start);

    // escape it
    for (i = 0; xml_entities[i].escaped != NULL; i++) {
      if (xml_entities[i].character == *p) {
	string = g_string_append (string, xml_entities[i].escaped);
	break;
      }
    }
    g_assert (xml_entities[i].escaped != NULL);

    p++;
    start = p;
  }
  string = g_string_append_len (string, start, length - (start - orginal));

  return g_string_free (string, FALSE);
}

char *
swfdec_xml_escape (const char *orginal)
{
  return swfdec_xml_escape_len (orginal, strlen (orginal));
}

char *
swfdec_xml_unescape_len (SwfdecAsContext *cx, const char *orginal,
    gssize length, gboolean unescape_nbsp)
{
  int i;
  const char *p, *start, *end;
  GString *string;

  string = g_string_new ("");

  p = start = orginal;
  end = orginal + length;
  while ((p = memchr (p, '&', end - p)) != NULL) {
    string = g_string_append_len (string, start, p - start);

    for (i = 0; xml_entities[i].escaped != NULL; i++) {
      if (!g_ascii_strncasecmp (p, xml_entities[i].escaped,
	    strlen (xml_entities[i].escaped))) {
	// FIXME: Do this cleaner
	if (xml_entities[i].character == '\xa0') {
	  if (unescape_nbsp)
	    string = g_string_append_c (string, '\xc2');
	  else
	    continue;
	}
	string = g_string_append_c (string, xml_entities[i].character);
	p += strlen (xml_entities[i].escaped);
	break;
      }
    }
    if (xml_entities[i].escaped == NULL) {
      string = g_string_append_c (string, '&');
      p++;
    }

    start = p;
  }
  string = g_string_append_len (string, start, length - (start - orginal));

  return g_string_free (string, FALSE);
}

char *
swfdec_xml_unescape (SwfdecAsContext *cx, const char *orginal)
{
  return swfdec_xml_unescape_len (cx, orginal, strlen (orginal), TRUE);
}

// this is never declared, only available as ASnative (100, 5)
SWFDEC_AS_NATIVE (100, 5, swfdec_xml_do_escape)
void
swfdec_xml_do_escape (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  char *escaped;

  if (argc < 1)
    return;

  escaped = swfdec_xml_escape (swfdec_as_value_to_string (cx, &argv[0]));
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, escaped));
}

static void
swfdec_xml_get_ignoreWhite (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, SWFDEC_XML (object)->ignore_white);
}

static void
swfdec_xml_set_ignoreWhite (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  // special case, call toString of objects
  if (SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]))
    swfdec_as_value_to_string (cx, &argv[0]);

  SWFDEC_XML (object)->ignore_white =
    swfdec_as_value_to_boolean (cx, &argv[0]);
}

static void
swfdec_xml_get_xmlDecl (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (SWFDEC_XML (object)->xml_decl != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_XML (object)->xml_decl);
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (ret);
  }
}

static void
swfdec_xml_set_xmlDecl (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  SWFDEC_XML (object)->xml_decl = swfdec_as_value_to_string (cx, &argv[0]);
}

static void
swfdec_xml_get_docTypeDecl (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (SWFDEC_XML (object)->doc_type_decl != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_XML (object)->doc_type_decl);
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (ret);
  }
}

static void
swfdec_xml_set_docTypeDecl (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  SWFDEC_XML (object)->doc_type_decl =
    swfdec_as_value_to_string (cx, &argv[0]);
}

static void
swfdec_xml_get_contentType (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  *ret = SWFDEC_XML (object)->content_type;
}

static void
swfdec_xml_set_contentType (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  SWFDEC_XML (object)->content_type = argv[0];
}

static void
swfdec_xml_get_loaded (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  *ret = SWFDEC_XML (object)->loaded;
}

static void
swfdec_xml_set_loaded (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  // special case, call toString of objects
  if (SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]))
    swfdec_as_value_to_string (cx, &argv[0]);

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  SWFDEC_AS_VALUE_SET_BOOLEAN (&SWFDEC_XML (object)->loaded,
      swfdec_as_value_to_boolean (cx, &argv[0]));
}

static void
swfdec_xml_get_status (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  SWFDEC_AS_VALUE_SET_INT (ret, SWFDEC_XML (object)->status);
}

static void
swfdec_xml_set_status (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  double d;

  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  swfdec_as_value_to_string (cx, &argv[0]);
  d = swfdec_as_value_to_number (cx, &argv[0]);
  if (!isfinite (d))
    SWFDEC_XML (object)->status = 0;
  else
    SWFDEC_XML (object)->status = d;
}

static const char *
swfdec_xml_parse_xmlDecl (SwfdecXml *xml, SwfdecXmlNode *node, const char *p)
{
  const char *end;
  GString *string;

  g_assert (p != NULL);
  g_return_val_if_fail (g_ascii_strncasecmp (p, "<?xml", strlen ("<?xml")) == 0,
      strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_XML (xml), strchr (p, '\0'));

  end = strstr (p, "?>");
  if (end == NULL) {
    xml->status = XML_PARSE_STATUS_XMLDECL_NOT_TERMINATED;
    return strchr (p, '\0');
  }

  end += strlen ("?>");

  string = g_string_new ((xml->xml_decl != NULL ? xml->xml_decl : ""));
  string = g_string_append_len (string, p, end - p);
  xml->xml_decl = swfdec_as_context_give_string (
      SWFDEC_AS_OBJECT (xml)->context, g_string_free (string, FALSE));

  // in version 5 parsing xmlDecl or docType always adds undefined element to
  // the childNodes array
  if (SWFDEC_AS_OBJECT (xml)->context->version < 6)
    SWFDEC_FIXME ("Need to add undefined element to childNodes array");

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_docTypeDecl (SwfdecXml *xml, SwfdecXmlNode *node,
    const char *p)
{
  const char *end;
  int open;

  g_assert (p != NULL);
  g_return_val_if_fail (
      g_ascii_strncasecmp (p, "<!DOCTYPE", strlen ("<!DOCTYPE")) == 0,
      strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_XML (xml), strchr (p, '\0'));

  end = p + 1;
  open = 1;
  do {
    end += strcspn (end, "<>");
    if (*end == '<') {
      open++;
      end++;
    } else if (*end == '>') {
      open--;
      end++;
    }
  } while (*end != '\0' && open > 0);

  if (*end == '\0') {
    xml->status = XML_PARSE_STATUS_DOCTYPEDECL_NOT_TERMINATED;
  } else {
    xml->doc_type_decl = swfdec_as_context_give_string (
	SWFDEC_AS_OBJECT (xml)->context, g_strndup (p, end - p));

    // in version 5 parsing xmlDecl or docType always adds undefined element to
    // the childNodes array
    if (SWFDEC_AS_OBJECT (xml)->context->version < 6)
      SWFDEC_FIXME ("Need to add undefined element to childNodes array");
  }

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_comment (SwfdecXml *xml, const char *p)
{
  const char *end;

  g_assert (p != NULL);
  g_return_val_if_fail (strncmp (p, "<!--", strlen ("<!--")) == 0,
      strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_XML (xml), strchr (p, '\0'));

  end = strstr (p, "-->");

  if (end == NULL) {
    xml->status = XML_PARSE_STATUS_COMMENT_NOT_TERMINATED;
    return strchr (p, '\0');
  }

  end += strlen("-->");

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static void
swfdec_xml_add_id_map (SwfdecXml *xml, SwfdecXmlNode *node, const char *id)
{
  SwfdecAsObject *object;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_XML (xml));
  g_return_if_fail (SWFDEC_IS_XML_NODE (xml));
  g_return_if_fail (id != NULL && id != SWFDEC_AS_STR_EMPTY);

  if (SWFDEC_AS_OBJECT (xml)->context->version >= 8) {
    if (swfdec_as_object_get_variable (SWFDEC_AS_OBJECT (xml),
	  SWFDEC_AS_STR_idMap, &val)) {
      if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
	object = SWFDEC_AS_VALUE_GET_OBJECT (&val);
      } else {
	return;
      }
    } else {
      object = swfdec_as_object_new_empty (SWFDEC_AS_OBJECT (xml)->context);
      SWFDEC_AS_VALUE_SET_OBJECT (&val, object);
      swfdec_as_object_set_variable (SWFDEC_AS_OBJECT (xml),
	  SWFDEC_AS_STR_idMap, &val);
    }
  } else {
    object = SWFDEC_AS_OBJECT (xml);
  }

  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (node));
  swfdec_as_object_set_variable (object, id, &val);
}

static const char *
swfdec_xml_parse_attribute (SwfdecXml *xml, SwfdecXmlNode *node, const char *p)
{
  SwfdecAsValue val;
  const char *end, *name;
  char *text;

  g_return_val_if_fail (SWFDEC_IS_XML (xml), strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_XML_NODE (node), strchr (p, '\0'));
  g_return_val_if_fail ((*p != '>' && *p != '\0'), p);

  end = p + strcspn (p, "=> \r\n\t");
  if (end - p <= 0) {
    xml->status = XML_PARSE_STATUS_ELEMENT_MALFORMED;
    return strchr (p, '\0');
  }

  text = g_strndup (p, end - p);
  name = swfdec_as_context_give_string (SWFDEC_AS_OBJECT (node)->context,
      swfdec_xml_unescape (SWFDEC_AS_OBJECT (xml)->context, text));
  g_free (text);

  p = end + strspn (end, " \r\n\t");
  if (*p != '=') {
    xml->status = XML_PARSE_STATUS_ELEMENT_MALFORMED;
    return strchr (p, '\0');
  }
  p = p + 1 + strspn (p + 1, " \r\n\t");

  if (*p != '"' && *p != '\'') {
    xml->status = XML_PARSE_STATUS_ELEMENT_MALFORMED;
    return strchr (p, '\0');
  }

  end = p + 1;
  do {
    end = strchr (end, *p);
  } while (end != NULL && *(end - 1) == '\\');

  if (end == NULL) {
    xml->status = XML_PARSE_STATUS_ATTRIBUTE_NOT_TERMINATED;
    return strchr (p, '\0');
  }

  if (!swfdec_as_object_get_variable (node->attributes, name, NULL)) {
    char *unescaped;
    const char *value;

    unescaped = swfdec_xml_unescape_len (SWFDEC_AS_OBJECT (xml)->context,
	p + 1, end - (p + 1), TRUE);
    value = swfdec_as_context_give_string (SWFDEC_AS_OBJECT (node)->context,
	unescaped);
    SWFDEC_AS_VALUE_SET_STRING (&val, value);

    swfdec_as_object_set_variable (node->attributes, name, &val);
  }

  g_return_val_if_fail (end + 1 > p, strchr (p, '\0'));

  return end + 1;
}

static const char *
swfdec_xml_parse_tag (SwfdecXml *xml, SwfdecXmlNode **node, const char *p)
{
  SwfdecAsObject *object;
  SwfdecXmlNode *child = NULL; // surpress warning
  char *name;
  const char *end;
  gboolean close;

  g_assert (p != NULL);
  g_return_val_if_fail (*p == '<', strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_XML (xml), strchr (p, '\0'));

  object = SWFDEC_AS_OBJECT (xml);

  // closing tag or opening tag?
  if (*(p + 1) == '/') {
    close = TRUE;
    p++;
  } else {
    close = FALSE;
  }

  // find the end of the name
  end = p + strcspn (p, "> \r\n\t");

  // don't count trailing / as part of the name if it's followed by >
  // note we do this for close tags also, so <test/ ></test/> doesn't work
  if (*end == '>' && *(end - 1) == '/')
    end = end - 1;

  if (end - (p + 1) <= 0 || *end == '\0') {
    xml->status = XML_PARSE_STATUS_ELEMENT_MALFORMED;
    return strchr (p, '\0');
  }

  name = g_strndup (p + 1 , end - (p + 1));

  // create the new element
  if (!close) {
    child = swfdec_xml_node_new_no_properties (
	SWFDEC_AS_OBJECT (*node)->context, SWFDEC_XML_NODE_ELEMENT,
	swfdec_as_context_give_string (SWFDEC_AS_OBJECT (*node)->context,
	  name));
    if (child == NULL)
      return strchr (p, '\0');
  }

  if (close) {
    end = strchr (end, '>');
    if (end == NULL)
      end = strchr (p, '\0');
  } else {
    end = end + strspn (end, " \r\n\t");
    while (*end != '\0' && *end != '>' && (*end != '/' || *(end + 1) != '>')) {
      end = swfdec_xml_parse_attribute (xml, child, end);
      end = end + strspn (end, " \r\n\t");
    }
    if (*end == '/')
      end += 1;
  }

  if (*end == '\0') {
    if (xml->status == XML_PARSE_STATUS_OK)
      xml->status = XML_PARSE_STATUS_ELEMENT_MALFORMED;
    if (close)
      g_free (name);
    return end;
  }

  if (close) {
    if ((*node)->parent != NULL && !g_ascii_strcasecmp ((*node)->name, name))
    {
      *node = (*node)->parent;
    }
    else // error
    {
      SwfdecXmlNode *iter = *node;
      while (iter != NULL && (iter->name == NULL || g_ascii_strcasecmp (iter->name, name))) {
	iter = iter->parent;
      }
      if (iter != NULL) {
        xml->status = XML_PARSE_STATUS_TAG_NOT_CLOSED;
      } else {
        xml->status = XML_PARSE_STATUS_TAG_MISMATCH;
      }
    }
    g_free (name);
  } else {
    const char *id;

    swfdec_xml_node_appendChild (*node, child);

    id = swfdec_xml_node_get_attribute (child, SWFDEC_AS_STR_id);
    if (id != NULL)
      swfdec_xml_add_id_map (xml, child, id);

    if (*(end - 1) != '/')
      *node = child;
  }

  end += 1;

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_cdata (SwfdecXml *xml, SwfdecXmlNode *node, const char *p)
{
  SwfdecXmlNode *child;
  const char *end;
  char *text;

  g_assert (p != NULL);
  g_return_val_if_fail (strncmp (p, "<![CDATA[", strlen ("<![CDATA[")) == 0,
      strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_XML (xml), strchr (p, '\0'));

  p += strlen ("<![CDATA[");

  end = strstr (p, "]]>");

  if (end == NULL) {
    xml->status = XML_PARSE_STATUS_CDATA_NOT_TERMINATED;
    return strchr (p, '\0');
  }

  text = g_strndup (p, end - p);

  child = swfdec_xml_node_new_no_properties (
      SWFDEC_AS_OBJECT (node)->context, SWFDEC_XML_NODE_TEXT,
      swfdec_as_context_give_string (SWFDEC_AS_OBJECT (xml)->context, text));
  if (child == NULL)
    return strchr (p, '\0');
  swfdec_xml_node_appendChild (node, child);

  end += strlen("]]>");

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_text (SwfdecXml *xml, SwfdecXmlNode *node,
    const char *p, gboolean ignore_white)
{
  SwfdecXmlNode *child;
  const char *end;
  char *text, *unescaped;

  g_assert (p != NULL);
  g_return_val_if_fail (*p != '\0', p);
  g_return_val_if_fail (SWFDEC_IS_XML (xml), strchr (p, '\0'));

  end = strchr (p, '<');
  if (end == NULL)
    end = strchr (p, '\0');

  if (!ignore_white || strspn (p, " \t\r\n") < (gsize)(end - p))
  {
    text = g_strndup (p, end - p);
    unescaped = swfdec_xml_unescape (SWFDEC_AS_OBJECT (xml)->context, text);
    g_free (text);
    child = swfdec_xml_node_new_no_properties (
	SWFDEC_AS_OBJECT (node)->context, SWFDEC_XML_NODE_TEXT,
	swfdec_as_context_give_string (SWFDEC_AS_OBJECT (xml)->context,
	  unescaped));
    if (child == NULL)
      return strchr (p, '\0');
    swfdec_xml_node_appendChild (node, child);
  }

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static void
swfdec_xml_parseXML (SwfdecXml *xml, const char *value)
{
  SwfdecAsObject *object;
  SwfdecXmlNode *node;
  const char *p;
  gboolean ignore_white;

  g_return_if_fail (SWFDEC_IS_XML (xml));
  g_return_if_fail (value != NULL);

  object = SWFDEC_AS_OBJECT (xml);

  swfdec_xml_node_removeChildren (SWFDEC_XML_NODE (xml));
  xml->xml_decl = NULL;
  xml->doc_type_decl = NULL;
  xml->status = XML_PARSE_STATUS_OK;

  p = value;
  node = SWFDEC_XML_NODE (xml);

  // special case: we only use the ignoreWhite set at the start
  ignore_white = xml->ignore_white;

  while (xml->status == XML_PARSE_STATUS_OK && *p != '\0') {
    if (*p == '<') {
      if (g_ascii_strncasecmp (p + 1, "?xml", strlen ("?xml")) == 0) {
	p = swfdec_xml_parse_xmlDecl (xml, node, p);
      } else if (g_ascii_strncasecmp (p + 1, "!DOCTYPE", strlen ("!DOCTYPE")) == 0) {
	p = swfdec_xml_parse_docTypeDecl (xml, node, p);
      } else if (strncmp (p + 1, "!--", strlen ("!--")) == 0) {
	p = swfdec_xml_parse_comment (xml, p);
      } else if (g_ascii_strncasecmp (p + 1, "![CDATA", strlen ("![CDATA")) == 0) {
	p = swfdec_xml_parse_cdata (xml, node, p);
      } else {
	p = swfdec_xml_parse_tag (xml, &node, p);
      }
    } else {
      p = swfdec_xml_parse_text (xml, node, p, ignore_white);
    }
    g_assert (p != NULL);
  }

  if (xml->status == XML_PARSE_STATUS_OK && node != SWFDEC_XML_NODE (xml))
    xml->status = XML_PARSE_STATUS_TAG_NOT_CLOSED;
}

// this is an old XML parsing function that is only available trough the
// ASnative code
SWFDEC_AS_NATIVE (300, 0, swfdec_xml_do_oldParseXML)
void
swfdec_xml_do_oldParseXML (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SWFDEC_STUB ("XML.oldParseXML (not-really-named)");
}

SWFDEC_AS_NATIVE (253, 12, swfdec_xml_do_parseXML)
void
swfdec_xml_do_parseXML (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  swfdec_xml_parseXML (SWFDEC_XML (object),
      swfdec_as_value_to_string (cx, &argv[0]));
}

SWFDEC_AS_NATIVE (253, 10, swfdec_xml_createElement)
void
swfdec_xml_createElement (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecXmlNode *node;

  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  node = swfdec_xml_node_new (cx, SWFDEC_XML_NODE_ELEMENT,
      swfdec_as_value_to_string (cx, &argv[0]));
  if (node == NULL)
    return;

  SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (node));
}

SWFDEC_AS_NATIVE (253, 11, swfdec_xml_createTextNode)
void
swfdec_xml_createTextNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecXmlNode *node;

  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  node = swfdec_xml_node_new (cx, SWFDEC_XML_NODE_TEXT,
      swfdec_as_value_to_string (cx, &argv[0]));
  if (node == NULL)
    return;

  SWFDEC_AS_VALUE_SET_OBJECT (rval, SWFDEC_AS_OBJECT (node));
}

static void
swfdec_xml_init_properties (SwfdecAsContext *cx)
{
  SwfdecAsValue val;
  SwfdecAsObject *xml, *proto;

  // FIXME: We should only initialize if the prototype Object has not been
  // initialized by any object's constructor with native properties
  // (TextField, TextFormat, XML, XMLNode at least)

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (cx));

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_XML, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  xml = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_get_variable (xml, SWFDEC_AS_STR_prototype, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_ignoreWhite,
      swfdec_xml_get_ignoreWhite, swfdec_xml_set_ignoreWhite);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_status,
      swfdec_xml_get_status, swfdec_xml_set_status);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_xmlDecl,
      swfdec_xml_get_xmlDecl, swfdec_xml_set_xmlDecl);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_docTypeDecl,
      swfdec_xml_get_docTypeDecl, swfdec_xml_set_docTypeDecl);

  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_contentType,
      swfdec_xml_get_contentType, swfdec_xml_set_contentType);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_loaded,
      swfdec_xml_get_loaded, swfdec_xml_set_loaded);
}

SWFDEC_AS_CONSTRUCTOR (253, 9, swfdec_xml_construct, swfdec_xml_get_type)
void
swfdec_xml_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!swfdec_as_context_is_constructing (cx))
    return;

  g_assert (SWFDEC_IS_XML (object));

  swfdec_xml_init_properties (cx);

  swfdec_xml_node_init_values (SWFDEC_XML_NODE (object),
      SWFDEC_XML_NODE_ELEMENT, SWFDEC_AS_STR_EMPTY);

  SWFDEC_AS_VALUE_SET_STRING (&SWFDEC_XML (object)->content_type,
      SWFDEC_AS_STR_application_x_www_form_urlencoded);

  SWFDEC_XML_NODE (object)->name = NULL;

  if (!SWFDEC_IS_VALID_XML_NODE (object))
    return;

  if (argc >= 1 && !SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0])) {
    swfdec_xml_parseXML (SWFDEC_XML (object),
	swfdec_as_value_to_string (cx, &argv[0]));
  }
}

SwfdecXml *
swfdec_xml_new_no_properties (SwfdecAsContext *context, const char *str,
    gboolean ignore_white)
{
  SwfdecAsValue val;
  SwfdecXml *xml;
  guint size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  size = sizeof (SwfdecXml);
  swfdec_as_context_use_mem (context, size);
  xml = g_object_new (SWFDEC_TYPE_XML, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (xml), context, size);
  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_XML, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (xml),
	SWFDEC_AS_VALUE_GET_OBJECT (&val));
  }

  xml->ignore_white = ignore_white;

  swfdec_xml_node_init_values (SWFDEC_XML_NODE (xml),
      SWFDEC_XML_NODE_ELEMENT, SWFDEC_AS_STR_EMPTY);

  SWFDEC_AS_VALUE_SET_STRING (&xml->content_type,
      SWFDEC_AS_STR_application_x_www_form_urlencoded);

  if (str != NULL)
    swfdec_xml_parseXML (xml, str);

  return xml;
}

SwfdecXml *
swfdec_xml_new (SwfdecAsContext *context, const char *str,
    gboolean ignore_white)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  swfdec_xml_init_properties (context);

  return swfdec_xml_new_no_properties (context, str, ignore_white);
}
