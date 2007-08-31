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

#include "swfdec_xml_node.h"
#include "swfdec_xml.h"
#include "swfdec_as_array.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_load_object.h"

G_DEFINE_TYPE (SwfdecXmlNode, swfdec_xml_node, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_xml_node_do_mark (SwfdecAsObject *object)
{
  SwfdecXmlNode *node = SWFDEC_XML_NODE (object);

  if (node->name != NULL)
    swfdec_as_string_mark (node->name);
  if (node->value != NULL)
    swfdec_as_string_mark (node->value);
  if (node->parent != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (node->parent));
  if (node->children != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (node->children));
  if (node->attributes != NULL)
    swfdec_as_object_mark (SWFDEC_AS_OBJECT (node->attributes));

  SWFDEC_AS_OBJECT_CLASS (swfdec_xml_node_parent_class)->mark (object);
}

static void
swfdec_xml_node_class_init (SwfdecXmlNodeClass *klass)
{
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  asobject_class->mark = swfdec_xml_node_do_mark;
}

static void
swfdec_xml_node_init (SwfdecXmlNode *xml_node)
{
}

/*** AS CODE ***/

static SwfdecXmlNode *
swfdec_xml_node_get_child (SwfdecXmlNode *node, gint32 i)
{
  SwfdecAsObject *child;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_XML_NODE (node), NULL);
  g_return_val_if_fail (i >= 0, NULL);

  swfdec_as_array_get_value (node->children, i, &val);

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return NULL;
  child = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  if (!SWFDEC_IS_XML_NODE (child))
    return NULL;

  return SWFDEC_XML_NODE (child);
}

static void
swfdec_xml_node_get_nodeType (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_INT (ret, SWFDEC_XML_NODE (object)->type);
}

static void
swfdec_xml_node_get_nodeValue (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_XML_NODE (object)->value);
}

static void
swfdec_xml_node_set_nodeValue (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *value;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  value = swfdec_as_value_to_string (cx, &argv[0]);

  SWFDEC_XML_NODE (object)->value = value;
  SWFDEC_AS_VALUE_SET_STRING (ret, value);
}

static void
swfdec_xml_node_get_nodeName (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_XML_NODE (object)->name);
}

static void
swfdec_xml_node_set_nodeName (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *name;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  name = swfdec_as_value_to_string (cx, &argv[0]);

  SWFDEC_XML_NODE (object)->name = name;
  SWFDEC_AS_VALUE_SET_STRING (ret, name);
}

static void
swfdec_xml_node_get_prefix (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *p, *prefix, *name;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  name = SWFDEC_XML_NODE (object)->name;
  p = strchr (name, ':');
  if (p == NULL || *(p + 1) == '\0') {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
    return;
  }

  prefix = swfdec_as_context_give_string (cx, g_strndup (name, p - name));
  SWFDEC_AS_VALUE_SET_STRING (ret, prefix);
}

static void
swfdec_xml_node_get_localName (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *p, *name;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  name = SWFDEC_XML_NODE (object)->name;
  p = strchr (name, ':');
  if (p == NULL || *(p + 1) == '\0') {
    SWFDEC_AS_VALUE_SET_STRING (ret, name);
    return;
  }
  p++;

  SWFDEC_AS_VALUE_SET_STRING (ret,
      swfdec_as_context_give_string (cx, g_strdup (p)));
}

static void
swfdec_xml_node_get_attributes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_XML_NODE (object)->attributes);
}

static void
swfdec_xml_node_get_parentNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  if (SWFDEC_XML_NODE (object)->parent != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret,
	SWFDEC_AS_OBJECT (SWFDEC_XML_NODE (object)->parent));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static SwfdecXmlNode *
swfdec_xml_node_previousSibling (SwfdecXmlNode *node)
{
  gint32 length, i;

  if (node->parent == NULL)
    return NULL;

  length = swfdec_as_array_length (node->parent->children);
  for (i = 0; i < length; i++) {
    if (swfdec_xml_node_get_child (node->parent, i) == node) {
      if (i == 0)
	return NULL;
      return swfdec_xml_node_get_child (node->parent, i - 1);
    }
  }

  g_assert_not_reached ();
}

