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

  gtk_widget_get_pointer (gtkwidget, &x, &y);

  swfdec_decoder_handle_mouse (widget->dec, 
      event->x / widget->scale, event->y / widget->scale, widget->button);
  
  return FALSE;
}

static gboolean
swfdec_widget_leave_notify (GtkWidget *gtkwidget, GdkEventCrossing *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  widget->button = 0;
  swfdec_decoder_handle_mouse (widget->dec, 
      event->x / widget->scale, event->y / widget->scale, 0);
  return FALSE;
}

static gboolean
swfdec_widget_button_press (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  if (event->button == 1) {
    widget->button = 1;
    swfdec_decoder_handle_mouse (widget->dec, 
	event->x / widget->scale, event->y / widget->scale, 1);
  }
  return FALSE;
}

static gboolean
swfdec_widget_button_release (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  if (event->button == 1) {
    widget->button = 0;
    swfdec_decoder_handle_mouse (widget->dec, 
	event->x / widget->scale, event->y / widget->scale, 0);
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

  if (!widget->use_image) {
    cr = gdk_cairo_create (gtkwidget->window);
  } else {
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 
      event->area.width, event->area.height);
    cr = cairo_create (surface);
    cairo_translate (cr, -event->area.x, -event->area.y);
    cairo_surface_destroy (surface);
  }
  cairo_scale (cr, widget->scale, widget->scale);
  swfdec_rect_init (&rect, event->area.x / widget->scale, event->area.y / widget->scale, 
      event->area.width / widget->scale, event->area.height / widget->scale);
  swfdec_decoder_render (widget->dec, cr, &rect);
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
  SwfdecWidget * widget = SWFDEC_WIDGET (object);

  swfdec_widget_set_decoder (widget, NULL);

  G_OBJECT_CLASS (swfdec_widget_parent_class)->dispose (object);
}

static void
swfdec_widget_size_request (GtkWidget *gtkwidget, GtkRequisition *req)
{
  SwfdecWidget * widget = SWFDEC_WIDGET (gtkwidget);

  if (widget->dec == NULL ||
      !swfdec_decoder_get_image_size (widget->dec, 
	  &req->width, &req->height)) {
    req->width = req->height = 0;
  } 
  req->width = ceil (req->width * widget->scale);
  req->height = ceil (req->height * widget->scale);
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
}

static void
swfdec_widget_class_init (SwfdecWidgetClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (g_class);

  object_class->dispose = swfdec_widget_dispose;

  widget_class->realize = swfdec_widget_realize;
  widget_class->size_request = swfdec_widget_size_request;
  widget_class->expose_event = swfdec_widget_expose;
  widget_class->button_press_event = swfdec_widget_button_press;
  widget_class->button_release_event = swfdec_widget_button_release;
  widget_class->motion_notify_event = swfdec_widget_motion_notify;
  widget_class->leave_notify_event = swfdec_widget_leave_notify;
}

static void
swfdec_widget_init (SwfdecWidget * widget)
{
  widget->scale = 1.0;
}

static void
swfdec_widget_notify_cb (SwfdecDecoder *dec, GParamSpec *pspec, SwfdecWidget *widget)
{
  SwfdecRect rect;

  swfdec_decoder_get_invalid (widget->dec, &rect);
  if (swfdec_rect_is_empty (&rect))
    return;
  swfdec_rect_scale (&rect, &rect, widget->scale);
  //g_print ("queing draw of %g %g  %g %g\n", inval.x0, inval.y0, inval.x1, inval.y1);
  gtk_widget_queue_draw_area (GTK_WIDGET (widget), floor (rect.x0), floor (rect.y0),
      ceil (rect.x1) - floor (rect.x0), ceil (rect.y1) - floor (rect.y0));
  swfdec_decoder_clear_invalid (widget->dec);
}

static void
swfdec_widget_notify_mouse_cb (SwfdecDecoder *dec, GParamSpec *pspec, SwfdecWidget *widget)
{
  GdkWindow *window = GTK_WIDGET (widget)->window;
  gboolean visible;

  if (window == NULL)
    return;
  g_object_get (dec, "mouse-visible", &visible, NULL);

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

void
swfdec_widget_set_decoder (SwfdecWidget *widget, SwfdecDecoder *dec)
{
  g_return_if_fail (SWFDEC_IS_WIDGET (widget));
  g_return_if_fail (dec == NULL || SWFDEC_IS_DECODER (dec));
  
  if (widget->dec) {
    g_signal_handlers_disconnect_by_func (widget->dec, swfdec_widget_notify_cb, widget);
    g_object_unref (widget->dec);
  }
  widget->dec = dec;
  if (dec) {
    g_signal_connect (dec, "notify::invalid", G_CALLBACK (swfdec_widget_notify_cb), widget);
    g_signal_connect (dec, "notify::mouse-visible", G_CALLBACK (swfdec_widget_notify_mouse_cb), widget);
    g_object_ref (dec);
    swfdec_widget_notify_mouse_cb (dec, NULL, widget);
  } else {
    if (GTK_WIDGET (widget)->window)
      gdk_window_set_cursor (GTK_WIDGET (widget)->window, NULL); 
  }
  gtk_widget_queue_resize (GTK_WIDGET (widget));
}

GtkWidget *
swfdec_widget_new (SwfdecDecoder *dec)
{
  SwfdecWidget *widget;
  
  g_return_val_if_fail (dec == NULL || SWFDEC_IS_DECODER (dec), NULL);

  widget = g_object_new (SWFDEC_TYPE_WIDGET, 0);
  swfdec_widget_set_decoder (widget, dec);

  return GTK_WIDGET (widget);
}

void
swfdec_widget_set_scale	(SwfdecWidget *widget, double scale)
{
  g_return_if_fail (SWFDEC_IS_WIDGET (widget));
  g_return_if_fail (scale > 0.0);

  widget->scale = scale;
  gtk_widget_queue_resize (GTK_WIDGET (widget));
}

double
swfdec_widget_get_scale (SwfdecWidget *widget)
{
  g_return_val_if_fail (SWFDEC_IS_WIDGET (widget), 1.0);

  return widget->scale;
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
