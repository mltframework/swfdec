
#ifndef _SWFDEC_BUTTON_H_
#define _SWFDEC_BUTTON_H_

#include <glib-object.h>
#include <swfdec_object.h>

G_BEGIN_DECLS

//typedef struct _SwfdecButton SwfdecButton;
typedef struct _SwfdecButtonClass SwfdecButtonClass;

#define SWFDEC_TYPE_BUTTON                    (swfdec_button_get_type()) 
#define SWFDEC_IS_BUTTON(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_BUTTON))
#define SWFDEC_IS_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_BUTTON))
#define SWFDEC_BUTTON(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_BUTTON, SwfdecButton))
#define SWFDEC_BUTTON_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_BUTTON, SwfdecButtonClass))

struct _SwfdecButton {
  SwfdecObject object;

  SwfdecSpriteSegment *state[3];
};

struct _SwfdecButtonClass {
  SwfdecObjectClass object_class;

};

GType swfdec_button_get_type (void);

G_END_DECLS

#endif