static void
swfdec_xml_node_get_previousSibling (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *sibling;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  sibling = swfdec_xml_node_previousSibling (SWFDEC_XML_NODE (object));
  if (sibling != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (sibling));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static SwfdecXmlNode *
swfdec_xml_node_nextSibling (SwfdecXmlNode *node)
{
  gint32 length, i;

  if (node->parent == NULL)
    return NULL;

  length = swfdec_as_array_length (node->parent->children);
  for (i = 0; i < length; i++) {
    if (swfdec_xml_node_get_child (node->parent, i) == node) {
      if (i + 1 == length)
	return NULL;
      return swfdec_xml_node_get_child (node->parent, i + 1);
    }
  }

  g_assert_not_reached ();
}

static void
swfdec_xml_node_get_nextSibling (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *sibling;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  sibling = swfdec_xml_node_nextSibling (SWFDEC_XML_NODE (object));
  if (sibling != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (sibling));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_get_firstChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *child;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  child = swfdec_xml_node_get_child (SWFDEC_XML_NODE (object), 0);
  if (child != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (child));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_get_lastChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  gint32 length;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  length = swfdec_as_array_length (SWFDEC_XML_NODE (object)->children);
  if (length > 0) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret,
	SWFDEC_AS_OBJECT (
	  swfdec_xml_node_get_child (SWFDEC_XML_NODE (object), length - 1)));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_get_childNodes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_OBJECT (ret,
      SWFDEC_AS_OBJECT (SWFDEC_XML_NODE (object)->children));
}

static SwfdecXmlNode*
swfdec_xml_node_get_parent (SwfdecXmlNode *node)
{
  SwfdecAsValue val;
  SwfdecAsObject *parent;

  g_return_val_if_fail (SWFDEC_IS_XML_NODE (node), NULL);

  swfdec_as_object_get_variable (SWFDEC_AS_OBJECT(node),
      SWFDEC_AS_STR_parentNode, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return NULL;

  parent = SWFDEC_AS_VALUE_GET_OBJECT (&val);
  if (!SWFDEC_IS_XML_NODE (parent))
    return NULL;

  return SWFDEC_XML_NODE (parent);
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

static char *
swfdec_xml_node_escape (const char *orginal)
{
  int i;
  const char *p, *start;
  GString *string;

  string = g_string_new ("");

  p = start = orginal;
  while (*(p += strcspn (p, "&<>\"'")) != '\0') {
    string = g_string_append_len (string, start, p - start);
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

static const char *
swfdec_xml_node_getNamespaceForPrefix (SwfdecXmlNode *node,
    const char *prefix)
{
  GString *string;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_XML_NODE (node), NULL);

  string = g_string_new ("xmlns:");
  string = g_string_append (string, prefix);

  do {
    swfdec_as_object_get_variable (node->attributes, string->str, &val);
    if (!SWFDEC_AS_VALUE_IS_UNDEFINED (&val)) {
      g_string_free (string, TRUE);
      return swfdec_as_value_to_string (SWFDEC_AS_OBJECT (node)->context, &val);
    }
    node = node->parent;
  } while (node != NULL);

  g_string_free (string, TRUE);
  return NULL;
}

SWFDEC_AS_NATIVE (253, 7, swfdec_xml_node_do_getNamespaceForPrefix)
void
swfdec_xml_node_do_getNamespaceForPrefix (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *namespace;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  namespace = swfdec_xml_node_getNamespaceForPrefix (SWFDEC_XML_NODE (object),
      swfdec_as_value_to_string (cx, &argv[0]));

  if (namespace != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, namespace);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

SWFDEC_AS_NATIVE (100, 5, swfdec_xml_node_do_escape)
void
swfdec_xml_node_do_escape (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  char *escaped;

  if (argc < 1)
    return;

  escaped = swfdec_xml_node_escape (swfdec_as_value_to_string (cx, &argv[0]));
  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_as_context_give_string (cx, escaped));
}

SWFDEC_AS_NATIVE (253, 1, swfdec_xml_node_cloneNode)
void
swfdec_xml_node_cloneNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_FIXME ("XMLNode.cloneNode not implemented");
}

void
swfdec_xml_node_removeNode (SwfdecXmlNode *node)
{
  SwfdecXmlNode *parent;

  parent = swfdec_xml_node_get_parent (node);
  if (parent == NULL)
    return;

  SWFDEC_FIXME ("XMLNode.removeNode not implemented");
}

void
swfdec_xml_node_removeChildren (SwfdecXmlNode *node)
{
  gint32 length, i;

  length = swfdec_as_array_length (node->children);

  for (i = 0; i < length; i++) {
    swfdec_xml_node_removeNode (swfdec_xml_node_get_child (node, i));
  }
}

SWFDEC_AS_NATIVE (253, 2, swfdec_xml_node_do_removeNode)
void
swfdec_xml_node_do_removeNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  swfdec_xml_node_removeNode (SWFDEC_XML_NODE (object));
}

SWFDEC_AS_NATIVE (253, 3, swfdec_xml_node_insertBefore)
void
swfdec_xml_node_insertBefore (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_FIXME ("XMLNode.insertBefore not implemented");
}

void
swfdec_xml_node_appendChild (SwfdecXmlNode *node, SwfdecXmlNode *child)
{
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_XML_NODE (node));
  g_return_if_fail (SWFDEC_IS_XML_NODE (child));
  g_return_if_fail (node->children != NULL);

  // remove the previous parent of the child
  swfdec_xml_node_removeNode (child);

  // append child to node's childNodes array
  SWFDEC_AS_VALUE_SET_OBJECT (&val, SWFDEC_AS_OBJECT (child));
  swfdec_as_array_push (node->children, &val);

  // set node as parent of child
  child->parent = node;
}

