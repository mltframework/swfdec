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

#ifndef _SWFDEC_XML_H_
#define _SWFDEC_XML_H_

#include <libswfdec/swfdec_scriptable.h>

G_BEGIN_DECLS


typedef struct _SwfdecXml SwfdecXml;
typedef struct _SwfdecXmlClass SwfdecXmlClass;

#define SWFDEC_TYPE_XML                    (swfdec_xml_get_type())
#define SWFDEC_IS_XML(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_XML))
#define SWFDEC_IS_XML_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_XML))
#define SWFDEC_XML(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_XML, SwfdecXml))
#define SWFDEC_XML_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_XML, SwfdecXmlClass))
#define SWFDEC_XML_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_XML, SwfdecXmlClass))

struct _SwfdecXml {
  SwfdecScriptable	scriptable;

  char *		text;		/* string that this XML displays */
  SwfdecPlayer *	player;		/* player we're playing in */
  SwfdecLoader *	loader;		/* loader when loading or NULL */
};

struct _SwfdecXmlClass {
  SwfdecScriptableClass	scriptable_class;
};

GType		swfdec_xml_get_type	(void);

SwfdecXml *	swfdec_xml_new		(SwfdecPlayer *	player);

void		swfdec_xml_load		(SwfdecXml *	xml,
					 const char *	url);


G_END_DECLS
#endif
