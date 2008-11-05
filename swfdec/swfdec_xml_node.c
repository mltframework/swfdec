/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#include <string.h>

#include "swfdec_xml_node.h"
#include "swfdec_xml.h"
#include "swfdec_as_array.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_load_object.h"
#include "swfdec_player_internal.h"

G_DEFINE_TYPE (SwfdecXmlNode, swfdec_xml_node, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_xml_node_mark (SwfdecGcObject *object)
{
  SwfdecXmlNode *node = SWFDEC_XML_NODE (object);

  if (node->name != NULL)
    swfdec_as_string_mark (node->name);
  if (node->value != NULL)
    swfdec_as_string_mark (node->value);
  if (node->parent != NULL)
    swfdec_gc_object_mark (node->parent);
  if (node->children != NULL)
    swfdec_as_object_mark (node->children);
  if (node->attributes != NULL)
    swfdec_as_object_mark (node->attributes);
  if (node->child_nodes != NULL)
    swfdec_as_object_mark (node->child_nodes);

  SWFDEC_GC_OBJECT_CLASS (swfdec_xml_node_parent_class)->mark (object);
}

static void
swfdec_xml_node_class_init (SwfdecXmlNodeClass *klass)
{
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (klass);

  gc_class->mark = swfdec_xml_node_mark;
}

static void
swfdec_xml_node_init (SwfdecXmlNode *xml_node)
{
}

/*** AS CODE ***/

gint32
swfdec_xml_node_num_children (SwfdecXmlNode *node)
{
  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), 0);

  return swfdec_as_array_get_length (node->children);
}

SwfdecXmlNode *
swfdec_xml_node_get_child (SwfdecXmlNode *node, gint32 index_)
{
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), NULL);
  g_return_val_if_fail (index_ >= 0, NULL);

  if (index_ >= swfdec_xml_node_num_children (node))
    return NULL;

  swfdec_as_array_get_value (node->children, index_, &val);

  g_return_val_if_fail (SWFDEC_AS_VALUE_IS_OBJECT (&val), NULL);
  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (
	SWFDEC_AS_VALUE_GET_OBJECT (&val)->relay), NULL);

  return SWFDEC_XML_NODE (SWFDEC_AS_VALUE_GET_OBJECT (&val)->relay);
}

static gint32
swfdec_xml_node_index_of_child (SwfdecXmlNode *node, SwfdecXmlNode *child)
{
  gint32 num, i;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), -1);
  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (child), -1);

  num = swfdec_xml_node_num_children (node);
  for (i = 0; i < num; i++) {
    if (swfdec_xml_node_get_child (node, i) == child)
      return i;
  }

  return -1;
}

static void
swfdec_xml_node_update_child_nodes (SwfdecXmlNode *node)
{
  SwfdecAsValue val;
  SwfdecAsValue *vals;
  gint32 num, i;

  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (node));

  // remove old
  swfdec_as_value_set_integer (swfdec_gc_object_get_context (node), &val, 0);
  swfdec_as_object_set_variable (node->child_nodes,
      SWFDEC_AS_STR_length, &val);

  // add everything
  num = swfdec_xml_node_num_children (node);
  vals = g_malloc (sizeof (SwfdecAsValue) * num);
  for (i = 0; i < num; i++) {
    SWFDEC_AS_VALUE_SET_OBJECT (&vals[i], swfdec_as_relay_get_as_object (
	  SWFDEC_AS_RELAY (swfdec_xml_node_get_child (node, i))));
  }
  swfdec_as_array_append_with_flags (node->child_nodes, num, vals,
      SWFDEC_AS_VARIABLE_CONSTANT);
  g_free (vals);
}

const char *
swfdec_xml_node_get_attribute (SwfdecXmlNode *node, const char *name)
{
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  if (swfdec_as_object_get_variable (node->attributes, name, &val)) {
    return swfdec_as_value_to_string (swfdec_gc_object_get_context (node), &val);
  } else {
    return NULL;
  }
}

