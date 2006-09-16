#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "swfdec_widget.h"


G_DEFINE_TYPE (SwfdecWidget, swfdec_widget, GTK_TYPE_WIDGET)


static void
queue_draw (SwfdecWidget *widget)
{
  //g_print ("queing draw of %g %g  %g %g\n", inval.x0, inval.y0, inval.x1, inval.y1);
  gtk_widget_queue_draw (GTK_WIDGET (widget));
  //_area (widget, floor (inval->x0), floor (inval->y0),
  //    ceil (inval->x1) - floor (inval->x0), ceil (inval->y1) - floor (inval->y0));
}

static gboolean
swfdec_widget_motion_notify (GtkWidget *gtkwidget, GdkEventMotion *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);
  int x, y;

  gtk_widget_get_pointer (gtkwidget, &x, &y);

  swfdec_decoder_handle_mouse (widget->dec, event->x, event->y, widget->button);
  queue_draw (widget);
  
  return FALSE;
}

static gboolean
swfdec_widget_leave_notify (GtkWidget *gtkwidget, GdkEventCrossing *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  widget->button = 0;
  swfdec_decoder_handle_mouse (widget->dec, event->x, event->y, 0);
  queue_draw (widget);
  return FALSE;
}

static gboolean
swfdec_widget_button_press (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  if (event->button == 1) {
    widget->button = 1;
    swfdec_decoder_handle_mouse (widget->dec, event->x, event->y, 1);
  }
  queue_draw (widget);
  return FALSE;
}

static gboolean
swfdec_widget_button_release (GtkWidget *gtkwidget, GdkEventButton *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);

  if (event->button == 1) {
    widget->button = 0;
    swfdec_decoder_handle_mouse (widget->dec, event->x, event->y, 0);
  }
  queue_draw (widget);
  return FALSE;
}

static gboolean
swfdec_widget_iterate (gpointer data)
{
  SwfdecWidget *widget = data;

  swfdec_decoder_iterate (widget->dec);
  queue_draw (widget);
  
  return TRUE;
}

static gboolean
swfdec_widget_expose (GtkWidget *gtkwidget, GdkEventExpose *event)
{
  SwfdecWidget *widget = SWFDEC_WIDGET (gtkwidget);
  SwfdecRect rect;
  cairo_t *cr;

  cr = gdk_cairo_create (gtkwidget->window);
  cairo_scale (cr, widget->scale, widget->scale);
  swfdec_rect_init (&rect, event->area.x / widget->scale, event->area.y / widget->scale, 
      event->area.width / widget->scale, event->area.height / widget->scale);

  swfdec_decoder_render (widget->dec, cr, &rect);

  cairo_destroy (cr);

  return FALSE;
}

static void
swfdec_widget_dispose (GObject *object)
{
  SwfdecWidget * widget = SWFDEC_WIDGET (object);

  swfdec_decoder_free (widget->dec);

  G_OBJECT_CLASS (swfdec_widget_parent_class)->dispose (object);
}

static void
swfdec_widget_size_request (GtkWidget *gtkwidget, GtkRequisition *req)
{
  SwfdecWidget * widget = SWFDEC_WIDGET (gtkwidget);

  if (widget->dec == NULL ||
      swfdec_decoder_get_image_size (widget->dec, 
	  &req->width, &req->height) != SWF_OK) {
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
swfdec_widget_map (GtkWidget *gtkwidget)
{
  double rate;
  SwfdecWidget * widget = SWFDEC_WIDGET (gtkwidget);

  GTK_WIDGET_CLASS (swfdec_widget_parent_class)->map (gtkwidget);

  swfdec_decoder_get_rate (widget->dec, &rate);
  widget->timeout = g_timeout_add (1000 / rate, swfdec_widget_iterate, widget);
}

static void
swfdec_widget_unmap (GtkWidget *gtkwidget)
{
  SwfdecWidget * widget = SWFDEC_WIDGET (gtkwidget);

  g_source_remove (widget->timeout);
  widget->timeout = 0;

  GTK_WIDGET_CLASS (swfdec_widget_parent_class)->unmap (gtkwidget);
}

static void
swfdec_widget_class_init (SwfdecWidgetClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (g_class);

  object_class->dispose = swfdec_widget_dispose;

  widget_class->realize = swfdec_widget_realize;
  widget_class->map = swfdec_widget_map;
  widget_class->unmap = swfdec_widget_unmap;
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

GtkWidget *
swfdec_widget_new (SwfdecDecoder *dec)
{
  SwfdecWidget *widget;
  
  g_return_val_if_fail (dec != NULL, NULL);

  widget = g_object_new (SWFDEC_TYPE_WIDGET, 0);
  widget->dec = dec;

  return GTK_WIDGET (widget);
}

void
swfdec_widget_set_scale	(SwfdecWidget *	widget, double scale)
{
  g_return_if_fail (SWFDEC_IS_WIDGET (widget));
  g_return_if_fail (scale > 0.0);

  widget->scale = scale;
  gtk_widget_queue_resize (GTK_WIDGET (widget));
}

double
swfdec_widget_get_scale (SwfdecWidget *	widget)
{
  g_return_val_if_fail (SWFDEC_IS_WIDGET (widget), 1.0);

  return widget->scale;
}

