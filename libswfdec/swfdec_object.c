
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


void *
swfdec_object_new (GType type)
{
  return g_object_new (type, NULL);
}

void swfdec_object_unref (SwfdecObject *object)
{
  g_object_unref (G_OBJECT (object));
}

SwfdecObject *
swfdec_object_get (SwfdecDecoder * s, int id)
{
  SwfdecObject *object;
  GList *g;

  for (g = g_list_first (s->objects); g; g = g_list_next (g)) {
    object = SWFDEC_OBJECT (g->data);
    if (object->id == id)
      return object;
  }
  SWFDEC_WARNING ("object not found (id==%d)", id);

  return NULL;
}

void
swfdec_object_dump (SwfdecObject *object)
{
  SwfdecObjectClass *klass;

  klass = SWFDEC_OBJECT_GET_CLASS (object);

  g_print ("Object %d:\n", object->id);
  if (klass->dump) {
    klass->dump (object);
  } else {
    g_print("  no dump\n");
  }
}

