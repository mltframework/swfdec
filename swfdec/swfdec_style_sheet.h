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

#ifndef _SWFDEC_STYLESHEET_H_
#define _SWFDEC_STYLESHEET_H_

#include <swfdec/swfdec_as_object.h>
#include <swfdec/swfdec_types.h>
#include <swfdec/swfdec_script.h>
#include <swfdec/swfdec_text_format.h>

G_BEGIN_DECLS

typedef struct _SwfdecStyleSheet SwfdecStyleSheet;
typedef struct _SwfdecStyleSheetClass SwfdecStyleSheetClass;

#define SWFDEC_TYPE_STYLESHEET                    (swfdec_style_sheet_get_type())
#define SWFDEC_IS_STYLESHEET(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_STYLESHEET))
#define SWFDEC_IS_STYLESHEET_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_STYLESHEET))
#define SWFDEC_STYLESHEET(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_STYLESHEET, SwfdecStyleSheet))
#define SWFDEC_STYLESHEET_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_STYLESHEET, SwfdecStyleSheetClass))
#define SWFDEC_STYLESHEET_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_STYLESHEET, SwfdecStyleSheetClass))

struct _SwfdecStyleSheet {
  SwfdecAsObject	object;
};

struct _SwfdecStyleSheetClass {
  SwfdecAsObjectClass	object_class;
};

GType		swfdec_style_sheet_get_type	(void);

SwfdecTextFormat *swfdec_style_sheet_get_tag_format	(SwfdecStyleSheet *	style,
							 const char *		name);
SwfdecTextFormat *swfdec_style_sheet_get_class_format	(SwfdecStyleSheet *	style,
							 const char *		name);

G_END_DECLS
#endif
