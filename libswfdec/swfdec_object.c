
#include "swfdec_internal.h"

#include <swfdec_object.h>
#include <string.h>


#ifndef GLIB_COMPAT
static void swfdec_object_base_init (gpointer g_class);
static void swfdec_object_class_init (gpointer g_class, gpointer user_data);
static void swfdec_object_init (GTypeInstance * instance, gpointer g_class);
static void swfdec_object_dispose (GObject * object);


static GType _swfdec_object_type;

static GObjectClass *parent_class = NULL;

GType
swfdec_object_get_type (void)
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

static void
swfdec_object_base_init (gpointer g_class)
{
  //GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

}

static void
swfdec_object_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  gobject_class->dispose = swfdec_object_dispose;

  parent_class = g_type_class_peek_parent (gobject_class);
}

static void
swfdec_object_init (GTypeInstance * instance, gpointer g_class)
{
  SWFDEC_LOG ("ref: created %s %p", G_OBJECT_CLASS_NAME (g_class), instance);

}

static void
swfdec_object_dispose (GObject * object)
{
  SWFDEC_LOG ("ref: disposed %s %p", G_OBJECT_TYPE_NAME (object), object);
  
  G_OBJECT_CLASS (parent_class)->dispose (object);
}
#endif

void *
swfdec_object_new (GType type)
{
  return g_object_new (type, NULL);
}

void
swfdec_object_unref (SwfdecObject * object)
{
  SWFDEC_LOG ("ref: unreffing %p", object);

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
swfdec_object_iterate (SwfdecDecoder *s, SwfdecObject *object, unsigned int frame, 
    const cairo_matrix_t *matrix, const SwfdecMouseInfo *mouse, SwfdecRect *inval)
{
  SwfdecObjectClass *klass;

  g_return_if_fail (s != NULL);
  g_return_if_fail (SWFDEC_IS_OBJECT (object));
  g_return_if_fail (inval != NULL);

  klass = SWFDEC_OBJECT_GET_CLASS (object);
  if (klass->iterate == NULL)
    return;

  if (matrix) {
    SwfdecRect rect;
    SwfdecMouseInfo tmp;
    tmp = *mouse;
    cairo_matrix_transform_point (matrix, &tmp.x, &tmp.y);
    klass->iterate (s, object, frame, &tmp, &rect);
    swfdec_rect_apply_matrix (inval, &rect, matrix);
    g_print ("%s %p: invalid area now %g %g  %g %g\n", G_OBJECT_TYPE_NAME (object),
	object, inval->x0, inval->y0, inval->x1, inval->y1);
  } else {
    klass->iterate (s, object, frame, mouse, inval);
  }
}

void
swfdec_object_render (SwfdecDecoder *s, SwfdecObject *object, cairo_t *cr, 
    const cairo_matrix_t *matrix, const SwfdecColorTransform *color, const SwfdecRect *inval)
{
  SwfdecObjectClass *klass;
  SwfdecRect rect;

  g_return_if_fail (s != NULL);
  g_return_if_fail (SWFDEC_IS_OBJECT (object));
  g_return_if_fail (cr != NULL);
  if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) {
    g_warning ("%s", cairo_status_to_string (cairo_status (cr)));
  }
  g_return_if_fail (color != NULL);
  g_return_if_fail (inval != NULL);
  
  klass = SWFDEC_OBJECT_GET_CLASS (object);
  if (klass->render == NULL)
    return;

  cairo_save (cr);
  if (matrix) {
    cairo_transform (cr, matrix);
    swfdec_rect_apply_matrix (&rect, inval, matrix);
  } else {
    rect = *inval;
  }
  //if (swfdec_rect_intersect (NULL, &object->extents, &rect)) {
    SWFDEC_LOG ("really rendering %s %p (id %d)", G_OBJECT_TYPE_NAME (object), object, object->id);
    klass->render (s, cr, color, object, &rect);
  //}
  cairo_restore (cr);
}

