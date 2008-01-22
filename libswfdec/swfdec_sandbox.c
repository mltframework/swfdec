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

#include "swfdec_sandbox.h"
#include "swfdec_as_internal.h"
#include "swfdec_debug.h"
#include "swfdec_initialize.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"

/*** GTK-DOC ***/

/**
 * SECTION:SwfdecSandbox
 * @title: SwfdecSandbox
 * @short_description: global object used for security
 *
 * The SwfdecSandbox object is a garbage-collected script object that does two
 * things. The simple thing is its use as the global object while code is 
 * executed in a #SwfdecPlayer. So you can always assume that the global object
 * is a #SwfdecSandbox. The second task it fulfills is acting as the security
 * mechanism used by native functions to determine if a given action should be
 * allowed or not. This is easy, because script functions can always refer to
 * it as the global object.
 */

G_DEFINE_TYPE (SwfdecSandbox, swfdec_sandbox, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_sandbox_mark (SwfdecAsObject *object)
{
  SwfdecSandbox *sandbox = SWFDEC_SANDBOX (object);

  swfdec_as_object_mark (sandbox->Function);
  swfdec_as_object_mark (sandbox->Function_prototype);
  swfdec_as_object_mark (sandbox->Object);
  swfdec_as_object_mark (sandbox->Object_prototype);
  swfdec_as_object_mark (sandbox->MovieClip);
  swfdec_as_object_mark (sandbox->Video);

  SWFDEC_AS_OBJECT_CLASS (swfdec_sandbox_parent_class)->mark (object);
}

static void
swfdec_sandbox_dispose (GObject *object)
{
  SwfdecSandbox *sandbox = SWFDEC_SANDBOX (object);

  swfdec_url_free (sandbox->url);

  G_OBJECT_CLASS (swfdec_sandbox_parent_class)->dispose (object);
}

static void
swfdec_sandbox_class_init (SwfdecSandboxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecAsObjectClass *asobject_class = SWFDEC_AS_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_sandbox_dispose;

  asobject_class->mark = swfdec_sandbox_mark;
}

static void
swfdec_sandbox_init (SwfdecSandbox *sandbox)
{
  sandbox->type = SWFDEC_SANDBOX_NONE;
}

static void
swfdec_sandbox_initialize (SwfdecSandbox *sandbox, guint version)
{
  SwfdecAsContext *context = SWFDEC_AS_OBJECT (sandbox)->context;
  SwfdecPlayer *player = SWFDEC_PLAYER (context);

  swfdec_sandbox_use (sandbox);
  if (context->state == SWFDEC_AS_CONTEXT_RUNNING)
    context->state = SWFDEC_AS_CONTEXT_NEW;
  swfdec_as_context_startup (context);
  /* reset state for initialization */
  /* FIXME: have a better way to do this */
  context->state = SWFDEC_AS_CONTEXT_NEW;
  swfdec_sprite_movie_init_context (player);
  swfdec_video_movie_init_context (player);
  swfdec_net_stream_init_context (player);

  swfdec_as_context_run_init_script (context, swfdec_initialize, 
      sizeof (swfdec_initialize), version);

  sandbox->Function = context->Function;
  sandbox->Function_prototype = context->Function_prototype;
  sandbox->Object = context->Object;
  sandbox->Object_prototype = context->Object_prototype;

  if (context->state == SWFDEC_AS_CONTEXT_NEW)
    context->state = SWFDEC_AS_CONTEXT_RUNNING;
  swfdec_sandbox_unuse (sandbox);
}
/**
 * swfdec_sandbox_set_allow_network:
 * @sandbox: a #SwfdecSandbox
 * finished, by giving the sandbox network or local file access. This function
 * should be called on all return values of swfdec_sandbox_get_for_url().
 *
 * Returns: %TRUE if the sandbox initialization could be finished as requested,
 *          %FALSE if not and it shouldn't be used.
 **/
static gboolean
swfdec_sandbox_set_allow_network (SwfdecSandbox *sandbox, gboolean network)
{
  g_return_val_if_fail (SWFDEC_IS_SANDBOX (sandbox), FALSE);

  switch (sandbox->type) {
    case SWFDEC_SANDBOX_REMOTE:
      return TRUE;
    case SWFDEC_SANDBOX_LOCAL_FILE:
      return !network;
    case SWFDEC_SANDBOX_LOCAL_NETWORK:
      return network;
    case SWFDEC_SANDBOX_LOCAL_TRUSTED:
      return TRUE;
    case SWFDEC_SANDBOX_NONE:
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  if (swfdec_url_is_local (sandbox->url)) {
    sandbox->type = network ? SWFDEC_SANDBOX_LOCAL_NETWORK : SWFDEC_SANDBOX_LOCAL_FILE;
  } else {
    sandbox->type = SWFDEC_SANDBOX_REMOTE;
  }

  return TRUE;
}

/**
 * swfdec_sandbox_get_for_url:
 * @player: a #SwfdecPlayer
 * @url: the URL this player refers to
 * @flash_version: The Flash version for looking up the sandbox
 * @allow_network: %TRUE to allow network access, %FALSE to only allow local 
 *                 file access. See the documentation of the use_network flag 
 *                 of the SWF FileAttributes tag for what that means.
 *
 *
 * Checks if a sandbox is already in use for a given URL and if so, returns it.
 * Otherwise a new sandbox is created, initialized and returned.
 * Note that the given url must be a HTTP, HTTPS or a FILE url.
 *
 * Returns: the sandbox corresponding to the given URL or %NULL if no such 
 *          sandbox is allowed.
 **/
SwfdecSandbox *
swfdec_sandbox_get_for_url (SwfdecPlayer *player, const SwfdecURL *url,
    guint flash_version, gboolean allow_network)
{
  SwfdecPlayerPrivate *priv;
  SwfdecSandbox *sandbox;
  SwfdecURL *real;
  guint as_version;
  GSList *walk;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  priv = player->priv;
  real = swfdec_url_new_components (swfdec_url_get_protocol (url),
      swfdec_url_get_host (url), swfdec_url_get_port (url), NULL, NULL);
  as_version = flash_version < 7 ? 1 : 2;

  for (walk = priv->sandboxes; walk; walk = walk->next) {
    sandbox = walk->data;
    if (sandbox->as_version == as_version &&
	swfdec_url_equal (sandbox->url, real))
      break;
  }

  if (walk) {
    swfdec_url_free (real);

    if (!swfdec_sandbox_set_allow_network (sandbox, allow_network))
      return NULL;
  } else {
    SwfdecAsContext *context = SWFDEC_AS_CONTEXT (player);
    guint size = sizeof (SwfdecSandbox);
    if (!swfdec_as_context_use_mem (context, size))
      size = 0;

    sandbox = g_object_new (SWFDEC_TYPE_SANDBOX, NULL);
    swfdec_as_object_add (SWFDEC_AS_OBJECT (sandbox), context, size);
    sandbox->url = real;
    sandbox->as_version = as_version;
    priv->sandboxes = g_slist_append (priv->sandboxes, sandbox);
  
    if (!swfdec_sandbox_set_allow_network (sandbox, allow_network))
      return NULL;

    swfdec_sandbox_initialize (sandbox, flash_version);
  }


  return sandbox;
}

/**
 * swfdec_sandbox_use:
 * @sandbox: the sandbox to use when executing scripts
 *
 * Sets @sandbox to be used for scripts that are going to be executed next. No
 * sandbox may be set yet. You must unset the sandbox with 
 * swfdec_sandbox_unuse() after calling your script.
 **/
void
swfdec_sandbox_use (SwfdecSandbox *sandbox)
{
  SwfdecAsContext *context;
  SwfdecPlayerPrivate *priv;

  g_return_if_fail (SWFDEC_IS_SANDBOX (sandbox));
  g_return_if_fail (sandbox->type != SWFDEC_SANDBOX_NONE);
  g_return_if_fail (SWFDEC_AS_OBJECT (sandbox)->context->global == NULL);

  context = SWFDEC_AS_OBJECT (sandbox)->context;
  priv = SWFDEC_PLAYER (context)->priv;
  context->global = SWFDEC_AS_OBJECT (sandbox);

  context->Function = sandbox->Function;
  context->Function_prototype = sandbox->Function_prototype;
  context->Object = sandbox->Object;
  context->Object_prototype = sandbox->Object_prototype;
}

/**
 * swfdec_sandbox_try_use:
 * @sandbox: the sandbox to use
 *
 * Makes sure a sandbox is in use. If no sandbox is in use currently, use the
 * given @sandbox. This function is intended for cases where code can be called
 * from both inside scripts with a sandbox already set or outside with no 
 * sandbox in use.
 *
 * Returns: %TRUE if the new sandbox will be used. You need to call 
 *          swfdec_sandbox_unuse() afterwards. %FALSE if a sandbox is already in
 *          use.
 **/
gboolean
swfdec_sandbox_try_use (SwfdecSandbox *sandbox)
{
  g_return_val_if_fail (SWFDEC_IS_SANDBOX (sandbox), FALSE);
  g_return_val_if_fail (sandbox->type != SWFDEC_SANDBOX_NONE, FALSE);

  if (SWFDEC_AS_OBJECT (sandbox)->context->global)
    return FALSE;

  swfdec_sandbox_use (sandbox);
  return TRUE;
}

/**
 * swfdec_sandbox_unuse:
 * @sandbox: a #SwfdecSandbox
 *
 * Unsets the sandbox as the current sandbox for executing scripts.
 **/
void
swfdec_sandbox_unuse (SwfdecSandbox *sandbox)
{
  SwfdecAsContext *context;

  g_return_if_fail (SWFDEC_IS_SANDBOX (sandbox));
  g_return_if_fail (SWFDEC_AS_OBJECT (sandbox)->context->global == SWFDEC_AS_OBJECT (sandbox));

  context = SWFDEC_AS_OBJECT (sandbox)->context;
  context->global = NULL;
  context->Function = NULL;
  context->Function_prototype = NULL;
  context->Object = NULL;
  context->Object_prototype = NULL;
}

