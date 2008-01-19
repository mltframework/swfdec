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

G_DEFINE_TYPE (SwfdecSandbox, swfdec_sandbox, G_TYPE_OBJECT)

static void
swfdec_sandbox_class_init (SwfdecSandboxClass *klass)
{
}

static void
swfdec_sandbox_init (SwfdecSandbox *sandbox)
{
  sandbox->type = SWFDEC_SANDBOX_NONE;
}

/**
 * swfdec_sandbox_get_for_url:
 * @player: a #SwfdecPlayer
 * @url: the URL this player refers to
 *
 * Checks if a sandbox is already in use for a given URL and if so, returns it.
 * Otherwise a new sandbox is created, initialized and returned.
 * Note that the given url must be a HTTP, HTTPS or a FILE url.
 *
 * Returns: the sandbox corresponding to the given URL.
 **/
SwfdecSandbox *
swfdec_sandbox_get_for_url (SwfdecPlayer *player, const SwfdecURL *url)
{
  SwfdecPlayerPrivate *priv;
  SwfdecSandbox *sandbox;
  SwfdecURL *real;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  priv = player->priv;
  real = swfdec_url_new_components (swfdec_url_get_protocol (url),
      swfdec_url_get_host (url), swfdec_url_get_port (url), NULL, NULL);

  sandbox = g_hash_table_lookup (priv->sandboxes, real);
  if (sandbox) {
    swfdec_url_free (real);
  } else {
    SwfdecAsContext *context = SWFDEC_AS_CONTEXT (player);
    guint size = sizeof (SwfdecSandbox);
    if (!swfdec_as_context_use_mem (context, size))
      size = 0;

    sandbox = g_object_new (SWFDEC_TYPE_SANDBOX, NULL);
    swfdec_as_object_add (SWFDEC_AS_OBJECT (sandbox), context, size);
    sandbox->url = real;
  }
  return sandbox;
}

static void
swfdec_sandbox_initialize (SwfdecSandbox *sandbox)
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
      sizeof (swfdec_initialize), 8);

  if (context->state == SWFDEC_AS_CONTEXT_NEW)
    context->state = SWFDEC_AS_CONTEXT_RUNNING;
  swfdec_sandbox_unuse (sandbox);
}
/**
 * swfdec_sandbox_set_allow_network:
 * @sandbox: a #SwfdecSandbox
 * @network: %TRUE to allow network access, %FALSE to only allow local file 
 *           access. See the documentation of the use_network flag of the SWF
 *           FileAttributes tag for what that means.
 *
 * This function finishes initialization of the @sandbox, if it is not yet 
 * finished, by giving the sandbox network or local file access. This function
 * should be called on all return values of swfdec_sandbox_get_for_url().
 *
 * Returns: %TRUE if the sandbox initialization could be finished as requested,
 *          %FALSE if not and it shouldn't be used.
 **/
gboolean
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

  swfdec_sandbox_initialize (sandbox);

  return TRUE;
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
  g_return_if_fail (SWFDEC_IS_SANDBOX (sandbox));
  g_return_if_fail (sandbox->type == SWFDEC_SANDBOX_NONE);
  g_return_if_fail (SWFDEC_AS_OBJECT (sandbox)->context->global == NULL);

  SWFDEC_AS_OBJECT (sandbox)->context->global = SWFDEC_AS_OBJECT (sandbox);
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
  g_return_if_fail (SWFDEC_IS_SANDBOX (sandbox));
  g_return_if_fail (sandbox->type == SWFDEC_SANDBOX_NONE);
  g_return_if_fail (SWFDEC_AS_OBJECT (sandbox)->context->global == SWFDEC_AS_OBJECT (sandbox));

  SWFDEC_AS_OBJECT (sandbox)->context->global = NULL;
}

