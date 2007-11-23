/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource_request.h"

/*** SWFDEC_LOADER_TARGET ***/

static SwfdecPlayer *
swfdec_load_object_loader_target_get_player (SwfdecLoaderTarget *target)
{
  return SWFDEC_PLAYER (SWFDEC_AS_OBJECT (target)->context);
}

static void
swfdec_load_object_loader_target_parse (SwfdecLoaderTarget *target,
    SwfdecLoader *loader)
{
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (target);
  SwfdecAsValue val;
  glong size;

  SWFDEC_AS_VALUE_SET_NUMBER (&val, swfdec_loader_get_loaded (loader));
  swfdec_as_object_set_variable_and_flags (load_object->target,
      SWFDEC_AS_STR__bytesLoaded, &val, SWFDEC_AS_VARIABLE_HIDDEN);

  size = swfdec_loader_get_size (loader);
  if (size < 0) 
    size = swfdec_loader_get_loaded (loader);
  SWFDEC_AS_VALUE_SET_NUMBER (&val, size);
  swfdec_as_object_set_variable_and_flags (load_object->target,
      SWFDEC_AS_STR__bytesTotal, &val, SWFDEC_AS_VARIABLE_HIDDEN);
}

static void
swfdec_load_object_ondata (SwfdecLoadObject *load_object)
{
  SwfdecAsValue val;

  if (load_object->text) {
    SWFDEC_AS_VALUE_SET_STRING (&val,
	swfdec_as_context_get_string (load_object->target->context,
	  load_object->text));
  } else {
    SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  }
  swfdec_as_object_call (load_object->target, SWFDEC_AS_STR_onData, 1, &val,
      NULL);
  swfdec_player_unroot_object (SWFDEC_PLAYER (SWFDEC_AS_OBJECT (load_object)->context), 
      G_OBJECT (load_object));
}

static void
swfdec_load_object_loader_target_error (SwfdecLoaderTarget *target,
    SwfdecLoader *loader)
{
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (target);

  /* break reference to the loader */
  swfdec_loader_set_target (loader, NULL);
  load_object->loader = NULL;
  g_object_unref (loader);
  /* emit onData */
  swfdec_load_object_ondata (load_object);
}

typedef struct {
  const char		*name;
  guint			length;
  guchar		data[4];
} ByteOrderMark;

static ByteOrderMark boms[] = {
  /*{ "UTF-32BE", 4, {0x00, 0x00, 0xFE, 0xFF} },
  { "UTF-32LE", 4, {0xFE, 0xFF, 0x00, 0x00} },*/
  { "UTF-8", 3, {0xEF, 0xBB, 0xBF, 0} },
  { "UTF-16BE", 2, {0xFE, 0xFF, 0, 0} },
  { "UTF-16LE", 2, {0xFF, 0xFE, 0, 0} },
  { "UTF-8", 0, {0, 0, 0, 0} }
};

