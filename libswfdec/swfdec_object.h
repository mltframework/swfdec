

#ifndef _SWFDEC_OBJECT_H_
#define _SWFDEC_OBJECT_H_

#include <glib-object.h>
#include "swfdec_types.h"

G_BEGIN_DECLS

enum
{
  SWF_OBJECT_SHAPE,
  SWF_OBJECT_TEXT,
  SWF_OBJECT_FONT,
  SWF_OBJECT_SPRITE,
  SWF_OBJECT_BUTTON,
  SWF_OBJECT_SOUND,
  SWF_OBJECT_IMAGE,
};

//typedef struct _SwfdecObject SwfdecObject;
typedef struct _SwfdecObjectClass SwfdecObjectClass;

#define SWFDEC_TYPE_OBJECT                    (swfdec_object_get_type()) 
#define SWFDEC_IS_OBJECT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_OBJECT))
#define SWFDEC_IS_OBJECT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_OBJECT))
#define SWFDEC_OBJECT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_OBJECT, SwfdecObject))
#define SWFDEC_OBJECT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_OBJECT, SwfdecObjectClass))

struct _SwfdecObject {
  GObject object;

  int id;
  int type;

  double trans[6];

  int number;

  void *priv;
};

struct _SwfdecObjectClass {
  GObjectClass object_class;

};

GType swfdec_object_get_type (void);

SwfdecObject *swfdec_object_new (SwfdecDecoder * s, int id);
SwfdecObject *swfdec_object_get (SwfdecDecoder * s, int id);


G_END_DECLS

#endif