static const char *
swfdec_xml_node_getNamespaceForPrefix (SwfdecXmlNode *node, const char *prefix)
{
  const char *var;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), NULL);

  if (prefix == NULL || strlen (prefix) == 0) {
    var = swfdec_as_context_get_string (swfdec_gc_object_get_context (node),
	"xmlns");
  } else {
    var = swfdec_as_context_give_string (swfdec_gc_object_get_context (node),
	g_strconcat ("xmlns:", prefix, NULL));
  }

  do {
    swfdec_as_object_get_variable (node->attributes, var, &val);
    if (!SWFDEC_AS_VALUE_IS_UNDEFINED (&val)) {
      return swfdec_as_value_to_string (swfdec_gc_object_get_context (node), &val);
    }
    node = node->parent;
  } while (node != NULL);

  return NULL;
}

typedef struct {
  const char	*namespace;
  const char	*variable;
} ForeachFindNamespaceData;

static gboolean
swfdec_xml_node_foreach_find_namespace (SwfdecAsObject *object,
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data)
{
  const char *uri;
  ForeachFindNamespaceData *fdata = data;

  // check whether it's namespace variable (xmlns or xmlns:*)
  if (strlen (variable) < strlen("xmlns"))
    return TRUE;

  if (g_ascii_strncasecmp (variable, "xmlns", strlen("xmlns")))
    return TRUE;

  if (variable[strlen("xmlns")] != '\0' && variable[strlen("xmlns")] != ':')
    return TRUE;

  // ok, now check if the uri is the one we are searching for
  uri = swfdec_as_value_to_string (object->context, value);
  if (!g_ascii_strcasecmp (uri, fdata->namespace)) {
    fdata->variable = variable;
    return FALSE;
  } else {
    return TRUE;
  }
}

static const char *
swfdec_xml_node_getPrefixForNamespace (SwfdecXmlNode *node,
    const char *namespace)
{
  ForeachFindNamespaceData fdata;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), NULL);
  g_return_val_if_fail (namespace != NULL, NULL);

  fdata.namespace = namespace;
  fdata.variable = NULL;

  do {
    swfdec_as_object_foreach (node->attributes,
	swfdec_xml_node_foreach_find_namespace, &fdata);
    node = node->parent;
  } while (node != NULL && fdata.variable == NULL);

  if (fdata.variable != NULL) {
    const char *p;

    p = strchr (fdata.variable, ':');
    if (p == NULL || *(p + 1) == '\0')
      return SWFDEC_AS_STR_EMPTY;

    return swfdec_as_context_get_string (swfdec_gc_object_get_context (node),
	p + 1);
  } else {
    return NULL;
  }
}

static void
swfdec_xml_node_get_nodeType (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  swfdec_as_value_set_integer (cx, ret, node->type);
}

static void
swfdec_xml_node_get_nodeValue (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  if (node->value != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, node->value);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_set_nodeValue (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;
  const char *value;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "s", &value);

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  node->value = value;
  SWFDEC_AS_VALUE_SET_STRING (ret, value);
}

static void
swfdec_xml_node_get_nodeName (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  if (node->name != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, node->name);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_set_nodeName (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;
  const char *name;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "s", &name);

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  // special case
  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]))
    return;

  node->name = name;
  SWFDEC_AS_VALUE_SET_STRING (ret, name);
}

static const char *
swfdec_xml_node_get_prefix (SwfdecXmlNode *node)
{
  const char *p;

  g_return_val_if_fail (SWFDEC_IS_VALID_XML_NODE (node), NULL);

  if (node->name == NULL)
    return NULL;

  p = strchr (node->name, ':');
  if (p == NULL || *(p + 1) == '\0')
    return NULL;

  return swfdec_as_context_give_string (swfdec_gc_object_get_context (node),
      g_strndup (node->name, p - node->name));
}

