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

G_DEFINE_TYPE (SwfdecXml, swfdec_xml, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_xml_class_init (SwfdecXmlClass *klass)
{
}

static void
swfdec_xml_init (SwfdecXml *xml)
{
}

typedef enum {
  XML_PARSE_ERROR_NONE = 0,
  XML_PARSE_ERROR_CDATA_NOT_TERMINATED = -2,
  XML_PARSE_ERROR_XMLDECL_NOT_TERMINATED = -3,
  XML_PARSE_ERROR_DOCTYPEDECL_NOT_TERMINATED = -4,
  XML_PARSE_ERROR_COMMENT_NOT_TERMINATED = -5,
  XML_PARSE_ERROR_ELEMENT_MALFORMED = -6,
  XML_PARSE_ERROR_OUT_OF_MEMORY = -7,
  XML_PARSE_ERROR_ATTRIBUTE_NOT_TERMINATED = -8,
  XML_PARSE_ERROR_TAG_NOT_CLOSED = -9, // FIXME are the two correct?
  XML_PARSE_ERROR_TAG_MISMATCH = -10
} XmlParseError;

static const char *
swfdec_xml_parse_xmlDecl (SwfdecAsObject *object, const char *p)
{
  SwfdecAsValue val;
  const char *end;
  GString *string;

  g_assert (p != NULL);
  g_return_val_if_fail (g_ascii_strncasecmp (p, "<?xml", strlen ("<?xml")) == 0,
      strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), strchr (p, '\0'));

  end = strstr (p, "?>");
  if (end == NULL) {
    SWFDEC_AS_VALUE_SET_INT (&val, XML_PARSE_ERROR_XMLDECL_NOT_TERMINATED);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_status, &val);
    return strchr (p, '\0');
  }

  end += strlen ("?>");

  swfdec_as_object_get_variable (object, SWFDEC_AS_STR_docTypeDecl, &val);
  string = g_string_new (swfdec_as_value_to_string (object->context, &val));
  string = g_string_append_len (string, p, end - p);
  SWFDEC_AS_VALUE_SET_STRING (&val,
      swfdec_as_context_give_string (object->context,
	g_string_free (string, FALSE)));
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_xmlDecl, &val);

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_docTypeDecl (SwfdecAsObject *object, const char *p)
{
  SwfdecAsValue val;
  const char *end;
  int open;

  g_assert (p != NULL);
  g_return_val_if_fail (
      g_ascii_strncasecmp (p, "<!DOCTYPE", strlen ("<!DOCTYPE")) == 0,
      strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), strchr (p, '\0'));

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
    SWFDEC_AS_VALUE_SET_INT (&val, XML_PARSE_ERROR_DOCTYPEDECL_NOT_TERMINATED);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_status, &val);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (&val,
	swfdec_as_context_give_string (object->context,
	  g_strndup (p, end - p)));
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_docTypeDecl, &val);
  }

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_comment (SwfdecAsObject *object, const char *p)
{
  const char *end;

  g_assert (p != NULL);
  g_return_val_if_fail (strncmp (p, "<!--", strlen ("<!--")) == 0,
      strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), strchr (p, '\0'));

  end = strstr (p, "-->");

  if (end == NULL) {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_INT (&val, XML_PARSE_ERROR_COMMENT_NOT_TERMINATED);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_status, &val);
    return strchr (p, '\0');
  }

  end += strlen("-->");

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_tag (SwfdecAsObject *object, SwfdecXmlNode **node,
    const char *p)
{
  SwfdecXmlNode *child;
  char *name;
  const char *end;

  g_assert (p != NULL);
  g_return_val_if_fail (*p == '<', strchr (p, '\0'));
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), strchr (p, '\0'));

  end = p + strcspn (p, " />");

  if (end - (p + 1) <= 0 || *end == '\0') {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_INT (&val, XML_PARSE_ERROR_ELEMENT_MALFORMED);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_status, &val);
    return strchr (p, '\0');
  }

  name = g_strndup (p + 1 , end - (p + 1));
  child = swfdec_xml_node_new (SWFDEC_AS_OBJECT (*node)->context,
      SWFDEC_XML_NODE_ELEMENT, name);
  swfdec_xml_node_appendChild (*node, child);
  g_free (name);

  end = strchr (end, '>');
  if (end == NULL) {
    SwfdecAsValue val;
    SWFDEC_AS_VALUE_SET_INT (&val, XML_PARSE_ERROR_ELEMENT_MALFORMED);
    swfdec_as_object_set_variable (object, SWFDEC_AS_STR_status, &val);
    return strchr (p, '\0');
  }

  end += 1;

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

static const char *
swfdec_xml_parse_text (SwfdecAsObject *object, const char *p)
{
  const char *end;

  g_assert (p != NULL);
  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (object), strchr (p, '\0'));

  end = strchr (p, '<');
  if (end == NULL)
    return strchr (p, '\0');

  /*tmp = g_strndup (p, end - p);
  swfdec_xml_node_new (object, SWFDEC_XML_NODE_TYPE_TEXT, tmp);
  g_free (tmp);*/

  g_return_val_if_fail (end > p, strchr (p, '\0'));

  return end;
}

SWFDEC_AS_NATIVE (253, 10, swfdec_xml_parseXML)
void
swfdec_xml_parseXML (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  SwfdecAsValue val;
  SwfdecXmlNode *node;
  const char *value, *p;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  SWFDEC_AS_VALUE_SET_STRING (&val, SWFDEC_AS_STR_EMPTY);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_xmlDecl, &val);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_docTypeDecl, &val);
  SWFDEC_AS_VALUE_SET_INT (&val, XML_PARSE_ERROR_NONE);
  swfdec_as_object_set_variable (object, SWFDEC_AS_STR_status, &val);

  value = swfdec_as_value_to_string (cx, &argv[0]);

  p = value;
  node = SWFDEC_XML_NODE (object);

  while (*p != '\0') {
    if (*p == '<') {
      if (g_ascii_strncasecmp (p + 1, "?xml", strlen ("?xml")) == 0) {
	p = swfdec_xml_parse_xmlDecl (object, p);
      } else if (g_ascii_strncasecmp (p + 1, "!DOCTYPE", strlen ("!DOCTYPE")) == 0) {
	p = swfdec_xml_parse_docTypeDecl (object, p);
      } else if (strncmp (p + 1, "!--", strlen ("!--")) == 0) {
	p = swfdec_xml_parse_comment (object, p);
      } else {
	p = swfdec_xml_parse_tag (object, &node, p);
      }
    } else {
      p = swfdec_xml_parse_text (object, p);
    }
    g_assert (p != NULL);
  }
}
