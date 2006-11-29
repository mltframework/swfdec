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
#include "swfdec_widget.h"


G_DEFINE_TYPE (SwfdecWidget, swfdec_widget, GTK_TYPE_WIDGET)


static gboolean
swfdec_widget_motion_notify (GtkWidget *gtkwidget, GdkEventMotion *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);
  int x, y;

  gdk_window_get_pointer (gtkwidget->window, &x, &y, NULL);

  swfdec_player_handle_mouse (widget->player, 
      x / widget->real_scale, y / widget->real_scale, widget->button);
  
  return FALSE;
}

static gboolean
swfdec_widget_leave_notify (GtkWidget *gtkwidget, GdkEventCrossing *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  widget->button = 0;
  swfdec_player_handle_mouse (widget->player, 
      event->x / widget->real_scale, event->y / widget->real_scale, 0);
  return FALSE;
}

static gboolean
swfdec_widget_button_press (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  if (event->button == 1) {
    widget->button = 1;
    swfdec_player_handle_mouse (widget->player, 
	event->x / widget->real_scale, event->y / widget->real_scale, 1);
  }
  return FALSE;
}

static gboolean
swfdec_widget_button_release (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  if (event->button == 1) {
    widget->button = 0;
    swfdec_player_handle_mouse (widget->player, 
	event->x / widget->real_scale, event->y / widget->real_scale, 0);
  }
  return FALSE;
}

static gboolean
swfdec_widget_expose (GtkWidget *gtkwidget, GdkEventExpose *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);
  SwfdecRect rect;
  cairo_t *cr;
  cairo_surface_t *surface = NULL;

  if (event->window != gtkwidget->window)
    return FALSE;

  if (!widget->use_image) {
    cr = gdk_cairo_create (gtkwidget->window);
  } else {
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 
      event->area.width, event->area.height);
    cr = cairo_create (surface);
    cairo_translate (cr, -event->area.x, -event->area.y);
    cairo_surface_destroy (surface);
  }
  cairo_scale (cr, widget->real_scale, widget->real_scale);
  swfdec_rect_init (&rect, event->area.x / widget->real_scale, event->area.y / widget->real_scale, 
      event->area.width / widget->real_scale, event->area.height / widget->real_scale);
  swfdec_player_render (widget->player, cr, &rect);
  if (widget->use_image) {
    cairo_t *crw = gdk_cairo_create (gtkwidget->window);
    cairo_set_source_surface (crw, surface, event->area.x, event->area.y);
    cairo_paint (crw);
    cairo_destroy (crw);
  }

  cairo_destroy (cr);

  return FALSE;
}

static void
swfdec_widget_dispose (GObject *object)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (object);

  swfdec_widget_set_player (widget, NULL);

  G_OBJECT_CLASS (swfdec_widget_parent_class)->dispose (object);
}

static void
swfdec_widget_size_allocate (GtkWidget *gtkwidget, GtkAllocation *allocation)
{
  double scale;
  int w, h;
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  gtkwidget->allocation = *allocation;

  swfdec_player_get_image_size (widget->player, &w, &h);
  if (widget->set_scale > 0.0) {
    scale = widget->set_scale;
  } else if (widget->player == NULL) {
    scale = 1.0;
  } else {
    if (w != 0 && h != 0)
      scale = MIN ((double) allocation->width / w, (double) allocation->height / h);
    else
      scale = 1.0;
  }
  w = ceil (w * scale);
  h = ceil (h * scale);
  if (w > allocation->width)
    w = allocation->width;
  if (h > allocation->height)
    h = allocation->height;

  if (GTK_WIDGET_REALIZED (gtkwidget)) {
    gdk_window_move_resize (gtkwidget->window, 
	allocation->x + (allocation->width - w) / 2,
	allocation->y + (allocation->height - h) / 2,
	w, h);
  }
  widget->real_scale = scale;
}

static void
swfdec_widget_size_request (GtkWidget *gtkwidget, GtkRequisition *req)
{
  double scale;
  SwfdecWidget * widget = SWFDEC_WIDGET (gtkwidget);

  if (widget->player == NULL) {
    req->width = req->height = 0;
  } else {
    swfdec_player_get_image_size (widget->player, 
	  &req->width, &req->height);
  } 
  if (widget->set_scale != 0.0)
    scale = widget->set_scale;
  else
    scale = 1.0;
  req->width = ceil (req->width * scale);
  req->height = ceil (req->height * scale);
}

static void
swfdec_widget_update_cursor (SwfdecWidget *widget)
{
  GdkWindow *window = GTK_WIDGET (widget)->window;
  gboolean visible;

  if (window == NULL)
    return;
  g_object_get (widget->player, "mouse-visible", &visible, NULL);

  if (visible) {
    gdk_window_set_cursor (window, NULL);
  } else {
    GdkBitmap *bitmap;
    GdkCursor *cursor;
    GdkColor color = { 0, 0, 0, 0 };
    char data = 0;

    bitmap = gdk_bitmap_create_from_data (window, &data, 1, 1);
    if (bitmap == NULL)
      return;
    cursor = gdk_cursor_new_from_pixmap (bitmap, bitmap, &color, &color, 0, 0);
    gdk_window_set_cursor (window, cursor);
    gdk_cursor_unref (cursor);
    g_object_unref (bitmap);
  }
}