static void
swfdec_xml_node_do_get_prefix (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *prefix;
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  if (node->name == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  prefix = swfdec_xml_node_get_prefix (node);
  if (prefix != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, prefix);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
  }
}

static void
swfdec_xml_node_get_localName (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *p, *name;
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  if (node->name == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  name = node->name;
  p = strchr (name, ':');
  if (p == NULL || *(p + 1) == '\0') {
    SWFDEC_AS_VALUE_SET_STRING (ret, name);
    return;
  }
  p++;

  SWFDEC_AS_VALUE_SET_STRING (ret,
      swfdec_as_context_get_string (cx, p));
}

static void
swfdec_xml_node_get_namespaceURI (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *uri;
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  if (node->name == NULL) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  uri = swfdec_xml_node_getNamespaceForPrefix (node,
      swfdec_xml_node_get_prefix (node));
  if (uri != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, uri);
  } else {
    SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_EMPTY);
  }
}

static void
swfdec_xml_node_get_attributes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  SWFDEC_AS_VALUE_SET_OBJECT (ret, node->attributes);
}

static void
swfdec_xml_node_get_parentNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  if (node->parent != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, 
	swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (node->parent)));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static SwfdecXmlNode *
swfdec_xml_node_previousSibling (SwfdecXmlNode *node)
{
  gint32 i;

  if (node->parent == NULL)
    return NULL;

  i = swfdec_xml_node_index_of_child (node->parent, node);
  g_assert (i >= 0);

  if (i <= 0)
    return NULL;

  return swfdec_xml_node_get_child (node->parent, i - 1);
}

static void
swfdec_xml_node_get_previousSibling (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *sibling;
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  sibling = swfdec_xml_node_previousSibling (node);
  if (sibling != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (sibling)));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static SwfdecXmlNode *
swfdec_xml_node_nextSibling (SwfdecXmlNode *node)
{
  gint32 i;

  if (node->parent == NULL)
    return NULL;

  i = swfdec_xml_node_index_of_child (node->parent, node);
  g_assert (i >= 0);

  return swfdec_xml_node_get_child (node->parent, i + 1);
}

static void
swfdec_xml_node_get_nextSibling (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *sibling;
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  sibling = swfdec_xml_node_nextSibling (node);
  if (sibling != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (sibling)));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_get_firstChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *child;
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  child = swfdec_xml_node_get_child (node, 0);
  if (child != NULL) {
    SWFDEC_AS_VALUE_SET_OBJECT (ret, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (child)));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static void
swfdec_xml_node_get_lastChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  gint32 num;
  SwfdecXmlNode *child;
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  num = swfdec_xml_node_num_children (node);
  if (num == 0) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  child = swfdec_xml_node_get_child (node, num - 1);
  g_assert (child != NULL);

  SWFDEC_AS_VALUE_SET_OBJECT (ret, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (child)));
}

static void
swfdec_xml_node_get_childNodes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  SWFDEC_AS_VALUE_SET_OBJECT (ret, node->child_nodes);
}

SWFDEC_AS_NATIVE (253, 7, swfdec_xml_node_do_getNamespaceForPrefix)
void
swfdec_xml_node_do_getNamespaceForPrefix (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *namespace, *prefix;
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "|s", &prefix);

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  if (argc < 1) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  namespace = swfdec_xml_node_getNamespaceForPrefix (node, prefix);

  if (namespace != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, namespace);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

SWFDEC_AS_NATIVE (253, 8, swfdec_xml_node_do_getPrefixForNamespace)
void
swfdec_xml_node_do_getPrefixForNamespace (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  const char *namespace, *prefix;
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "|s", &namespace);

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  if (argc < 1) {
    SWFDEC_AS_VALUE_SET_NULL (ret);
    return;
  }

  prefix = swfdec_xml_node_getPrefixForNamespace (node, namespace);

  if (prefix != NULL) {
    SWFDEC_AS_VALUE_SET_STRING (ret, prefix);
  } else {
    SWFDEC_AS_VALUE_SET_NULL (ret);
  }
}

