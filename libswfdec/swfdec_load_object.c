/* Swfdec
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

#include <string.h>
#include "swfdec_load_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"

/*** SWFDEC_LOADER_TARGET ***/

static SwfdecPlayer *
swfdec_load_object_loader_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_PLAYER (SWFDEC_AS_OBJECT (target)->context);
}

static void
swfdec_load_object_ondata (SwfdecLoadObject *load_object)
{
  SwfdecAsValue val;

  if (load_object->text) {
    SWFDEC_AS_VALUE_SET_STRING (&val,
	swfdec_as_context_get_string (SWFDEC_AS_OBJECT (load_object->target)->context, load_object->text));
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  }
  swfdec_as_object_call (SWFDEC_AS_OBJECT (load_object->target), SWFDEC_AS_STR_onData, 1, &val, NULL);
}

static void
swfdec_load_object_loader_target_error (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (target);

  /* break reference to the loader */
  swfdec_loader_set_target (loader, NULL);
  load_object->loader = NULL;
  g_object_unref (loader);
  /* emit onData */
  swfdec_load_object_ondata (load_object);
}

static void
swfdec_load_object_loader_target_eof (SwfdecLoaderTarget *target, SwfdecLoader *loader)
{
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (target);
  guint size;

  /* get the text from the loader */
  size = swfdec_buffer_queue_get_depth (loader->queue);
  load_object->text = g_try_malloc (size + 1);
  if (load_object->text) {
    SwfdecBuffer *buffer;
    guint i = 0;
    while ((buffer = swfdec_buffer_queue_pull_buffer (loader->queue))) {
      memcpy (load_object->text + i, buffer->data, buffer->length);
      i += buffer->length;
      swfdec_buffer_unref (buffer);
    }
    g_assert (i == size);
    load_object->text[size] = '\0';
    /* FIXME: validate otherwise? */
    if (!g_utf8_validate (load_object->text, size, NULL)) {
      SWFDEC_ERROR ("downloaded data is not valid utf-8");
      g_free (load_object->text);
      load_object->text = NULL;
    }
  } else {
    SWFDEC_ERROR ("not enough memory to copy %u bytes", size);
  }

  /* break reference to the loader */
  swfdec_loader_set_target (loader, NULL);
  load_object->loader = NULL;
  g_object_unref (loader);
  /* emit onData */
  swfdec_load_object_ondata (load_object);
}

static void
swfdec_load_object_loader_target_init (SwfdecLoaderTargetInterface *iface)
{
  iface->get_player = swfdec_load_object_loader_target_get_player;
  iface->eof = swfdec_load_object_loader_target_eof;
  iface->error = swfdec_load_object_loader_target_error;
}

/*** SWFDEC_LOAD_OBJECT ***/

G_DEFINE_TYPE_WITH_CODE (SwfdecLoadObject, swfdec_load_object, SWFDEC_TYPE_AS_OBJECT,
    G_IMPLEMENT_INTERFACE (SWFDEC_TYPE_LOADER_TARGET, swfdec_load_object_loader_target_init))

static void
swfdec_load_object_reset (SwfdecLoadObject *load_object)
{
  if (load_object->loader) {
    swfdec_loader_set_target (load_object->loader, NULL);
    g_object_unref (load_object->loader);
    load_object->loader = NULL;
  }
  g_free (load_object->text);
  load_object->text = NULL;
}

static void
swfdec_load_object_dispose (GObject *object)
{
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (object);

  swfdec_load_object_reset (load_object);

  G_OBJECT_CLASS (swfdec_load_object_parent_class)->dispose (object);
}

static void
swfdec_load_object_class_init (SwfdecLoadObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_load_object_dispose;
}

static void
swfdec_load_object_init (SwfdecLoadObject *load_object)
{
}

static void
swfdec_load_object_load (SwfdecLoadObject *load_object, const char *url)
{
  g_return_if_fail (SWFDEC_IS_LOAD_OBJECT (load_object));
  g_return_if_fail (url != NULL);

  swfdec_load_object_reset (load_object);
  load_object->loader = swfdec_player_load (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (load_object)->context), url);
  swfdec_loader_set_target (load_object->loader, SWFDEC_LOADER_TARGET (load_object));
  swfdec_loader_set_data_type (load_object->loader, SWFDEC_LOADER_DATA_TEXT);
}

SwfdecAsObject *
swfdec_load_object_new (SwfdecAsObject *target, const char *url)
{
  SwfdecAsObject *load_object;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (target), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  if (!swfdec_as_context_use_mem (target->context, sizeof (SwfdecLoadObject)))
    return NULL;
  load_object = g_object_new (SWFDEC_TYPE_LOAD_OBJECT, NULL);
  swfdec_as_object_add (load_object, target->context, sizeof (SwfdecLoadObject));

  SWFDEC_LOAD_OBJECT (load_object)->target = target;

  SWFDEC_PLAYER (target->context)->load_objects =
    g_list_append (SWFDEC_PLAYER (target->context)->load_objects, load_object);

  swfdec_load_object_load (SWFDEC_LOAD_OBJECT (load_object), url);

  return load_object;
}
