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

#ifndef _SWFDEC_XML_NODE_H_
#define _SWFDEC_XML_NODE_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_script.h>
#include <swfdec/swfdec_player.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_XML_NODE_ELEMENT = 1,
  SWFDEC_XML_NODE_ATTRIBUTE = 2,
  SWFDEC_XML_NODE_TEXT = 3,
  SWFDEC_XML_NODE_CDATA_SECTION = 4,
  SWFDEC_XML_NODE_ENTITY_REFERENCE = 5,
  SWFDEC_XML_NODE_ENTITY = 6,
  SWFDEC_XML_NODE_PROCESSING_INSTRUCTION = 7,
  SWFDEC_XML_NODE_COMMENT = 8,
  SWFDEC_XML_NODE_DOCUMENT = 9,
  SWFDEC_XML_NODE_DOCUMENT_TYPE = 10,
  SWFDEC_XML_NODE_DOCUMENT_FRAGMENT = 11,
  SWFDEC_XML_NODE_NOTATION = 12,
} SwfdecXmlNodeType;

typedef struct _SwfdecXmlNode SwfdecXmlNode;
typedef struct _SwfdecXmlNodeClass SwfdecXmlNodeClass;

#define SWFDEC_TYPE_XML_NODE                    (swfdec_xml_node_get_type())
#define SWFDEC_IS_XML_NODE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_XML_NODE))
#define SWFDEC_IS_XML_NODE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_XML_NODE))
#define SWFDEC_XML_NODE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_XML_NODE, SwfdecXmlNode))
#define SWFDEC_XML_NODE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_XML_NODE, SwfdecXmlNodeClass))
#define SWFDEC_XML_NODE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_XML_NODE, SwfdecXmlNodeClass))

#define SWFDEC_IS_VALID_XML_NODE(obj)           (SWFDEC_IS_XML_NODE (obj) && SWFDEC_XML_NODE (obj)->valid)

struct _SwfdecXmlNode {
  SwfdecAsObject	object;

  gboolean		valid;

  guint			type;		// SwfdecXmlNodeType
  const char		*name;		// for type == element
  const char		*value;		// for type != element

  SwfdecXmlNode		*parent;
  SwfdecAsArray		*children;
  SwfdecAsObject	*attributes;

  // visible trough childNodes property, if modified by the user directly, the
  // changes are not visible in children and will get overwritten by next
  // internal change
  SwfdecAsArray		*child_nodes;
};

struct _SwfdecXmlNodeClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_xml_node_get_type	(void);

SwfdecXmlNode *	swfdec_xml_node_new		(SwfdecAsContext *	context,
						 SwfdecXmlNodeType	type,
						 const char *		value);
SwfdecXmlNode *	swfdec_xml_node_new_no_properties (SwfdecAsContext *	context,
						 SwfdecXmlNodeType	type,
						 const char *		value);
void		swfdec_xml_node_removeNode	(SwfdecXmlNode *	node);
void		swfdec_xml_node_appendChild	(SwfdecXmlNode *	node,
						 SwfdecXmlNode *	child);
void		swfdec_xml_node_removeChildren	(SwfdecXmlNode *	node);
void		swfdec_xml_node_init_values	(SwfdecXmlNode *	node,
						 int			type,
						 const char *		value);
gint32		swfdec_xml_node_num_children	(SwfdecXmlNode *	node);
SwfdecXmlNode *	swfdec_xml_node_get_child	(SwfdecXmlNode *	node,
						 gint32			index_);
const char *	swfdec_xml_node_get_attribute	(SwfdecXmlNode *	node,
						 const char *		name);

G_END_DECLS
#endif
