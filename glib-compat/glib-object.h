
#ifndef __GLIB_OBJECT_H__
#define __GLIB_OBJECT_H__

#include <glib.h>

#define G_TYPE_CHECK_INSTANCE_TYPE(obj,gtype) (G_TYPE_FROM_INSTANCE(obj) == (gtype))
#define G_TYPE_CHECK_CLASS_TYPE(klass,gtype)  ((GType)klass == gtype)
#define G_TYPE_CHECK_INSTANCE_CAST(obj,gtype,type) ((type *)(obj))
#define G_TYPE_CHECK_CLASS_CAST(klass,gtype,type) ((type *)(klass))
#define G_TYPE_FROM_INSTANCE(obj) ((GType)((GObject *)obj)->klass)
#define G_TYPE_INSTANCE_GET_CLASS(obj,gtype,type) ((type *)((GObject *)obj)->klass)

#define G_OBJECT(obj) ((GObject *)(obj))

typedef unsigned long GType;
typedef struct _GObject GObject;
typedef struct _GObjectClass GObjectClass;

struct _GObject {
  GObjectClass *klass;
  int refcount;
};

struct _GObjectClass {
  int size;
  char *name;
  void (*dispose) (GObject *);
  void (*base_init) (gpointer);
  void (*class_init) (GObjectClass *);
  void (*init) (GObject *);
};

const char * g_type_name (GType type);
void * g_object_new (GType type, void *ignore);
void g_object_unref (void *ptr);
void g_type_init (void);

#endif /* __G_LIB_H__ */