SWFDEC_AS_NATIVE (253, 4, swfdec_xml_node_do_appendChild)
void
swfdec_xml_node_do_appendChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsObject *child;

  if (!SWFDEC_IS_XML_NODE (object))
    return;

  if (argc < 1)
    return;

  if (!SWFDEC_AS_VALUE_IS_OBJECT (&argv[0]))
    return;

  child = SWFDEC_AS_VALUE_GET_OBJECT (&argv[0]);
  if (!SWFDEC_IS_XML_NODE (child))
    return;

  swfdec_xml_node_appendChild (SWFDEC_XML_NODE (object),
      SWFDEC_XML_NODE (child));
}

SWFDEC_AS_NATIVE (253, 5, swfdec_xml_node_hasChildNodes)
void
swfdec_xml_node_hasChildNodes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  if (swfdec_as_array_length (SWFDEC_XML_NODE (object)->children) > 0) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, TRUE);
  } else {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, FALSE);
  }
}

static gboolean
swfdec_xml_node_foreach_string_append_attribute (SwfdecAsObject *object,
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data)
{
  GString *string = *(GString **)data;
  char *escaped;

  string = g_string_append (string, " ");
  string = g_string_append (string, variable);
  string = g_string_append (string, "=\"");
  escaped =
    swfdec_xml_node_escape (swfdec_as_value_to_string (object->context, value));
  string = g_string_append (string, escaped);
  g_free (escaped);
  string = g_string_append (string, "\"");

  return TRUE;
}

static const char *
swfdec_xml_node_toString (SwfdecXmlNode *node)
{
  GString *string;
  SwfdecAsObject *object;

  g_assert (SWFDEC_IS_XML_NODE (node));

  object = SWFDEC_AS_OBJECT (node);

  string = g_string_new ("");
  if (SWFDEC_IS_XML (node)) {
    string = g_string_append (string, SWFDEC_XML (node)->xmlDecl);
    string = g_string_append (string, SWFDEC_XML (node)->docTypeDecl);
  }

  switch (node->type) {
    case SWFDEC_XML_NODE_ELEMENT:
      {
	SwfdecXmlNode *child;
	gint32 i, length;
	gboolean visible;

	if (node->name == NULL) {
	  visible = FALSE;
	} else {
	  visible = TRUE;
	}

	if (visible) {
	  string = g_string_append (string, "<");
	  string = g_string_append (string, node->name);

	  swfdec_as_object_foreach (node->attributes,
	      swfdec_xml_node_foreach_string_append_attribute, &string);
	}

	length = swfdec_as_array_length (node->children);

	if (length > 0) {
	  if (visible)
	    string = g_string_append (string, ">");

	  for (i = 0; i < length; i++) {
	    child = swfdec_xml_node_get_child (node, i);
	    g_assert (child != NULL);
	    string = g_string_append (string, swfdec_xml_node_toString (child));
	  }

	  if (visible) {
	    string = g_string_append (string, "</");
	    string = g_string_append (string, node->name);
	    string = g_string_append (string, ">");
	  }
	} else {
	  if (visible)
	    string = g_string_append (string, " />");
	}

	break;
      }
    case SWFDEC_XML_NODE_TEXT:
    default:
      string = g_string_append (string, node->value);
      break;
  }

  return swfdec_as_context_give_string (object->context,
      g_string_free (string, FALSE));
}

SWFDEC_AS_NATIVE (253, 6, swfdec_xml_node_do_toString)
void
swfdec_xml_node_do_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!SWFDEC_IS_XML_NODE (object))
    return;

  SWFDEC_AS_VALUE_SET_STRING (ret,
      swfdec_xml_node_toString (SWFDEC_XML_NODE (object)));
}

static void
swfdec_xml_node_init_properties (SwfdecXmlNode *node, int type,
    const char* value)
{
  SwfdecAsObject *object;

  g_return_if_fail (SWFDEC_IS_XML_NODE (node));
  g_return_if_fail (value != NULL);

  object = SWFDEC_AS_OBJECT (node);

  node->parent = NULL;
  // FIXME: use _global.Array constructor?
  node->children = SWFDEC_AS_ARRAY (swfdec_as_array_new (object->context));
  node->attributes = swfdec_as_object_new_empty (object->context);
  node->type = type;
  if (node->type == SWFDEC_XML_NODE_ELEMENT) {
    node->name = value;
  } else {
    node->value = value;
  }
}

