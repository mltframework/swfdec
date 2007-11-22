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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "vivi_widget.h"

enum {
  PROP_0,
  PROP_APP
};

G_DEFINE_TYPE (ViviWidget, vivi_widget, SWFDEC_TYPE_GTK_WIDGET)

static void
vivi_widget_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  ViviWidget *widget = VIVI_WIDGET (object);
  
  switch (param_id) {
    case PROP_APP:
      g_value_set_object (value, widget->app);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
vivi_widget_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  ViviWidget *widget = VIVI_WIDGET (object);
  
  switch (param_id) {
    case PROP_APP:
      vivi_widget_set_application (widget, VIVI_APPLICATION (g_value_get_object (value)));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static gboolean
vivi_widget_motion_notify (GtkWidget *gtkwidget, GdkEventMotion *event)
{
  return FALSE;
}

static gboolean
vivi_widget_leave_notify (GtkWidget *gtkwidget, GdkEventCrossing *event)
{
  return FALSE;
}

#define RADIUS 10
static void
vivi_widget_invalidate_click_area (ViviWidget *widget)
{
  GtkWidget *gtkwidget = GTK_WIDGET (widget);
  GdkRectangle rect = { widget->x - RADIUS, widget->y - RADIUS, 2 * RADIUS, 2 * RADIUS };

  gdk_window_invalidate_rect (gtkwidget->window, &rect, FALSE);
}

static gboolean
vivi_widget_button_press (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  ViviWidget *debug = VIVI_WIDGET (gtkwidget);

  if (event->window != gtkwidget->window)
    return FALSE;

  if (event->button == 1 && swfdec_gtk_widget_get_interactive (widget)) {
    SwfdecPlayer *player = swfdec_gtk_widget_get_player (widget);
    // cast to int to get rid of unhandled enum warnings...
    switch ((int)event->type) {
      case GDK_BUTTON_PRESS:
	vivi_widget_invalidate_click_area (debug);
	debug->x = event->x;
	debug->y = event->y;
	swfdec_player_mouse_move (player, debug->x, debug->y);
	vivi_widget_invalidate_click_area (debug);
	break;
      case GDK_2BUTTON_PRESS:
	debug->button = 1 - debug->button;
	debug->x = event->x;
	debug->y = event->y;
	if (debug->button)
	  swfdec_player_mouse_press (player, debug->x, debug->y, 1);
	else
	  swfdec_player_mouse_release (player, debug->x, debug->y, 1);
	vivi_widget_invalidate_click_area (debug);
	break;
      default:
	break;
    }
  }
  return FALSE;
}

static gboolean
vivi_widget_button_release (GtkWidget *gtkwidget, GdkEventButton *event)
{
  return FALSE;
}

static gboolean
vivi_widget_expose (GtkWidget *gtkwidget, GdkEventExpose *event)
{
  ViviWidget *debug = VIVI_WIDGET (gtkwidget);
  cairo_t *cr;

  if (event->window != gtkwidget->window)
    return FALSE;

  if (GTK_WIDGET_CLASS (vivi_widget_parent_class)->expose_event (gtkwidget, event))
    return TRUE;

  cr = gdk_cairo_create (gtkwidget->window);

  cairo_arc (cr, debug->x, debug->y, RADIUS - 1.5, 0.0, 2 * G_PI);
  if (debug->button) {
    cairo_set_source_rgba (cr, 0.25, 0.25, 0.25, 0.5);
    cairo_fill_preserve (cr);
  }
  cairo_set_line_width (cr, 3);
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_stroke_preserve (cr);
  cairo_set_line_width (cr, 1);
  cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);
  cairo_stroke (cr);
  cairo_destroy (cr);

  return FALSE;
}

static void
vivi_widget_dispose (GObject *object)
{
  ViviWidget *widget = VIVI_WIDGET (object);

  vivi_widget_set_application (widget, NULL);

  G_OBJECT_CLASS (vivi_widget_parent_class)->dispose (object);
}

static void
vivi_widget_class_init (ViviWidgetClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (g_class);

  object_class->dispose = vivi_widget_dispose;
  object_class->get_property = vivi_widget_get_property;
  object_class->set_property = vivi_widget_set_property;

  g_object_class_install_property (object_class, PROP_APP,
      g_param_spec_object ("application", "application", "application that is playing",
	  VIVI_TYPE_APPLICATION, G_PARAM_READWRITE));

  widget_class->expose_event = vivi_widget_expose;
  widget_class->button_press_event = vivi_widget_button_press;
  widget_class->button_release_event = vivi_widget_button_release;
  widget_class->motion_notify_event = vivi_widget_motion_notify;
  widget_class->leave_notify_event = vivi_widget_leave_notify;
}

static void
vivi_widget_init (ViviWidget *widget)
{
}

GtkWidget *
vivi_widget_new (ViviApplication *app)
{
  ViviWidget *widget;

  g_return_val_if_fail (VIVI_IS_APPLICATION (app), NULL);

  widget = g_object_new (VIVI_TYPE_WIDGET, "player", vivi_application_get_player (app), NULL);

  return GTK_WIDGET (widget);
}

static void
vivi_widget_app_notify (ViviApplication *app, GParamSpec *pspec, ViviWidget *widget)
{
  if (g_str_equal (pspec->name, "interrupted")) {
    swfdec_gtk_widget_set_interactive (SWFDEC_GTK_WIDGET (widget),
	!vivi_application_get_interrupted (widget->app));
  } else if (g_str_equal (pspec->name, "player")) {
    swfdec_gtk_widget_set_player (SWFDEC_GTK_WIDGET (widget),
	vivi_application_get_player (widget->app));
  }
}

void
vivi_widget_set_application (ViviWidget *widget, ViviApplication *app)
{
  g_return_if_fail (VIVI_IS_WIDGET (widget));
  g_return_if_fail (app == NULL || VIVI_IS_APPLICATION (app));

  if (widget->app) {
    g_signal_handlers_disconnect_by_func (widget->app, vivi_widget_app_notify, widget);
    g_object_unref (widget->app);
  }
  widget->app = app;
  if (app) {
    g_object_ref (app);
    g_signal_connect (app, "notify", G_CALLBACK (vivi_widget_app_notify), widget);
    swfdec_gtk_widget_set_interactive (SWFDEC_GTK_WIDGET (widget),
	!vivi_application_get_interrupted (app));
    swfdec_gtk_widget_set_player (SWFDEC_GTK_WIDGET (widget),
	vivi_application_get_player (app));
  } else {
    swfdec_gtk_widget_set_interactive (SWFDEC_GTK_WIDGET (widget), TRUE);
    swfdec_gtk_widget_set_player (SWFDEC_GTK_WIDGET (widget), NULL);
  }
  g_object_notify (G_OBJECT (widget), "application");
}

