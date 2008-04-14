/* Swfdec
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

#ifndef _SWFDEC_GTK_WIDGET_H_
#define _SWFDEC_GTK_WIDGET_H_

#include <gtk/gtk.h>
#include <swfdec/swfdec.h>

G_BEGIN_DECLS

typedef struct _SwfdecGtkWidget SwfdecGtkWidget;
typedef struct _SwfdecGtkWidgetClass SwfdecGtkWidgetClass;
typedef struct _SwfdecGtkWidgetPrivate SwfdecGtkWidgetPrivate;

#define SWFDEC_TYPE_GTK_WIDGET                    (swfdec_gtk_widget_get_type())
#define SWFDEC_IS_GTK_WIDGET(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_GTK_WIDGET))
#define SWFDEC_IS_GTK_WIDGET_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_GTK_WIDGET))
#define SWFDEC_GTK_WIDGET(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_GTK_WIDGET, SwfdecGtkWidget))
#define SWFDEC_GTK_WIDGET_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_GTK_WIDGET, SwfdecGtkWidgetClass))

struct _SwfdecGtkWidget
{
  GtkWidget			widget;

  /*< private >*/
  SwfdecGtkWidgetPrivate *	priv;
};

struct _SwfdecGtkWidgetClass
{
  GtkWidgetClass		widget_class;
};

GType		swfdec_gtk_widget_get_type		(void);

GtkWidget *	swfdec_gtk_widget_new			(SwfdecPlayer *		player);
GtkWidget *	swfdec_gtk_widget_new_fullscreen	(SwfdecPlayer *		player);

void		swfdec_gtk_widget_set_player		(SwfdecGtkWidget *	widget,
							 SwfdecPlayer *		player);
SwfdecPlayer *	swfdec_gtk_widget_get_player		(SwfdecGtkWidget *	widget);
void		swfdec_gtk_widget_set_renderer		(SwfdecGtkWidget *	widget,
							 cairo_surface_type_t	renderer);
void		swfdec_gtk_widget_unset_renderer	(SwfdecGtkWidget *      widget);
cairo_surface_type_t
		swfdec_gtk_widget_get_renderer		(SwfdecGtkWidget *	widget);
gboolean	swfdec_gtk_widget_uses_renderer		(SwfdecGtkWidget *	widget);
void		swfdec_gtk_widget_set_interactive	(SwfdecGtkWidget *	widget,
							 gboolean		interactive);
gboolean	swfdec_gtk_widget_get_interactive	(SwfdecGtkWidget *	widget);

G_END_DECLS
#endif
