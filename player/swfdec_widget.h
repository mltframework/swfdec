
#ifndef _SWFDEC_WIDGET_H_
#define _SWFDEC_WIDGET_H_

#include <gtk/gtk.h>
#include <swfdec.h>

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
void		swfdec_widget_set_player	(SwfdecWidget *	widget,
						 SwfdecPlayer *	player);

G_END_DECLS
#endif
