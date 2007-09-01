/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_xml.h"
#include "swfdec_xml_node.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecXml, swfdec_xml, SWFDEC_TYPE_XML_NODE)

static void
swfdec_xml_do_mark (SwfdecAsObject *object)
{
  SwfdecXml *xml = SWFDEC_XML (object);

  if (xml->xmlDecl != NULL)
    swfdec_as_string_mark (xml->xmlDecl);
  if (xml->docTypeDecl != NULL)
    swfdec_as_string_mark (xml->docTypeDecl);

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
  { '\0', NULL }
};

char *
swfdec_xml_escape (const char *orginal)
{
  int i;
  const char *p, *start;
  GString *string;

  string = g_string_new ("");

  p = start = orginal;
  while (*(p += strcspn (p, "&<>\"'")) != '\0') {
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
  string = g_string_append (string, start);

  return g_string_free (string, FALSE);
}

char *
swfdec_xml_unescape (const char *orginal)
{
  int i;
  const char *p, *start;
  GString *string;

  string = g_string_new ("");

  p = start = orginal;
  while ((p = strchr (p, '&')) != NULL) {
    string = g_string_append_len (string, start, p - start);

    for (i = 0; xml_entities[i].escaped != NULL; i++) {
      if (!g_ascii_strncasecmp (p, xml_entities[i].escaped,
	    strlen (xml_entities[i].escaped))) {
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
  string = g_string_append (string, start);

  return g_string_free (string, FALSE);
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

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret, SWFDEC_XML (object)->ignoreWhite);
}

static void
swfdec_xml_set_ignoreWhite (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  SWFDEC_XML (object)->ignoreWhite = swfdec_as_value_to_boolean (cx, &argv[0]);
}

static void
swfdec_xml_get_xmlDecl (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (SWFDEC_XML (object)->xmlDecl != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_XML (object)->xmlDecl);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
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

  SWFDEC_XML (object)->xmlDecl = swfdec_as_value_to_string (cx, &argv[0]);
}

static void
swfdec_xml_get_docTypeDecl (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (SWFDEC_XML (object)->docTypeDecl != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_XML (object)->docTypeDecl);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
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

  SWFDEC_XML (object)->docTypeDecl = swfdec_as_value_to_string (cx, &argv[0]);
}

static void
swfdec_xml_get_contentType (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  *ret = SWFDEC_XML (object)->contentType;
}

static void
swfdec_xml_set_contentType (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  SWFDEC_XML (object)->contentType = argv[0];
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

  SWFDEC_XML (object)->loaded = argv[0];
}

static void
swfdec_xml_get_status (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML (object))
    return;

  SWFDEC_AS_VALUE_SET_INT (ret, SWFDEC_XML (object)->status);
}

static const char *
swfdec_xml_parse_xmlDecl (SwfdecXml *xml, const char *p)
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

  string = g_string_new (xml->xmlDecl);
  string = g_string_append_len (string, p, end - p);
  xml->xmlDecl = swfdec_as_context_give_string (SWFDEC_AS_OBJECT (xml)->context,
	g_string_free (string, FALSE));

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_docTypeDecl (SwfdecXml *xml, const char *p)
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
    xml->docTypeDecl = swfdec_as_context_give_string (
	SWFDEC_AS_OBJECT (xml)->context, g_strndup (p, end - p));
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

static const char *
swfdec_xml_parse_attribute (SwfdecXml *xml, SwfdecXmlNode *node, const char *p)
{
  SwfdecAsValue val;
  const char *end, *name, *value;
  char *text, *unescaped;

  g_return_val_if_fail (SWFDEC_IS_XML (xml), strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_XML_NODE (node), strchr (p, '\0'));
  g_return_val_if_fail ((*p != '>' && *p != '\0'), p);

  end = p + strcspn (p, "=> \r\n\t");
  if (end - p <= 0) {
    xml->status = XML_PARSE_STATUS_ELEMENT_MALFORMED;
    return strchr (p, '\0');
  }

  name = swfdec_as_context_give_string (SWFDEC_AS_OBJECT (node)->context,
      g_strndup (p, end - p));

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

  text = g_strndup (p + 1, end - (p + 1));
  unescaped = swfdec_xml_unescape (text);
  g_free (text);
  value = swfdec_as_context_give_string (SWFDEC_AS_OBJECT (node)->context,
      unescaped);
  SWFDEC_AS_VALUE_SET_STRING (&val, value);

  swfdec_as_object_set_variable (node->attributes, name, &val);

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
    child = swfdec_xml_node_new (SWFDEC_AS_OBJECT (*node)->context,
	SWFDEC_XML_NODE_ELEMENT, name);
    g_free (name);
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
    swfdec_xml_node_appendChild (*node, child);

    if (*(end - 1) != '/')
      *node = child;
  }

  end += 1;

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_text (SwfdecXml *xml, SwfdecXmlNode *node,
    const char *p)
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

  text = g_strndup (p, end - p);
  unescaped = swfdec_xml_unescape (text);
  g_free (text);
  child = swfdec_xml_node_new (SWFDEC_AS_OBJECT (node)->context,
      SWFDEC_XML_NODE_TEXT, unescaped);
  g_free (unescaped);
  swfdec_xml_node_appendChild (node, child);

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static void
swfdec_xml_parseXML (SwfdecXml *xml, const char *value)
{
  SwfdecAsObject *object;
  SwfdecXmlNode *node;
  const char *p;

  g_return_if_fail (SWFDEC_IS_XML (xml));
  g_return_if_fail (value != NULL);

  object = SWFDEC_AS_OBJECT (xml);

  swfdec_xml_node_removeChildren (SWFDEC_XML_NODE (xml));
  xml->xmlDecl = SWFDEC_AS_STR_EMPTY;
  xml->docTypeDecl = SWFDEC_AS_STR_EMPTY;
  xml->status = XML_PARSE_STATUS_OK;

  p = value;
  node = SWFDEC_XML_NODE (xml);

  while (xml->status == XML_PARSE_STATUS_OK && *p != '\0') {
    if (*p == '<') {
      if (g_ascii_strncasecmp (p + 1, "?xml", strlen ("?xml")) == 0) {
	p = swfdec_xml_parse_xmlDecl (xml, p);
      } else if (g_ascii_strncasecmp (p + 1, "!DOCTYPE", strlen ("!DOCTYPE")) == 0) {
	p = swfdec_xml_parse_docTypeDecl (xml, p);
      } else if (strncmp (p + 1, "!--", strlen ("!--")) == 0) {
	p = swfdec_xml_parse_comment (xml, p);
      } else {
	p = swfdec_xml_parse_tag (xml, &node, p);
      }
    } else {
      p = swfdec_xml_parse_text (xml, node, p);
    }
    g_assert (p != NULL);
  }

  if (xml->status == XML_PARSE_STATUS_OK && node != SWFDEC_XML_NODE (xml))
    xml->status = XML_PARSE_STATUS_TAG_NOT_CLOSED;
}

SWFDEC_AS_NATIVE (253, 10, swfdec_xml_do_parseXML)
void
swfdec_xml_do_parseXML (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  if (!SWFDEC_IS_XML (object))
    return;

  if (argc < 1)
    return;

  swfdec_xml_parseXML (SWFDEC_XML (object),
      swfdec_as_value_to_string (cx, &argv[0]));
}

SWFDEC_AS_CONSTRUCTOR (253, 9, swfdec_xml_construct, swfdec_xml_get_type)
void
swfdec_xml_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  g_assert (SWFDEC_IS_XML (object));

  swfdec_xml_node_init_properties (SWFDEC_XML_NODE (object),
      SWFDEC_XML_NODE_ELEMENT, SWFDEC_AS_STR_EMPTY);

  swfdec_as_object_unset_variable_flags (object, SWFDEC_AS_STR___constructor__,
      SWFDEC_AS_VARIABLE_HIDDEN);

  SWFDEC_AS_VALUE_SET_STRING (&SWFDEC_XML (object)->contentType,
      SWFDEC_AS_STR_application_x_www_form_urlencoded);

  SWFDEC_XML_NODE (object)->name = NULL;

  if (argc >= 1) {
    swfdec_xml_parseXML (SWFDEC_XML (object),
	swfdec_as_value_to_string (cx, &argv[0]));
  }
}

static void
swfdec_xml_add_variable (SwfdecAsObject *object, const char *variable,
    SwfdecAsNative get, SwfdecAsNative set)
{
  SwfdecAsFunction *get_func, *set_func;

  g_return_if_fail (SWFDEC_IS_AS_OBJECT (object));
  g_return_if_fail (variable != NULL);
  g_return_if_fail (get != NULL);

  get_func =
    swfdec_as_native_function_new (object->context, variable, get, 0, NULL);
  if (get_func == NULL)
    return;

  if (set != NULL) {
    set_func =
      swfdec_as_native_function_new (object->context, variable, set, 0, NULL);
  } else {
    set_func = NULL;
  }

  swfdec_as_object_add_variable (object, variable, get_func, set_func);
}

void
swfdec_xml_init_native (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context;
  SwfdecAsValue val;
  SwfdecAsObject *xml;
  SwfdecAsObject *proto;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  context = SWFDEC_AS_CONTEXT (player);
  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_XML, &val);
  g_return_if_fail (SWFDEC_AS_VALUE_IS_OBJECT (&val));
  xml = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_get_variable (xml, SWFDEC_AS_STR_prototype, &val);
  g_return_if_fail (SWFDEC_AS_VALUE_IS_OBJECT (&val));
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_xml_add_variable (proto, SWFDEC_AS_STR_ignoreWhite,
      swfdec_xml_get_ignoreWhite, swfdec_xml_set_ignoreWhite);
  swfdec_xml_add_variable (proto, SWFDEC_AS_STR_status, swfdec_xml_get_status,
      NULL);
  swfdec_xml_add_variable (proto, SWFDEC_AS_STR_xmlDecl, swfdec_xml_get_xmlDecl,
      swfdec_xml_set_xmlDecl);
  swfdec_xml_add_variable (proto, SWFDEC_AS_STR_docTypeDecl,
      swfdec_xml_get_docTypeDecl, swfdec_xml_set_docTypeDecl);

  swfdec_xml_add_variable (proto, SWFDEC_AS_STR_contentType,
      swfdec_xml_get_contentType, swfdec_xml_set_contentType);
  swfdec_xml_add_variable (proto, SWFDEC_AS_STR_loaded,
      swfdec_xml_get_loaded, swfdec_xml_set_loaded);
}