/**
 * swfdec_xml_node_new:
 * @player: a #SwfdecPlayer
 * @type: the #SwfdecXmlNodeType
 * @value: initial value of the node
 *
 * Creates a new #SwfdecXmlNode.
 *
 * Returns: The new XML node or %NULL on OOM
 **/
SwfdecXmlNode *
swfdec_xml_node_new (SwfdecAsContext *context, SwfdecXmlNodeType type,
    const char* value)
{
  SwfdecAsValue val;
  SwfdecXmlNode *node;
  guint size;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  size = sizeof (SwfdecXmlNode);
  if (!swfdec_as_context_use_mem (context, size))
    return NULL;
  node = g_object_new (SWFDEC_TYPE_XML_NODE, NULL);
  swfdec_as_object_add (SWFDEC_AS_OBJECT (node), context, size);
  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_XMLNode, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return NULL;
  swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (node), SWFDEC_AS_VALUE_GET_OBJECT (&val));

  swfdec_xml_node_init_properties (node, type,
      swfdec_as_context_get_string (SWFDEC_AS_OBJECT (node)->context, value));

  return node;
}

void
swfdec_xml_node_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  if (!swfdec_as_context_is_constructing (cx)) {
    SWFDEC_FIXME ("What do we do if not constructing?");
    return;
  }

  g_assert (SWFDEC_IS_XML_NODE (object));
  g_assert (argc >= 2);

  swfdec_xml_node_init_properties (SWFDEC_XML_NODE (object),
      swfdec_as_value_to_integer (cx, &argv[0]),
      swfdec_as_value_to_string (cx, &argv[1]));

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}

static void
swfdec_xml_node_add_variable (SwfdecAsObject *object, const char *variable,
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
swfdec_xml_node_init_context (SwfdecPlayer *player, guint version)
{
  SwfdecAsContext *context;
  SwfdecAsObject *xml_node, *proto;
  SwfdecAsValue val;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  context = SWFDEC_AS_CONTEXT (player);
  xml_node = SWFDEC_AS_OBJECT (swfdec_as_object_add_function (context->global,
      SWFDEC_AS_STR_XMLNode, 0, swfdec_xml_node_construct, 2));
  if (!xml_node)
    return;
  swfdec_as_native_function_set_construct_type (
      SWFDEC_AS_NATIVE_FUNCTION (xml_node), SWFDEC_TYPE_XML_NODE);

  proto = swfdec_as_object_new (context);

  /* set the right properties on the XmlNode object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, proto);
  swfdec_as_object_set_variable (xml_node, SWFDEC_AS_STR_prototype, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Function);
  swfdec_as_object_set_variable (xml_node, SWFDEC_AS_STR_constructor, &val);

  /* set the right properties on the XmlNode.prototype object */
  SWFDEC_AS_VALUE_SET_OBJECT (&val, context->Object_prototype);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR___proto__, &val);
  SWFDEC_AS_VALUE_SET_OBJECT (&val, xml_node);
  swfdec_as_object_set_variable (proto, SWFDEC_AS_STR_constructor, &val);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_cloneNode,
      SWFDEC_TYPE_XML_NODE, swfdec_xml_node_cloneNode, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_removeNode,
      SWFDEC_TYPE_XML_NODE, swfdec_xml_node_do_removeNode, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_insertBefore, 0,
      swfdec_xml_node_insertBefore, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_appendChild, 0,
      swfdec_xml_node_do_appendChild, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_hasChildNodes, 0,
      swfdec_xml_node_hasChildNodes, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_getNamespaceForPrefix, 0,
      swfdec_xml_node_do_getNamespaceForPrefix, 0);
  swfdec_as_object_add_function (proto, SWFDEC_AS_STR_toString, 0,
      swfdec_xml_node_do_toString, 0); // FIXME: set type?
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_nodeType,
      swfdec_xml_node_get_nodeType, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_nodeValue,
      swfdec_xml_node_get_nodeValue, swfdec_xml_node_set_nodeValue);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_nodeName,
      swfdec_xml_node_get_nodeName, swfdec_xml_node_set_nodeName);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_prefix,
      swfdec_xml_node_get_prefix, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_localName,
      swfdec_xml_node_get_localName, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_attributes,
      swfdec_xml_node_get_attributes, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_parentNode,
      swfdec_xml_node_get_parentNode, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_previousSibling,
      swfdec_xml_node_get_previousSibling, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_nextSibling,
      swfdec_xml_node_get_nextSibling, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_firstChild,
      swfdec_xml_node_get_firstChild, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_lastChild,
      swfdec_xml_node_get_lastChild, NULL);
  swfdec_xml_node_add_variable (proto, SWFDEC_AS_STR_childNodes,
      swfdec_xml_node_get_childNodes, NULL);
}
