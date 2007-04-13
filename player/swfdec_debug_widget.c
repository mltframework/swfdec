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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "swfdec_debug_widget.h"


G_DEFINE_TYPE (SwfdecDebugWidget, swfdec_debug_widget, SWFDEC_TYPE_GTK_WIDGET)


static gboolean
swfdec_debug_widget_motion_notify (GtkWidget *gtkwidget, GdkEventMotion *event)
{
  return FALSE;
}

static gboolean
swfdec_debug_widget_leave_notify (GtkWidget *gtkwidget, GdkEventCrossing *event)
{
  return FALSE;
}

#define RADIUS 10
static void
swfdec_debug_widget_invalidate_click_area (SwfdecDebugWidget *widget)
{
  GtkWidget *gtkwidget = GTK_WIDGET (widget);
  GdkRectangle rect = { widget->x - RADIUS, widget->y - RADIUS, 2 * RADIUS, 2 * RADIUS };

  gdk_window_invalidate_rect (gtkwidget->window, &rect, FALSE);
}

static gboolean
swfdec_debug_widget_button_press (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecDebugWidget *debug = SWFDEC_DEBUG_WIDGET (gtkwidget);

  if (event->window != gtkwidget->window)
    return FALSE;

  if (event->button == 1 && swfdec_gtk_widget_get_interactive (widget)) {
    double scale = swfdec_gtk_widget_get_current_scale (widget);
    SwfdecPlayer *player = swfdec_gtk_widget_get_player (widget);
    switch (event->type) {
      case GDK_BUTTON_PRESS:
	swfdec_debug_widget_invalidate_click_area (debug);
	debug->x = event->x;
	debug->y = event->y;
	swfdec_player_handle_mouse (player, 
	    debug->x / scale, debug->y / scale, 
	    debug->button);
	swfdec_debug_widget_invalidate_click_area (debug);
	break;
      case GDK_2BUTTON_PRESS:
	debug->button = 1 - debug->button;
	swfdec_player_handle_mouse (player, 
	    debug->x / scale, debug->y / scale, 
	    debug->button);
	swfdec_debug_widget_invalidate_click_area (debug);
	break;
      default:
	break;
    }
  }
  return FALSE;
}

static gboolean
swfdec_debug_widget_button_release (GtkWidget *gtkwidget, GdkEventButton *event)
{
  return FALSE;
}

static gboolean
swfdec_debug_widget_expose (GtkWidget *gtkwidget, GdkEventExpose *event)
{
  SwfdecDebugWidget *debug = SWFDEC_DEBUG_WIDGET (gtkwidget);
  cairo_t *cr;

  if (event->window != gtkwidget->window)
    return FALSE;

  if (GTK_WIDGET_CLASS (swfdec_debug_widget_parent_class)->expose_event (gtkwidget, event))
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
swfdec_debug_widget_dispose (GObject *object)
{
  //SwfdecWidget *widget = SWFDEC_WIDGET (object);

  G_OBJECT_CLASS (swfdec_debug_widget_parent_class)->dispose (object);
}

static void
swfdec_debug_widget_class_init (SwfdecDebugWidgetClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  GtkWidgetClass *debug_widget_class = GTK_WIDGET_CLASS (g_class);

  object_class->dispose = swfdec_debug_widget_dispose;

  debug_widget_class->expose_event = swfdec_debug_widget_expose;
  debug_widget_class->button_press_event = swfdec_debug_widget_button_press;
  debug_widget_class->button_release_event = swfdec_debug_widget_button_release;
  debug_widget_class->motion_notify_event = swfdec_debug_widget_motion_notify;
  debug_widget_class->leave_notify_event = swfdec_debug_widget_leave_notify;
}

static void
swfdec_debug_widget_init (SwfdecDebugWidget * debug_widget)
{
}

GtkWidget *
swfdec_debug_widget_new (SwfdecPlayer *player)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  return g_object_new (SWFDEC_TYPE_DEBUG_WIDGET, "player", player, NULL);
}

