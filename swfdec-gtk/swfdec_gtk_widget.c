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
#include <gdk/gdkkeysyms.h>
#include "swfdec_gtk_widget.h"
#include "swfdec_gtk_keys.h"
#include "swfdec_gtk_player.h"

struct _SwfdecGtkWidgetPrivate
{
  SwfdecPlayer *	player;		/* the video we play */

  gboolean		renderer_set;	/* TRUE if a special renderer has been set */
  cairo_surface_type_t	renderer_type;	/* the renderer that was set */
  SwfdecRenderer *	renderer;	/* renderer in use */
  gboolean		interactive;	/* TRUE if this widget propagates keyboard and mouse events */
  GdkRegion *		invalid;	/* invalid regions we didn't yet repaint */
  guint			invalidator;	/* GSource used for invalidating window contents */
};

enum {
  PROP_0,
  PROP_PLAYER,
  PROP_INTERACTIVE,
  PROP_RENDERER_SET,
  PROP_RENDERER
};

/*** gtk-doc ***/

/**
 * SECTION:SwfdecGtkWidget
 * @title: SwfdecGtkWidget
 * @short_description: a #GtkWidget for embedding SWF files
 *
 * This is a widget for playing Flash movies rendered with Swfdec in a Gtk 
 * application. It supports a lot of advanced features, if you want to use
 * them. If you don't want to use them and just want to embed a movie in 
 * your application, swfdec_gtk_widget_new() will probably be the only 
 * function you need.
 *
 * @see_also: SwfdecGtkPlayer
 */

/**
 * SwfdecGtkWidget:
 *
 * The structure for the Swfdec Gtk widget contains no public fields.
 */

/*** SWFDEC_GTK_WIDGET ***/

G_DEFINE_TYPE (SwfdecGtkWidget, swfdec_gtk_widget, GTK_TYPE_WIDGET)

static gboolean
swfdec_gtk_widget_motion_notify (GtkWidget *gtkwidget, GdkEventMotion *event)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;
  int x, y;

  gdk_window_get_pointer (gtkwidget->window, &x, &y, NULL);

  if (priv->interactive && priv->player)
    swfdec_player_mouse_move (priv->player, x, y);
  
  return FALSE;
}

static gboolean
swfdec_gtk_widget_leave_notify (GtkWidget *gtkwidget, GdkEventCrossing *event)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  if (priv->interactive && priv->player) {
    swfdec_player_mouse_move (priv->player, event->x, event->y);
  }
  return FALSE;
}

static gboolean
swfdec_gtk_widget_button_press (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  if (event->type == GDK_BUTTON_PRESS && event->button <= 32 && priv->interactive && priv->player) {
    swfdec_player_mouse_press (priv->player, event->x, event->y, event->button);
  }
  return FALSE;
}

static gboolean
swfdec_gtk_widget_button_release (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  if (event->button <= 32 && priv->interactive && priv->player) {
    swfdec_player_mouse_release (priv->player, event->x, event->y, event->button);
  }
  return FALSE;
}

static guint
swfdec_gtk_event_to_keycode (GdkEventKey *event)
{
  guint ret;

  /* we try to match as well as possible to Flash _Windows_ key codes.
   * Since a lot of Flash files won't special case weird Linux key codes and we
   * want the best compatibility possible, we have to do that.
   */
  /* FIXME: I have no clue about non-western keyboards, so if you happen to use
   * such a keyboard, please help out here if it doesn't match.
   */

  /* try to match latin keys directly */
  if (event->keyval >= GDK_A && event->keyval <= GDK_Z)
    return event->keyval - GDK_A + SWFDEC_KEY_A;
  if (event->keyval >= GDK_a && event->keyval <= GDK_z)
    return event->keyval - GDK_a + SWFDEC_KEY_A;

  /* last resort: try to translate the hardware keycode directly */
  ret = swfdec_gtk_keycode_from_hardware_keycode (event->hardware_keycode);
  if (ret == 0)
    g_printerr ("could not translate key to Flash keycode. HW keycode %u, keyval %u\n",
	event->hardware_keycode, event->keyval);
  return ret;
}

