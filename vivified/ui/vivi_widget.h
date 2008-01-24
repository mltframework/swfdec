/* Vivi
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _VIVI_WIDGET_H_
#define _VIVI_WIDGET_H_

#include <swfdec-gtk/swfdec-gtk.h>
#include <vivified/core/vivified-core.h>

G_BEGIN_DECLS

typedef struct _ViviWidget ViviWidget;
typedef struct _ViviWidgetClass ViviWidgetClass;

#define VIVI_TYPE_WIDGET                    (vivi_widget_get_type())
#define VIVI_IS_WIDGET(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_WIDGET))
#define VIVI_IS_WIDGET_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_WIDGET))
#define VIVI_WIDGET(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_WIDGET, ViviWidget))
#define VIVI_WIDGET_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_WIDGET, ViviWidgetClass))

struct _ViviWidget
{
  SwfdecGtkWidget     	widget;

  ViviApplication *	app;

  int			x;
  int			y;
  int			button;
};

struct _ViviWidgetClass
{
  SwfdecGtkWidgetClass	widget_class;
};

GType		vivi_widget_get_type		(void);

GtkWidget *	vivi_widget_new			(ViviApplication *	app);

void		vivi_widget_set_application	(ViviWidget *		widget,
						 ViviApplication *	app);


G_END_DECLS
#endif
