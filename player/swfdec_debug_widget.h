/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_DEBUG_WIDGET_H_
#define _SWFDEC_DEBUG_WIDGET_H_

#include <libswfdec-gtk/swfdec_gtk_widget.h>

G_BEGIN_DECLS

typedef struct _SwfdecDebugWidget SwfdecDebugWidget;
typedef struct _SwfdecDebugWidgetClass SwfdecDebugWidgetClass;

#define SWFDEC_TYPE_DEBUG_WIDGET                    (swfdec_debug_widget_get_type())
#define SWFDEC_IS_DEBUG_WIDGET(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DEBUG_WIDGET))
#define SWFDEC_IS_DEBUG_WIDGET_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DEBUG_WIDGET))
#define SWFDEC_DEBUG_WIDGET(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DEBUG_WIDGET, SwfdecDebugWidget))
#define SWFDEC_DEBUG_WIDGET_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DEBUG_WIDGET, SwfdecDebugWidgetClass))

struct _SwfdecDebugWidget
{
  SwfdecGtkWidget     	widget;

  int			x;
  int			y;
  int			button;
};

struct _SwfdecDebugWidgetClass
{
  SwfdecGtkWidgetClass	debug_widget_class;
};

GType		swfdec_debug_widget_get_type		(void);

GtkWidget *	swfdec_debug_widget_new			(SwfdecPlayer *	player);

G_END_DECLS
#endif
