
#include "swfdec_internal.h"

#include <swfdec_object.h>


static void swfdec_object_base_init (gpointer g_class);
static void swfdec_object_class_init (gpointer g_class, gpointer user_data);
static void swfdec_object_init (GTypeInstance *instance, gpointer g_class);
static void swfdec_object_dispose (GObject *object);


GType _swfdec_object_type;

static GObjectClass *parent_class = NULL;

GType swfdec_object_get_type (void)
{
  if (!_swfdec_object_type) {
    static const GTypeInfo object_info = {
      sizeof (SwfdecObjectClass),
      swfdec_object_base_init,
      NULL,
      swfdec_object_class_init,
      NULL,
      NULL,
      sizeof (SwfdecObject),
      32,
      swfdec_object_init,
      NULL
    };
    _swfdec_object_type = g_type_register_static (G_TYPE_OBJECT,
        "SwfdecObject", &object_info, 0);
  }
  return _swfdec_object_type;
}

static void swfdec_object_base_init (gpointer g_class)
{
  //GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

}

static void swfdec_object_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  gobject_class->dispose = swfdec_object_dispose;

  parent_class = g_type_class_peek_parent (gobject_class);

}

static void swfdec_object_init (GTypeInstance *instance, gpointer g_class)
{

}

static void swfdec_object_dispose (GObject *object)
{

}



SwfdecObject *
swfdec_object_new (SwfdecDecoder * s, int id)
{
  SwfdecObject *obj;

  obj = g_new0 (SwfdecObject, 1);

  obj->id = id;

  s->objects = g_list_append (s->objects, obj);

  return obj;
}

SwfdecObject *
swfdec_object_get (SwfdecDecoder * s, int id)
{
  SwfdecObject *object;
  GList *g;

  for (g = g_list_first (s->objects); g; g = g_list_next (g)) {
    object = (SwfdecObject *) g->data;
    if (object->id == id)
      return object;
  }
  SWF_DEBUG (2, "object not found (id==%d)\n", id);

  return NULL;
}

