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
swfdec_xml_class_init (SwfdecXmlClass *klass)
{
}

static void
swfdec_xml_init (SwfdecXml *xml)
{
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

  value = swfdec_as_context_give_string (SWFDEC_AS_OBJECT (node)->context,
      g_strndup (p + 1, end - (p + 1)));
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
    g_free (name);
    return end;
  }

  if (close) {
    if ((*node)->parent == NULL || g_ascii_strcasecmp ((*node)->name, name))
      xml->status = XML_PARSE_STATUS_TAG_MISMATCH;
    while ((*node)->parent != NULL &&
	((*node)->type != SWFDEC_XML_NODE_ELEMENT ||
	  g_ascii_strcasecmp ((*node)->name, name))) {
      *node = (*node)->parent;
    }
    if ((*node)->parent != NULL)
      *node = (*node)->parent;
  } else {
    swfdec_xml_node_appendChild (*node, child);
  }
  g_free (name);

  if (!close) {
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
  char *text;

  g_assert (p != NULL);
  g_return_val_if_fail (*p != '\0', p);
  g_return_val_if_fail (SWFDEC_IS_XML (xml), strchr (p, '\0'));

  end = strchr (p, '<');
  if (end == NULL)
    end = strchr (p, '\0');

  text = g_strndup (p, end - p);
  child = swfdec_xml_node_new (SWFDEC_AS_OBJECT (node)->context,
      SWFDEC_XML_NODE_TEXT, text);
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

  g_return_if_fail (SWFDEC_IS_XML_NODE (xml));
  g_return_if_fail (value != NULL);

  object = SWFDEC_AS_OBJECT (xml);

  xml->xmlDecl = SWFDEC_AS_STR_EMPTY;
  xml->docTypeDecl = SWFDEC_AS_STR_EMPTY;
  xml->status = XML_PARSE_STATUS_OK;

  p = value;
  node = SWFDEC_XML_NODE (xml);

  while (*p != '\0') {
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

  if (node != SWFDEC_XML_NODE (xml))
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

static void
swfdec_xml_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsValue vals[2];

  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  g_assert (SWFDEC_IS_XML (object));

  SWFDEC_AS_VALUE_SET_INT (&vals[0], SWFDEC_XML_NODE_ELEMENT);
  SWFDEC_AS_VALUE_SET_STRING (&vals[1], SWFDEC_AS_STR_EMPTY);

  swfdec_xml_node_construct (cx, object, 2, vals, ret);
  SWFDEC_XML_NODE (object)->name = NULL;
  if (argc >= 1) {
    swfdec_xml_parseXML (SWFDEC_XML (object),
	swfdec_as_value_to_string (cx, &argv[0]));
  }
}

void
swfdec_xml_init_context (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context;
  SwfdecAsObject *xml;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  context = SWFDEC_AS_CONTEXT (player);
  xml = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_XML, 0, swfdec_xml_construct, 0));
  if (!xml)
    return;
  swfdec_as_native_function_set_construct_type (
      SWFDEC_AS_NATIVE_FUNCTION (xml), SWFDEC_TYPE_XML);
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
swfdec_xml_init_context2 (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context;
  SwfdecAsValue val;
  SwfdecAsObject *xml;
  SwfdecAsObject *proto;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  context = SWFDEC_AS_CONTEXT (player);
  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_XML, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  xml = SWFDEC_AS_VALUE_GET_OBJECT (&val);
  if (!xml)
    return;
  swfdec_as_object_get_variable (xml, SWFDEC_AS_STR_prototype, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);
  if (!proto)
    return;

  swfdec_xml_add_variable (proto, SWFDEC_AS_STR_status, swfdec_xml_get_status,
      NULL);
}