static void
swfdec_widget_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  gint attributes_mask;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= GDK_EXPOSURE_MASK | 
			   GDK_BUTTON_PRESS_MASK |
			   GDK_BUTTON_RELEASE_MASK |
			   GDK_LEAVE_NOTIFY_MASK | 
			   GDK_POINTER_MOTION_MASK | 
			   GDK_POINTER_MOTION_HINT_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
      &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);

  if (SWFDEC_WIDGET (widget)->player) {
    swfdec_widget_update_cursor (SWFDEC_WIDGET (widget));
  }
}

static void
swfdec_widget_class_init (SwfdecWidgetClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (g_class);

  object_class->dispose = swfdec_widget_dispose;

  widget_class->realize = swfdec_widget_realize;
  widget_class->size_request = swfdec_widget_size_request;
  widget_class->size_allocate = swfdec_widget_size_allocate;
  widget_class->expose_event = swfdec_widget_expose;
  widget_class->button_press_event = swfdec_widget_button_press;
  widget_class->button_release_event = swfdec_widget_button_release;
  widget_class->motion_notify_event = swfdec_widget_motion_notify;
  widget_class->leave_notify_event = swfdec_widget_leave_notify;
}

static void
swfdec_widget_init (SwfdecWidget * widget)
{
  widget->real_scale = 1.0;
}

static void
swfdec_widget_invalidate_cb (SwfdecPlayer *player, const SwfdecRect *invalid, SwfdecWidget *widget)
{
  GdkRectangle gdkrect;
  SwfdecRect rect;

  if (!GTK_WIDGET_REALIZED (widget))
    return;
  swfdec_rect_scale (&rect, invalid, widget->real_scale);
  gdkrect.x = floor (rect.x0);
  gdkrect.y = floor (rect.y0);
  gdkrect.width = ceil (rect.x1) - gdkrect.x;
  gdkrect.height = ceil (rect.y1) - gdkrect.y;
  //g_print ("queing draw of %g %g  %g %g\n", rect.x0, rect.y0, rect.x1, rect.y1);
  gdk_window_invalidate_rect (GTK_WIDGET (widget)->window, &gdkrect, FALSE);
}

static void
swfdec_widget_notify_cb (SwfdecPlayer *player, GParamSpec *pspec, SwfdecWidget *widget)
{
  if (g_str_equal (pspec->name, "mouse-visible")) {
    swfdec_widget_update_cursor (widget);
  } else if (g_str_equal (pspec->name, "initialized")) {
    gtk_widget_queue_resize (GTK_WIDGET (widget));
  }
}

void
swfdec_widget_set_player (SwfdecWidget *widget, SwfdecPlayer *player)
{
  g_return_if_fail (SWFDEC_IS_WIDGET (widget));
  g_return_if_fail (player == NULL || SWFDEC_IS_PLAYER (player));
  
  if (widget->player) {
    g_signal_handlers_disconnect_by_func (widget->player, swfdec_widget_invalidate_cb, widget);
    g_signal_handlers_disconnect_by_func (widget->player, swfdec_widget_notify_cb, widget);
    g_object_unref (widget->player);
  }
  widget->player = player;
  if (player) {
    g_signal_connect (player, "invalidate", G_CALLBACK (swfdec_widget_invalidate_cb), widget);
    g_signal_connect (player, "notify", G_CALLBACK (swfdec_widget_notify_cb), widget);
    g_object_ref (player);
    swfdec_widget_update_cursor (widget);
  } else {
    if (GTK_WIDGET (widget)->window)
      gdk_window_set_cursor (GTK_WIDGET (widget)->window, NULL); 
  }
  gtk_widget_queue_resize (GTK_WIDGET (widget));
}

GtkWidget *
swfdec_widget_new (SwfdecPlayer *player)
{
  SwfdecWidget *widget;
  
  g_return_val_if_fail (player == NULL || SWFDEC_IS_PLAYER (player), NULL);

  widget = g_object_new (SWFDEC_TYPE_WIDGET, 0);
  swfdec_widget_set_player (widget, player);

  return GTK_WIDGET (widget);
}

void
swfdec_widget_set_scale	(SwfdecWidget *widget, double scale)
{
  g_return_if_fail (SWFDEC_IS_WIDGET (widget));
  g_return_if_fail (scale >= 0.0);

  widget->set_scale = scale;
  gtk_widget_queue_resize (GTK_WIDGET (widget));
}

double
swfdec_widget_get_scale (SwfdecWidget *widget)
{
  g_return_val_if_fail (SWFDEC_IS_WIDGET (widget), 1.0);

  return widget->set_scale;
}

void
swfdec_widget_set_use_image (SwfdecWidget *widget, gboolean use_image)
{
  g_return_if_fail (SWFDEC_IS_WIDGET (widget));

  widget->use_image = use_image;
  gtk_widget_queue_draw (GTK_WIDGET (widget));
}

gboolean
swfdec_widget_get_use_image (SwfdecWidget *widget)
{
  g_return_val_if_fail (SWFDEC_IS_WIDGET (widget), 1.0);

  return widget->use_image;
}
