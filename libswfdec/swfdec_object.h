

#ifndef _SWFDEC_OBJECT_H_
#define _SWFDEC_OBJECT_H_

#include <glib-object.h>
#include "swfdec_types.h"
#include "swfdec_transform.h"

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

#define SWFDEC_OBJECT_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_OBJECT, SwfdecObjectClass))

struct _SwfdecObject {
  GObject object;

  int id;

  SwfdecTransform transform;

  int number;
};

struct _SwfdecObjectClass {
  GObjectClass object_class;

  void (*render) (SwfdecDecoder *decoder, SwfdecSpriteSegment *seg,
      SwfdecObject *object);
  
  void (*dump) (SwfdecObject *object);
};

GType swfdec_object_get_type (void);

SwfdecObject *swfdec_object_get (SwfdecDecoder * s, int id);

void swfdec_object_dump (SwfdecObject *object);


#define SWFDEC_OBJECT_BOILERPLATE(type, type_as_function) \
static void type_as_function ## _base_init (gpointer g_class); \
static void type_as_function ## _class_init (type ## Class *g_class); \
static void type_as_function ## _init (type *object); \
static void type_as_function ## _dispose (type *object); \
 \
static SwfdecObjectClass *parent_class = NULL; \
 \
static void \
type_as_function ## _class_init_trampoline (gpointer g_class, gpointer data) \
{ \
  parent_class = (SwfdecObjectClass *) g_type_class_peek_parent (g_class); \
  \
  G_OBJECT_CLASS (g_class)->dispose = (void (*)(GObject *)) \
    type_as_function ## _dispose; \
  type_as_function ## _class_init ((type ## Class *)g_class); \
} \
 \
GType type_as_function ## _get_type (void) \
{ \
  static GType _type; \
  if (!_type) { \
    static const GTypeInfo object_info = { \
      sizeof (type ## Class), \
      type_as_function ## _base_init, \
      NULL, \
      type_as_function ## _class_init_trampoline, \
      NULL, \
      NULL, \
      sizeof (type), \
      0, \
      (void (*)(GTypeInstance *,gpointer))type_as_function ## _init, \
      NULL \
    }; \
    _type = g_type_register_static (SWFDEC_TYPE_OBJECT, \
      #type , &object_info, 0); \
  } \
  return _type; \
}


G_END_DECLS

#endif