static gboolean
swfdec_xml_node_foreach_copy_attributes (SwfdecAsObject *object,
    const char *variable, SwfdecAsValue *value, guint flags, gpointer data)
{
  SwfdecAsObject *target = data;
  swfdec_as_object_set_variable (target, variable, value);
  return TRUE;
}

static void
swfdec_xml_node_copy_attributes (SwfdecXmlNode *node, SwfdecXmlNode *target)
{
  swfdec_as_object_foreach (node->attributes,
      swfdec_xml_node_foreach_copy_attributes, target->attributes);
}

static SwfdecXmlNode *
swfdec_xml_node_clone (SwfdecAsContext *cx, SwfdecXmlNode *node, gboolean deep)
{
  SwfdecXmlNode *new;

  g_assert (SWFDEC_IS_AS_CONTEXT (cx));
  g_assert (SWFDEC_IS_VALID_XML_NODE (node));

  new = swfdec_xml_node_new (cx, SWFDEC_XML_NODE_ELEMENT, SWFDEC_AS_STR_EMPTY);

  new->valid = TRUE;
  new->type = node->type;
  new->name = node->name;
  new->value = node->value;

  swfdec_xml_node_copy_attributes (node, new);

  if (deep) {
    SwfdecAsValue val;
    SwfdecXmlNode *child, *child_new;
    gint32 num, i;

    num = swfdec_xml_node_num_children (node);

    for (i = 0; i < num; i++) {
      child = swfdec_xml_node_get_child (node, i);
      child_new = swfdec_xml_node_clone (cx, child, TRUE);
      child_new->parent = new;
      SWFDEC_AS_VALUE_SET_OBJECT (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (child_new)));
      swfdec_as_array_push (new->children, &val);
    }

    swfdec_xml_node_update_child_nodes (new);
  }

  return new;
}

SWFDEC_AS_NATIVE (253, 1, swfdec_xml_node_cloneNode)
void
swfdec_xml_node_cloneNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  gboolean deep = FALSE;
  SwfdecXmlNode *new, *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "|b", &deep);

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  new = swfdec_xml_node_clone (cx, node, deep);

  SWFDEC_AS_VALUE_SET_OBJECT (ret, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (new)));
}

void
swfdec_xml_node_removeNode (SwfdecXmlNode *node)
{
  gint32 i;

  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (node));

  if (node->parent == NULL)
    return;

  i = swfdec_xml_node_index_of_child (node->parent, node);
  g_assert (i >= 0);

  swfdec_as_array_remove (node->parent->children, i);
  swfdec_xml_node_update_child_nodes (node->parent);
  node->parent = NULL;
}

SWFDEC_AS_NATIVE (253, 2, swfdec_xml_node_do_removeNode)
void
swfdec_xml_node_do_removeNode (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  swfdec_xml_node_removeNode (node);
}

void
swfdec_xml_node_removeChildren (SwfdecXmlNode *node)
{
  gint32 num, i;

  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (node));

  num = swfdec_xml_node_num_children (node);

  for (i = 0; i < num; i++) {
    swfdec_xml_node_removeNode (swfdec_xml_node_get_child (node, 0));
  }
}

static void
swfdec_xml_node_insertAt (SwfdecXmlNode *node, SwfdecXmlNode *child, gint32 ind)
{
  SwfdecAsValue val;

  g_assert (SWFDEC_IS_VALID_XML_NODE (node));
  g_assert (SWFDEC_IS_VALID_XML_NODE (child));
  g_assert (ind >= 0);

  if (swfdec_gc_object_get_context (node)->version >= 8) {
    SwfdecXmlNode *parent = node;
    while (parent != NULL) {
      if (parent == child)
	return;
      parent = parent->parent;
    }
  }

  // remove the previous parent of the child
  swfdec_xml_node_removeNode (child);

  // insert child to node's child_nodes array
  SWFDEC_AS_VALUE_SET_OBJECT (&val, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (child)));
  swfdec_as_array_insert (node->children, ind, &val);
  swfdec_xml_node_update_child_nodes (node);

  // set node as parent of child
  child->parent = node;
}