static gboolean
swfdec_gtk_widget_key_press (GtkWidget *gtkwidget, GdkEventKey *event)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  if (priv->interactive && priv->player) {
    guint keycode = swfdec_gtk_event_to_keycode (event);
    if (keycode != 0) {
      swfdec_player_key_press (priv->player, keycode, 
	  gdk_keyval_to_unicode (event->keyval));
    }
    return TRUE;
  }

  return FALSE;
}

static gboolean
swfdec_gtk_widget_key_release (GtkWidget *gtkwidget, GdkEventKey *event)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  if (priv->interactive && priv->player) {
    guint keycode = swfdec_gtk_event_to_keycode (event);
    if (keycode != 0) {
      swfdec_player_key_release (priv->player, keycode, 
	  gdk_keyval_to_unicode (event->keyval));
    }
    return TRUE;
  }

  return FALSE;
}

/* NB: called for both focus in and focus out */
static gboolean
swfdec_gtk_widget_focus_inout (GtkWidget *gtkwidget, GdkEventFocus *focus)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  if (priv->interactive && priv->player)
    swfdec_player_set_focus (priv->player, focus->in);
  return FALSE;
}

static gboolean
swfdec_gtk_widget_focus (GtkWidget *gtkwidget, GtkDirectionType direction)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  if (!priv->interactive || priv->player == NULL)
    return FALSE;

  return GTK_WIDGET_CLASS (swfdec_gtk_widget_parent_class)->focus (gtkwidget, direction);
}

static void
swfdec_gtk_widget_clear_invalidations (SwfdecGtkWidget *widget)
{
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  if (priv->invalidator) {
    g_source_remove (priv->invalidator);
    priv->invalidator = 0;
    gdk_region_destroy (priv->invalid);
    priv->invalid = gdk_region_new ();
  } else {
    g_assert (gdk_region_empty (priv->invalid));
  }
}

static cairo_surface_t *
swfdec_gtk_widget_create_renderer (cairo_surface_type_t type, int width, int height)
{
  if (type == CAIRO_SURFACE_TYPE_IMAGE) {
    return cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  } else {
    return NULL;
  }
}

static gboolean
swfdec_gtk_widget_expose (GtkWidget *gtkwidget, GdkEventExpose *event)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;
  cairo_t *cr;
  cairo_surface_t *surface = NULL;

  if (event->window != gtkwidget->window)
    return FALSE;
  if (priv->player == NULL)
    return FALSE;

  /* FIXME: This might be ugly */
  gdk_region_union (event->region, priv->invalid);
  gdk_window_begin_paint_region (event->window, event->region);

  if (!priv->renderer_set ||
      (surface = swfdec_gtk_widget_create_renderer (priv->renderer_type, 
	      event->area.width, event->area.height)) == NULL) {
    cr = gdk_cairo_create (gtkwidget->window);
  } else {
    cr = cairo_create (surface);
    cairo_translate (cr, -event->area.x, -event->area.y);
  }
  swfdec_player_render (priv->player, cr,
      event->area.x, event->area.y, event->area.width, event->area.height);
  cairo_show_page (cr);
  cairo_destroy (cr);

  if (surface) {
    cairo_t *crw = gdk_cairo_create (gtkwidget->window);
    cairo_set_source_surface (crw, surface, event->area.x, event->area.y);
    cairo_paint (crw);
    cairo_destroy (crw);
    cairo_surface_destroy (surface);
  }

  swfdec_gtk_widget_clear_invalidations (widget);
  gdk_window_end_paint (event->window);
  return FALSE;
}

