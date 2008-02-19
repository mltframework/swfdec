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

#ifndef _SWFDEC_XML_H_
#define _SWFDEC_XML_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_script.h>
#include <swfdec/swfdec_xml_node.h>

G_BEGIN_DECLS

typedef struct _SwfdecXml SwfdecXml;
typedef struct _SwfdecXmlClass SwfdecXmlClass;

#define SWFDEC_TYPE_XML                    (swfdec_xml_get_type())
#define SWFDEC_IS_XML(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_XML))
#define SWFDEC_IS_XML_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_XML))
#define SWFDEC_XML(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_XML, SwfdecXml))
#define SWFDEC_XML_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_XML, SwfdecXmlClass))
#define SWFDEC_XML_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_XML, SwfdecXmlClass))

typedef enum {
  XML_PARSE_STATUS_OK = 0,
  XML_PARSE_STATUS_CDATA_NOT_TERMINATED = -2,
  XML_PARSE_STATUS_XMLDECL_NOT_TERMINATED = -3,
  XML_PARSE_STATUS_DOCTYPEDECL_NOT_TERMINATED = -4,
  XML_PARSE_STATUS_COMMENT_NOT_TERMINATED = -5,
  XML_PARSE_STATUS_ELEMENT_MALFORMED = -6,
  XML_PARSE_STATUS_OUT_OF_MEMORY = -7,
  XML_PARSE_STATUS_ATTRIBUTE_NOT_TERMINATED = -8,
  XML_PARSE_STATUS_TAG_NOT_CLOSED = -9, // FIXME are the two correct?
  XML_PARSE_STATUS_TAG_MISMATCH = -10
} SwfdecXmlParseStatus;

struct _SwfdecXml {
  SwfdecXmlNode		xml_node;

  gboolean		ignore_white;
  int			status;
  const char		*xml_decl;
  const char		*doc_type_decl;

  SwfdecAsValue		content_type;
  SwfdecAsValue		loaded;
};

struct _SwfdecXmlClass {
  SwfdecXmlNodeClass	xml_node_class;
};

GType		swfdec_xml_get_type		(void);

char *		swfdec_xml_escape		(const char *		orginal);
char *		swfdec_xml_escape_len		(const char *		orginal,
						 gssize			length);
char *		swfdec_xml_unescape		(SwfdecAsContext *	cx,
						 const char *		orginal);
char *		swfdec_xml_unescape_len		(SwfdecAsContext *	cx,
						 const char *		orginal,
						 gssize			length,
						 gboolean		unescape_nbsp);

SwfdecXml *	swfdec_xml_new			(SwfdecAsContext *	context,
						 const char *		str,
						 gboolean		ignore_white);
SwfdecXml *	swfdec_xml_new_no_properties	(SwfdecAsContext *	context,
						 const char *		str,
						 gboolean		ignore_white);

G_END_DECLS
#endif