SWFDEC_AS_NATIVE (253, 3, swfdec_xml_node_insertBefore)
void
swfdec_xml_node_insertBefore (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  gint32 i;
  SwfdecAsObject *child, *point;
  SwfdecXmlNode *node, *child_node, *point_node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "oo", &child, &point);

  if (!SWFDEC_IS_VALID_XML_NODE (node) ||
      !SWFDEC_IS_VALID_XML_NODE (child->relay) ||
      !SWFDEC_IS_VALID_XML_NODE (point->relay))
    return;

  child_node = SWFDEC_XML_NODE (child->relay);
  point_node = SWFDEC_XML_NODE (point->relay);

  // special case
  if (swfdec_xml_node_index_of_child (node, child_node) != -1)
    return;

  i = swfdec_xml_node_index_of_child (node, point_node);

  if (i != -1) {
    swfdec_xml_node_insertAt (node, child_node, i);
  }
}

void
swfdec_xml_node_appendChild (SwfdecXmlNode *node, SwfdecXmlNode *child)
{
  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (node));
  g_return_if_fail (SWFDEC_IS_VALID_XML_NODE (child));
  g_return_if_fail (node->children != NULL);

  swfdec_xml_node_insertAt (node, child,
      swfdec_as_array_get_length (node->children));
}

SWFDEC_AS_NATIVE (253, 4, swfdec_xml_node_do_appendChild)
void
swfdec_xml_node_do_appendChild (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsObject *child;
  SwfdecXmlNode *node, *child_node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "o", &child);

  if (!SWFDEC_IS_VALID_XML_NODE (node) ||
      !SWFDEC_IS_VALID_XML_NODE (child->relay))
    return;

  child_node = SWFDEC_XML_NODE (child->relay);

  // special case
  if (swfdec_xml_node_index_of_child (node, child_node) != -1)
    return;

  swfdec_xml_node_appendChild (node, child_node);
}

SWFDEC_AS_NATIVE (253, 5, swfdec_xml_node_hasChildNodes)
void
swfdec_xml_node_hasChildNodes (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  SWFDEC_AS_VALUE_SET_BOOLEAN (ret,
      swfdec_xml_node_num_children (node) > 0);
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
    swfdec_xml_escape (swfdec_as_value_to_string (object->context, value));
  string = g_string_append (string, escaped);
  g_free (escaped);
  string = g_string_append (string, "\"");

  return TRUE;
}