static void
swfdec_gtk_widget_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (object);
  SwfdecGtkWidgetPrivate *priv = widget->priv;
  
  switch (param_id) {
    case PROP_PLAYER:
      g_value_set_object (value, priv->player);
      break;
    case PROP_INTERACTIVE:
      g_value_set_boolean (value, priv->interactive);
      break;
    case PROP_RENDERER_SET:
      g_value_set_boolean (value, priv->renderer_set);
      break;
    case PROP_RENDERER:
      g_value_set_uint (value, priv->renderer_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_gtk_widget_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (object);
  SwfdecGtkWidgetPrivate *priv = widget->priv;
  
  switch (param_id) {
    case PROP_PLAYER:
      swfdec_gtk_widget_set_player (widget, g_value_get_object (value));
      break;
    case PROP_INTERACTIVE:
      swfdec_gtk_widget_set_interactive (widget, g_value_get_boolean (value));
      break;
    case PROP_RENDERER_SET:
      priv->renderer_set = g_value_get_boolean (value);
      gtk_widget_queue_draw (GTK_WIDGET (widget));
      break;
    case PROP_RENDERER:
      priv->renderer_type = g_value_get_uint (value);
      if (priv->renderer_set)
	gtk_widget_queue_draw (GTK_WIDGET (widget));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_gtk_widget_dispose (GObject *object)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (object);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  swfdec_gtk_widget_set_player (widget, NULL);

  g_assert (priv->renderer == NULL);
  if (priv->invalid) {
    gdk_region_destroy (priv->invalid);
    priv->invalid = NULL;
  }
  g_assert (priv->invalidator == 0);

  G_OBJECT_CLASS (swfdec_gtk_widget_parent_class)->dispose (object);
}

static void
swfdec_gtk_widget_size_allocate (GtkWidget *gtkwidget, GtkAllocation *allocation)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  gtkwidget->allocation = *allocation;

  if (priv->player && swfdec_player_is_initialized (priv->player))
    swfdec_player_set_size (priv->player, allocation->width, allocation->height);
  if (GTK_WIDGET_REALIZED (gtkwidget)) {
    gdk_window_move_resize (gtkwidget->window, 
	allocation->x, allocation->y, allocation->width, allocation->height);
  }
}

static void
swfdec_gtk_widget_size_request (GtkWidget *gtkwidget, GtkRequisition *req)
{
  SwfdecGtkWidget * widget = SWFDEC_GTK_WIDGET (gtkwidget);
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  if (priv->player == NULL) {
    req->width = req->height = 0;
  } else {
    guint w, h;
    swfdec_player_get_default_size (priv->player, &w, &h);
    /* FIXME: set some sane upper limit here? */
    req->width = MIN (w, G_MAXINT);
    req->height = MIN (h, G_MAXINT);
  } 
}

static void
swfdec_gtk_widget_update_cursor (SwfdecGtkWidget *widget)
{
  GdkWindow *window = GTK_WIDGET (widget)->window;
  GdkDisplay *display = gtk_widget_get_display (GTK_WIDGET (widget));
  SwfdecGtkWidgetPrivate *priv = widget->priv;
  SwfdecMouseCursor swfcursor;
  GdkCursor *cursor;

  if (window == NULL)
    return;
  if (priv->interactive)
    g_object_get (priv->player, "mouse-cursor", &swfcursor, NULL);
  else
    swfcursor = SWFDEC_MOUSE_CURSOR_NORMAL;

  switch (swfcursor) {
    case SWFDEC_MOUSE_CURSOR_NONE:
      {
	GdkBitmap *bitmap;
	GdkColor color = { 0, 0, 0, 0 };
	char data = 0;

	bitmap = gdk_bitmap_create_from_data (window, &data, 1, 1);
	if (bitmap == NULL)
	  return;
	cursor = gdk_cursor_new_from_pixmap (bitmap, bitmap, &color, &color, 0, 0);
	gdk_window_set_cursor (window, cursor);
	gdk_cursor_unref (cursor);
	g_object_unref (bitmap);
	break;
      }
    case SWFDEC_MOUSE_CURSOR_TEXT:
      cursor = gdk_cursor_new_for_display (display, GDK_XTERM);
      gdk_window_set_cursor (window, cursor);
      gdk_cursor_unref (cursor);
      break;
    case SWFDEC_MOUSE_CURSOR_CLICK:
      cursor = gdk_cursor_new_for_display (display, GDK_HAND2);
      gdk_window_set_cursor (window, cursor);
      gdk_cursor_unref (cursor);
      break;
    case SWFDEC_MOUSE_CURSOR_NORMAL:
      cursor = gdk_cursor_new_for_display (display, GDK_LEFT_PTR);
      gdk_window_set_cursor (window, cursor);
      gdk_cursor_unref (cursor);
      break;
    default:
      g_warning ("invalid cursor %d", (int) swfcursor);
      gdk_window_set_cursor (window, NULL);
      break;
  }
}

static void
swfdec_gtk_widget_update_renderer (SwfdecGtkWidget *widget)
{
  SwfdecGtkWidgetPrivate *priv = widget->priv;
  gboolean needs_renderer;

  needs_renderer = GTK_WIDGET_REALIZED (widget) &&
    priv->player != NULL;

  if (priv->renderer != NULL) {
    swfdec_player_set_renderer (priv->player, NULL);
    g_object_unref (priv->renderer);
    priv->renderer = NULL;
  }
  if (needs_renderer) {
    cairo_t *cr = gdk_cairo_create (GTK_WIDGET (widget)->window);

    if (priv->renderer_set)
      g_printerr ("FIXME: create the right renderer\n");
    priv->renderer = swfdec_renderer_new_for_player (
	cairo_get_target (cr), priv->player);
    swfdec_player_set_renderer (priv->player, priv->renderer);
    cairo_destroy (cr);
  }
}

static void
swfdec_gtk_widget_realize (GtkWidget *widget)
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
			   GDK_POINTER_MOTION_HINT_MASK |
			   GDK_KEY_PRESS_MASK |
			   GDK_KEY_RELEASE_MASK |
			   GDK_FOCUS_CHANGE_MASK;

  attributes_mask = GDK_WA_X | GDK_WA_Y;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
      &attributes, attributes_mask);
  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);

  if (SWFDEC_GTK_WIDGET (widget)->priv->player) {
    swfdec_gtk_widget_update_cursor (SWFDEC_GTK_WIDGET (widget));
  }
  swfdec_gtk_widget_update_renderer (SWFDEC_GTK_WIDGET (widget));
}