static void
swfdec_load_object_loader_target_eof (SwfdecLoaderTarget *target,
    SwfdecLoader *loader)
{
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (target);
  char *text;
  guint size;

  /* get the text from the loader */
  size = swfdec_buffer_queue_get_depth (loader->queue);
  text = g_try_malloc (size + 1);
  if (text) {
    SwfdecBuffer *buffer;
    guint i = 0, j;
    while ((buffer = swfdec_buffer_queue_pull_buffer (loader->queue))) {
      memcpy (text + i, buffer->data, buffer->length);
      i += buffer->length;
      swfdec_buffer_unref (buffer);
    }
    g_assert (i == size);
    text[size] = '\0';

    for (i = 0; boms[i].length > 0; i++) {
      if (size < boms[i].length)
	continue;

      for (j = 0; j < boms[i].length; j++) {
	if ((guchar)text[j] != boms[i].data[j])
	  break;
      }
      if (j == boms[i].length)
	break;
    }

    if (!strcmp (boms[i].name, "UTF-8")) {
      if (!g_utf8_validate (text + boms[i].length, -1, NULL)) {
	SWFDEC_ERROR ("downloaded data is not valid UTF-8");
	g_free (text);
	text = NULL;
	load_object->text = NULL;
      } else {
	if (boms[i].length == 0) {
	  load_object->text = text;
	  text = NULL;
	} else {
	  load_object->text = g_strdup (text + boms[i].length);
	  g_free (text);
	  text = NULL;
	}
      }
    } else {
      load_object->text = g_convert (text + boms[i].length, -1, "UTF-8",
	  boms[i].name, NULL, NULL, NULL);
      if (load_object->text == NULL)
	SWFDEC_ERROR ("downloaded data is not valid %s", boms[i].name);
      g_free (text);
      text = NULL;
    }
  } else {
    SWFDEC_ERROR ("not enough memory to copy %u bytes", size);
    load_object->text = NULL;
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
  iface->parse = swfdec_load_object_loader_target_parse;
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
swfdec_load_object_mark (SwfdecAsObject *object)
{
  swfdec_as_object_mark (SWFDEC_LOAD_OBJECT (object)->target);

  SWFDEC_AS_OBJECT_CLASS (swfdec_load_object_parent_class)->mark (object);
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
  SwfdecAsObjectClass *as_object_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_load_object_dispose;

  as_object_class->mark = swfdec_load_object_mark;
}

static void
swfdec_load_object_init (SwfdecLoadObject *load_object)
{
}

static void
swfdec_load_object_got_loader (SwfdecPlayer *player, SwfdecLoader *loader, gpointer obj)
{
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (obj);

  if (loader == NULL) {
    return;
  }
  load_object->loader = loader;

  swfdec_loader_set_target (load_object->loader,
      SWFDEC_LOADER_TARGET (load_object));
  swfdec_loader_set_data_type (load_object->loader, SWFDEC_LOADER_DATA_TEXT);
}

static gboolean
swfdec_load_object_load (SwfdecLoadObject *load_object, const char *url, 
    SwfdecLoaderRequest request, SwfdecBuffer *data)
{
  SwfdecPlayer *player;
  SwfdecSecurity *sec;
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_LOAD_OBJECT (load_object), FALSE);
  g_return_val_if_fail (url != NULL, FALSE);

  player = SWFDEC_PLAYER (SWFDEC_AS_OBJECT (load_object)->context);
  swfdec_load_object_reset (load_object);
  /* get the current security */
  g_assert (SWFDEC_AS_CONTEXT (player)->frame);
  sec = SWFDEC_AS_CONTEXT (player)->frame->security;
  g_object_ref (load_object);
  swfdec_player_request_resource (player, sec, url, request, data,
      swfdec_load_object_got_loader, load_object, g_object_unref);

  SWFDEC_AS_VALUE_SET_INT (&val, 0);
  swfdec_as_object_set_variable_and_flags (load_object->target,
      SWFDEC_AS_STR__bytesLoaded, &val, SWFDEC_AS_VARIABLE_HIDDEN);
  SWFDEC_AS_VALUE_SET_UNDEFINED (&val);
  swfdec_as_object_set_variable_and_flags (load_object->target,
      SWFDEC_AS_STR__bytesTotal, &val, SWFDEC_AS_VARIABLE_HIDDEN);

  SWFDEC_AS_VALUE_SET_BOOLEAN (&val, FALSE);
  swfdec_as_object_set_variable_and_flags (load_object->target,
      SWFDEC_AS_STR_loaded, &val, SWFDEC_AS_VARIABLE_HIDDEN);
  return TRUE;
}

SwfdecAsObject *
swfdec_load_object_new (SwfdecAsObject *target, const char *url,
    SwfdecLoaderRequest request, SwfdecBuffer *data)
{
  SwfdecAsObject *load_object;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (target), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  if (!swfdec_as_context_use_mem (target->context, sizeof (SwfdecLoadObject)))
    return NULL;
  load_object = g_object_new (SWFDEC_TYPE_LOAD_OBJECT, NULL);
  swfdec_as_object_add (load_object, target->context,
      sizeof (SwfdecLoadObject));

  SWFDEC_LOAD_OBJECT (load_object)->target = target;

  if (!swfdec_load_object_load (SWFDEC_LOAD_OBJECT (load_object), url, request, data))
    return NULL;

  swfdec_player_root_object (SWFDEC_PLAYER (target->context), G_OBJECT (load_object));

  return load_object;
}
