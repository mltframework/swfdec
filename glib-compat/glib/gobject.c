
#include <glib-object.h>


void g_type_init (void)
{

}

const char *
g_type_name (GType type)
{
  GObjectClass *klass = (GObjectClass *)type;

  return klass->name;
}

void *
g_object_new (GType type, void *ignore)
{
  GObject *object;
  GObjectClass *klass = (GObjectClass *)type;

  object = g_malloc (klass->size);
  memset (object, 0, klass->size);

  object->refcount = 1;
  object->klass = (GObjectClass *)klass;
  if (klass->init) {
    klass->init ((GObject *)object);
  }
  return object;
}

void g_object_unref (void *ptr)
{
  GObject *object = ptr;
  GObjectClass *klass = (GObjectClass *)(((GObject *)object)->klass);

  object->refcount--;
  if (object->refcount < 1) {
    if (klass->dispose) {
      klass->dispose (object);
    }
    g_free (object);
  }
}