static void
swfdec_gtk_widget_unrealize (GtkWidget *widget)
{
  GTK_WIDGET_CLASS (swfdec_gtk_widget_parent_class)->unrealize (widget);

  swfdec_gtk_widget_update_renderer (SWFDEC_GTK_WIDGET (widget));
}

static void
swfdec_gtk_widget_map (GtkWidget *gtkwidget)
{
  SwfdecGtkWidgetPrivate *priv = SWFDEC_GTK_WIDGET (gtkwidget)->priv;

  g_assert (gdk_region_empty (priv->invalid));

  GTK_WIDGET_CLASS (swfdec_gtk_widget_parent_class)->map (gtkwidget);
}

static void
swfdec_gtk_widget_unmap (GtkWidget *gtkwidget)
{
  SwfdecGtkWidget *widget = SWFDEC_GTK_WIDGET (gtkwidget);

  GTK_WIDGET_CLASS (swfdec_gtk_widget_parent_class)->unmap (gtkwidget);

  swfdec_gtk_widget_clear_invalidations (widget);
}

static void
swfdec_gtk_widget_class_init (SwfdecGtkWidgetClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (g_class);

  object_class->dispose = swfdec_gtk_widget_dispose;
  object_class->get_property = swfdec_gtk_widget_get_property;
  object_class->set_property = swfdec_gtk_widget_set_property;

  g_object_class_install_property (object_class, PROP_PLAYER,
      g_param_spec_object ("player", "player", "player that is displayed",
	  SWFDEC_TYPE_PLAYER, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));
  g_object_class_install_property (object_class, PROP_INTERACTIVE,
      g_param_spec_boolean ("interactive", "interactive", "if mouse events are processed",
	  TRUE, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_RENDERER_SET,
      g_param_spec_boolean ("renderer-set", "renderer set", "if an intermediate renderer should be used",
	  TRUE, G_PARAM_READWRITE));
  /* FIXME: get an enum for cairo_surface_type_t */
  g_object_class_install_property (object_class, PROP_RENDERER,
      g_param_spec_uint ("renderer", "renderer", "cairo_surface_type_t of intermediate renderer to use",
	  0, G_MAXUINT, CAIRO_SURFACE_TYPE_IMAGE, G_PARAM_READWRITE));

  widget_class->realize = swfdec_gtk_widget_realize;
  widget_class->unrealize = swfdec_gtk_widget_unrealize;
  widget_class->map = swfdec_gtk_widget_map;
  widget_class->unmap = swfdec_gtk_widget_unmap;
  widget_class->size_request = swfdec_gtk_widget_size_request;
  widget_class->size_allocate = swfdec_gtk_widget_size_allocate;
  widget_class->expose_event = swfdec_gtk_widget_expose;
  widget_class->button_press_event = swfdec_gtk_widget_button_press;
  widget_class->button_release_event = swfdec_gtk_widget_button_release;
  widget_class->motion_notify_event = swfdec_gtk_widget_motion_notify;
  widget_class->leave_notify_event = swfdec_gtk_widget_leave_notify;
  widget_class->key_press_event = swfdec_gtk_widget_key_press;
  widget_class->key_release_event = swfdec_gtk_widget_key_release;
  widget_class->focus_in_event = swfdec_gtk_widget_focus_inout;
  widget_class->focus_out_event = swfdec_gtk_widget_focus_inout;
  widget_class->focus = swfdec_gtk_widget_focus;

  g_type_class_add_private (object_class, sizeof (SwfdecGtkWidgetPrivate));
}

static void
swfdec_gtk_widget_init (SwfdecGtkWidget *widget)
{
  SwfdecGtkWidgetPrivate *priv;
  
  priv = widget->priv = G_TYPE_INSTANCE_GET_PRIVATE (widget, SWFDEC_TYPE_GTK_WIDGET, SwfdecGtkWidgetPrivate);

  priv->interactive = TRUE;
  priv->renderer_type = CAIRO_SURFACE_TYPE_IMAGE;
  priv->invalid = gdk_region_new ();

  gtk_widget_set_double_buffered (GTK_WIDGET (widget), FALSE);

  GTK_WIDGET_SET_FLAGS (widget, GTK_CAN_FOCUS);
}

static gboolean
swfdec_gtk_widget_do_invalidate (gpointer widgetp)
{
  SwfdecGtkWidget *widget = widgetp;
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  g_assert (GTK_WIDGET_REALIZED (widget));

  gdk_window_invalidate_region (GTK_WIDGET (widget)->window, priv->invalid, FALSE);
  swfdec_gtk_widget_clear_invalidations (widget);
  return FALSE;
}

static void
swfdec_gtk_widget_invalidate_cb (SwfdecPlayer *player, const SwfdecRectangle *extents,
    const SwfdecRectangle *rect, guint n_rects, SwfdecGtkWidget *widget)
{
  SwfdecGtkWidgetPrivate *priv = widget->priv;
  guint i;

  if (!GTK_WIDGET_MAPPED (widget))
    return;

  for (i = 0; i < n_rects; i++) {
    gdk_region_union_with_rect (priv->invalid, (GdkRectangle *) &rect[i]);
  }
  if (priv->invalidator == 0) {
    priv->invalidator = g_idle_add_full (SWFDEC_GTK_PRIORITY_REDRAW,
	swfdec_gtk_widget_do_invalidate, widget, NULL);
  }
}

static void
swfdec_gtk_widget_notify_cb (SwfdecPlayer *player, GParamSpec *pspec, SwfdecGtkWidget *widget)
{
  if (g_str_equal (pspec->name, "mouse-cursor")) {
    swfdec_gtk_widget_update_cursor (widget);
  } else if (g_str_equal (pspec->name, "initialized")) {
    gtk_widget_queue_resize (GTK_WIDGET (widget));
  }
}

/*** PUBLIC API ***/

/**
 * swfdec_gtk_widget_set_player:
 * @widget: a #SwfdecGtkWidget
 * @player: the #SwfdecPlayer to display or %NULL for none
 *
 * Sets the new player to display in @widget.
 **/
void
swfdec_gtk_widget_set_player (SwfdecGtkWidget *widget, SwfdecPlayer *player)
{
  SwfdecGtkWidgetPrivate *priv = widget->priv;

  g_return_if_fail (SWFDEC_IS_GTK_WIDGET (widget));
  g_return_if_fail (player == NULL || SWFDEC_IS_PLAYER (player));
  
  if (player) {
    g_signal_connect (player, "invalidate", G_CALLBACK (swfdec_gtk_widget_invalidate_cb), widget);
    g_signal_connect (player, "notify", G_CALLBACK (swfdec_gtk_widget_notify_cb), widget);
    g_object_ref (player);
    swfdec_gtk_widget_update_cursor (widget);
    swfdec_player_set_focus (player, GTK_WIDGET_HAS_FOCUS (GTK_WIDGET (widget)));
  } else {
    if (GTK_WIDGET (widget)->window)
      gdk_window_set_cursor (GTK_WIDGET (widget)->window, NULL); 
  }
  if (priv->player) {
    g_signal_handlers_disconnect_by_func (priv->player, swfdec_gtk_widget_invalidate_cb, widget);
    g_signal_handlers_disconnect_by_func (priv->player, swfdec_gtk_widget_notify_cb, widget);
    g_object_unref (priv->player);
  }
  priv->player = player;
  gtk_widget_queue_resize (GTK_WIDGET (widget));
  g_object_notify (G_OBJECT (widget), "player");
  swfdec_gtk_widget_update_renderer (widget);
}

/**
 * swfdec_gtk_widget_get_player:
 * @widget: a #SwfdecGtkWidget
 *
 * Gets the player that is currently played back in this @widget.
 *
 * Returns: the #SwfdecPlayer or %NULL if none
 **/
SwfdecPlayer *
swfdec_gtk_widget_get_player (SwfdecGtkWidget *widget)
{
  g_return_val_if_fail (SWFDEC_IS_GTK_WIDGET (widget), NULL);

  return widget->priv->player;
}

/**
 * swfdec_gtk_widget_new:
 * @player: a #SwfdecPlayer or %NULL
 *
 * Creates a new #SwfdecGtkWidget to display @player.
 *
 * Returns: the new widget that displays @player
 **/
GtkWidget *
swfdec_gtk_widget_new (SwfdecPlayer *player)
{
  SwfdecGtkWidget *widget;
  
  g_return_val_if_fail (player == NULL || SWFDEC_IS_PLAYER (player), NULL);

  widget = g_object_new (SWFDEC_TYPE_GTK_WIDGET, "player", player, NULL);

  return GTK_WIDGET (widget);
}

/**
 * swfdec_gtk_widget_set_interactive:
 * @widget: a #SwfdecGtkWidget
 * @interactive: %TRUE to make the widget interactive
 *
 * Sets the widget to be interactive or not. An interactive widget processes 
 * mouse and keyboard events, while a non-interactive widget does not care about
 * user input. Widgets are interactive by default.
 **/
void
swfdec_gtk_widget_set_interactive (SwfdecGtkWidget *widget, gboolean interactive)
{
  g_return_if_fail (SWFDEC_IS_GTK_WIDGET (widget));

  widget->priv->interactive = interactive;
  swfdec_gtk_widget_update_cursor (widget);
  g_object_notify (G_OBJECT (widget), "interactive");
}

/**
 * swfdec_gtk_widget_get_interactive:
 * @widget: a #SwfdecGtkWidget
 *
 * Queries if the @widget is currently interactive. See 
 * swfdec_gtk_widget_set_interactive() for details.
 *
 * Returns: %TRUE if the widget is interactive, %FALSE otherwise.
 **/
gboolean
swfdec_gtk_widget_get_interactive (SwfdecGtkWidget *widget)
{
  g_return_val_if_fail (SWFDEC_IS_GTK_WIDGET (widget), FALSE);

  return widget->priv->interactive;
}

/**
 * swfdec_gtk_widget_set_renderer:
 * @widget: a #SwfdecGtkWidget
 * @renderer: a #cairo_surface_type_t for the intermediate renderer
 *
 * Tells @widget to use an intermediate surface for rendering. This is
 * useful for debugging or performance measurements inside swfdec and is 
 * probably not interesting for anyone else.
 **/
void
swfdec_gtk_widget_set_renderer (SwfdecGtkWidget *widget, cairo_surface_type_t renderer)
{
  g_return_if_fail (SWFDEC_IS_GTK_WIDGET (widget));

  widget->priv->renderer_type = renderer;
  if (widget->priv->renderer_set == FALSE) {
    widget->priv->renderer_set = TRUE;
    g_object_notify (G_OBJECT (widget), "renderer-set");
  }
  g_object_notify (G_OBJECT (widget), "renderer");
  swfdec_gtk_widget_update_renderer (widget);
}

/**
 * swfdec_gtk_widget_unset_renderer:
 * @widget: a #SwfdecGtkWidget
 *
 * Unsets the use of an intermediate rendering surface. See 
 * swfdec_gtk_widget_set_renderer() for details.
 **/
void
swfdec_gtk_widget_unset_renderer (SwfdecGtkWidget *widget)
{
  g_return_if_fail (SWFDEC_IS_GTK_WIDGET (widget));

  if (widget->priv->renderer_set == FALSE)
    return;
  widget->priv->renderer_set = FALSE;
  g_object_notify (G_OBJECT (widget), "renderer-set");
  swfdec_gtk_widget_update_renderer (widget);
}

/**
 * swfdec_gtk_widget_get_renderer:
 * @widget: a #SwfdecGtkWidget
 *
 * Gets the intermediate renderer that is or would be in use by @widget. Use
 * swfdec_gtk_widget_uses_renderer() to check if an intermediate renderer is in
 * use. See swfdec_gtk_widget_set_renderer() for details.
 *
 * Returns: the type of the intermediate renderer
 **/
cairo_surface_type_t
swfdec_gtk_widget_get_renderer (SwfdecGtkWidget *widget)
{
  g_return_val_if_fail (SWFDEC_IS_GTK_WIDGET (widget), CAIRO_SURFACE_TYPE_IMAGE);

  return widget->priv->renderer_type;
}

/**
 * swfdec_gtk_widget_uses_renderer:
 * @widget: a #SwfdecGtkWidget
 *
 * Queries if an intermediate renderer set via swfdec_gtk_widget_set_renderer()
 * is currently in use.
 *
 * Returns: %TRUE if an intermediate renderer is used.
 **/
gboolean
swfdec_gtk_widget_uses_renderer (SwfdecGtkWidget *widget)
{
  g_return_val_if_fail (SWFDEC_IS_GTK_WIDGET (widget), FALSE);

  return widget->priv->renderer_set;
}

