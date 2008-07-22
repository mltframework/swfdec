/* Vivified
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vivi_wrap.h"
#include "vivi_application.h"

G_DEFINE_TYPE (ViviWrap, vivi_wrap, SWFDEC_TYPE_AS_OBJECT)

static void
vivi_wrap_dispose (GObject *object)
{
  ViviWrap *wrap = VIVI_WRAP (object);

  if (wrap->wrap) {
    g_hash_table_remove (VIVI_APPLICATION (SWFDEC_AS_OBJECT (wrap)->context)->wraps, wrap->wrap);
    wrap->wrap = NULL;
  }

  G_OBJECT_CLASS (vivi_wrap_parent_class)->dispose (object);
}

static void
vivi_wrap_class_init (ViviWrapClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_wrap_dispose;
}

static void
vivi_wrap_init (ViviWrap *wrap)
{
}

SwfdecAsObject *
vivi_wrap_object (ViviApplication *app, SwfdecAsObject *object)
{
  const char *name;
  SwfdecAsContext *cx;
  SwfdecAsObject *wrap;
  SwfdecAsValue val;
  
  wrap = g_hash_table_lookup (app->wraps, object);
  if (wrap)
    return wrap;

  cx = SWFDEC_AS_CONTEXT (app);
  swfdec_as_context_use_mem (cx, sizeof (ViviWrap));
  wrap = g_object_new (VIVI_TYPE_WRAP, NULL);
  swfdec_as_object_add (wrap, cx, sizeof (ViviWrap));
  /* frames are special */
  if (SWFDEC_IS_AS_FRAME (object))
    name = "Frame";
  else
    name = "Wrap";
  swfdec_as_object_get_variable (cx->global, swfdec_as_context_get_string (cx, name), &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val))
    swfdec_as_object_set_constructor (wrap, SWFDEC_AS_VALUE_GET_OBJECT (&val));
  VIVI_WRAP (wrap)->wrap = object;
  g_hash_table_insert (app->wraps, object, wrap);
  return wrap;
}

void
vivi_wrap_value (ViviApplication *app, SwfdecAsValue *dest, const SwfdecAsValue *src)
{
  g_return_if_fail (VIVI_IS_APPLICATION (app));
  g_return_if_fail (dest != NULL);
  g_return_if_fail (SWFDEC_IS_AS_VALUE (src));

  switch (src->type) {
    case SWFDEC_AS_TYPE_UNDEFINED:
    case SWFDEC_AS_TYPE_BOOLEAN:
    case SWFDEC_AS_TYPE_NUMBER:
    case SWFDEC_AS_TYPE_NULL:
      *dest = *src;
      break;
    case SWFDEC_AS_TYPE_STRING:
      SWFDEC_AS_VALUE_SET_STRING (dest,
	  swfdec_as_context_get_string (SWFDEC_AS_CONTEXT (app),
	    SWFDEC_AS_VALUE_GET_STRING (src)));
      break;
    case SWFDEC_AS_TYPE_OBJECT:
      SWFDEC_AS_VALUE_SET_OBJECT (dest,
	  vivi_wrap_object (app, SWFDEC_AS_VALUE_GET_OBJECT (src)));
      break;
    case SWFDEC_AS_TYPE_INT:
    default:
      g_assert_not_reached ();
      break;
  }
}

