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

#ifndef _SWFDEC_WIDGET_H_
#define _SWFDEC_WIDGET_H_

#include <gtk/gtk.h>
#include <libswfdec/swfdec.h>

G_BEGIN_DECLS

typedef struct _SwfdecWidget SwfdecWidget;
typedef struct _SwfdecWidgetClass SwfdecWidgetClass;

#define SWFDEC_TYPE_WIDGET                    (swfdec_widget_get_type())
#define SWFDEC_IS_WIDGET(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_WIDGET))
#define SWFDEC_IS_WIDGET_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_WIDGET))
#define SWFDEC_WIDGET(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_WIDGET, SwfdecWidget))
#define SWFDEC_WIDGET_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_WIDGET, SwfdecWidgetClass))

struct _SwfdecWidget
{
  GtkWidget		widget;

  SwfdecPlayer *	player;		/* the video we play */

  double		real_scale;	/* the real scale factor used */
  double		set_scale;    	/* the set scale factor of the video */
  gboolean		use_image;	/* TRUE to draw to an image first before rendering to Gtk */
  gboolean		interactive;	/* if this widget propagates keyboard and mouse events */

  int			button;		/* status of mouse button in displayed movie */
};

struct _SwfdecWidgetClass
{
  GtkWidgetClass	widget_class;
};

GType		swfdec_widget_get_type		(void);

GtkWidget *	swfdec_widget_new		(SwfdecPlayer *	player);

void		swfdec_widget_set_scale		(SwfdecWidget *	widget,
						 double		scale);
double		swfdec_widget_get_scale		(SwfdecWidget *	widget);
void		swfdec_widget_set_use_image	(SwfdecWidget *	widget,
						 gboolean	use_image);
gboolean	swfdec_widget_get_use_image	(SwfdecWidget *	widget);
void		swfdec_widget_set_interactive	(SwfdecWidget * widget,
						 gboolean	interactive);
gboolean	swfdec_widget_get_interactive	(SwfdecWidget * widget);
void		swfdec_widget_set_player	(SwfdecWidget *	widget,
						 SwfdecPlayer *	player);

G_END_DECLS
#endif