static const char *
swfdec_xml_node_toString (SwfdecXmlNode *node)
{
  GString *string;

  g_assert (SWFDEC_IS_VALID_XML_NODE (node));

  string = g_string_new ("");
  if (SWFDEC_IS_XML (node)) {
    if (SWFDEC_XML (node)->xml_decl != NULL)
      string = g_string_append (string, SWFDEC_XML (node)->xml_decl);
    if (SWFDEC_XML (node)->doc_type_decl != NULL)
      string = g_string_append (string, SWFDEC_XML (node)->doc_type_decl);
  }

  switch (node->type) {
    case SWFDEC_XML_NODE_ELEMENT:
      {
	SwfdecXmlNode *child;
	gint32 i, num;
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

	num = swfdec_xml_node_num_children (node);

	if (num > 0) {
	  if (visible)
	    string = g_string_append (string, ">");

	  for (i = 0; i < num; i++) {
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
      {
	char *escaped = swfdec_xml_escape (node->value);
	string = g_string_append (string, escaped);
	g_free (escaped);
	break;
      }
  }

  return swfdec_as_context_give_string (swfdec_gc_object_get_context (node),
      g_string_free (string, FALSE));
}

SWFDEC_AS_NATIVE (253, 6, swfdec_xml_node_do_toString)
void
swfdec_xml_node_do_toString (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_XML_NODE, &node, "");

  if (!SWFDEC_IS_VALID_XML_NODE (node))
    return;

  SWFDEC_AS_VALUE_SET_STRING (ret, swfdec_xml_node_toString (node));
}

void
swfdec_xml_node_init_values (SwfdecXmlNode *node, int type, const char* value)
{
  SwfdecAsContext *cx;

  g_return_if_fail (SWFDEC_IS_XML_NODE (node));
  g_return_if_fail (value != NULL);

  cx = swfdec_gc_object_get_context (node);

  node->valid = TRUE;
  node->parent = NULL;
  node->children = swfdec_as_array_new (cx);
  node->attributes = swfdec_as_object_new_empty (cx);
  node->type = type;
  if (node->type == SWFDEC_XML_NODE_ELEMENT) {
    node->name = value;
  } else {
    node->value = value;
  }

  node->child_nodes = swfdec_as_array_new (cx);
}

static void
swfdec_xml_node_init_properties (SwfdecAsContext *cx)
{
  SwfdecAsValue val;
  SwfdecAsObject *node, *proto;

  // FIXME: We should only initialize if the prototype Object has not been
  // initialized by any object's constructor with native properties
  // (TextField, TextFormat, XML, XMLNode at least)

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (cx));

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_XMLNode, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  node = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_get_variable (node, SWFDEC_AS_STR_prototype, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_nodeType,
      swfdec_xml_node_get_nodeType, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_nodeValue,
      swfdec_xml_node_get_nodeValue, swfdec_xml_node_set_nodeValue);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_nodeName,
      swfdec_xml_node_get_nodeName, swfdec_xml_node_set_nodeName);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_prefix,
      swfdec_xml_node_do_get_prefix, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_localName,
      swfdec_xml_node_get_localName, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_namespaceURI,
      swfdec_xml_node_get_namespaceURI, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_attributes,
      swfdec_xml_node_get_attributes, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_parentNode,
      swfdec_xml_node_get_parentNode, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_previousSibling,
      swfdec_xml_node_get_previousSibling, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_nextSibling,
      swfdec_xml_node_get_nextSibling, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_firstChild,
      swfdec_xml_node_get_firstChild, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_lastChild,
      swfdec_xml_node_get_lastChild, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_childNodes,
      swfdec_xml_node_get_childNodes, NULL);
}

SwfdecXmlNode *
swfdec_xml_node_new_no_properties (SwfdecAsContext *context,
    SwfdecXmlNodeType type, const char* value)
{
  SwfdecXmlNode *node;
  SwfdecAsObject *object;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (value != NULL, NULL);

  object = swfdec_as_object_new (context, NULL);
  swfdec_as_object_set_constructor_by_name (object,
      SWFDEC_AS_STR_XMLNode, NULL);

  node = g_object_new (SWFDEC_TYPE_XML_NODE, "context", context, NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (node));
  swfdec_xml_node_init_values (node, type, value);

  return node;
}

SwfdecXmlNode *
swfdec_xml_node_new (SwfdecAsContext *context, SwfdecXmlNodeType type,
    const char* value)
{
  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  swfdec_xml_node_init_properties (context);

  return swfdec_xml_node_new_no_properties (context, type, value);
}

SWFDEC_AS_NATIVE (253, 0, swfdec_xml_node_construct)
void
swfdec_xml_node_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecXmlNode *node;
  const char *value;
  int type;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  SWFDEC_AS_CHECK (0, NULL, "is", &type, &value);

  if (SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[0]) ||
      SWFDEC_AS_VALUE_IS_UNDEFINED (&argv[1]))
    return;

  swfdec_xml_node_init_properties (cx);

  node = g_object_new (SWFDEC_TYPE_XML_NODE, "context", cx, NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (node));
  swfdec_xml_node_init_values (node, type, value);

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}
