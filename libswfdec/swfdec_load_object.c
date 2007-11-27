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

  if (load_object->progress != NULL) {
    load_object->progress (load_object->target,
	swfdec_loader_get_loaded (loader), swfdec_loader_get_size (loader));
  }
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

  /* call finish */
  load_object->finish (load_object->target, NULL);

  /* unroot */
  swfdec_player_unroot_object (SWFDEC_PLAYER (
	SWFDEC_AS_OBJECT (load_object)->context), G_OBJECT (load_object));
}

static void
swfdec_load_object_loader_target_eof (SwfdecLoaderTarget *target,
    SwfdecLoader *loader)
{
  SwfdecLoadObject *load_object = SWFDEC_LOAD_OBJECT (target);
  char *text;

  // get text
  text =
    swfdec_loader_get_text (loader, load_object->target->context->version);

  /* break reference to the loader */
  swfdec_loader_set_target (loader, NULL);
  load_object->loader = NULL;
  g_object_unref (loader);

  /* call finish */
  if (text != NULL) {
    load_object->finish (load_object->target, 
	swfdec_as_context_give_string (load_object->target->context, text));
  } else {
    load_object->finish (load_object->target, SWFDEC_AS_STR_EMPTY);
  }

  /* unroot */
  swfdec_player_unroot_object (SWFDEC_PLAYER (
	SWFDEC_AS_OBJECT (load_object)->context), G_OBJECT (load_object));
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

  return TRUE;
}

SwfdecAsObject *
swfdec_load_object_new (SwfdecAsObject *target, const char *url,
    SwfdecLoaderRequest request, SwfdecBuffer *data,
    SwfdecLoadObjectProgress progress, SwfdecLoadObjectFinish finish)
{
  SwfdecLoadObject *load_object;

  g_return_val_if_fail (SWFDEC_IS_AS_OBJECT (target), NULL);
  g_return_val_if_fail (url != NULL, NULL);
  g_return_val_if_fail (finish != NULL, NULL);

  if (!swfdec_as_context_use_mem (target->context, sizeof (SwfdecLoadObject)))
    return NULL;
  load_object = SWFDEC_LOAD_OBJECT (g_object_new (
	SWFDEC_TYPE_LOAD_OBJECT, NULL));
  swfdec_as_object_add (SWFDEC_AS_OBJECT (load_object), target->context,
      sizeof (SwfdecLoadObject));

  load_object->target = target;
  load_object->progress = progress;
  load_object->finish = finish;

  if (!swfdec_load_object_load (load_object, url, request, data))
    return NULL;

  swfdec_player_root_object (SWFDEC_PLAYER (target->context),
      G_OBJECT (load_object));

  return SWFDEC_AS_OBJECT (load_object);
}
