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

#include <stdlib.h>
#include <string.h>

#include "swfdec_interval.h"
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_function.h"
#include "swfdec_as_internal.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"

G_DEFINE_TYPE (SwfdecInterval, swfdec_interval, SWFDEC_TYPE_GC_OBJECT)

static void
swfdec_interval_mark (SwfdecGcObject *object)
{
  guint i;
  SwfdecInterval *interval = SWFDEC_INTERVAL (object);

  swfdec_gc_object_mark (interval->sandbox);
  if (interval->fun_name)
    swfdec_as_string_mark (interval->fun_name);
  for (i = 0; i < interval->n_args + 1; i++) {
    swfdec_as_value_mark (&interval->args[i]);
  }

  SWFDEC_GC_OBJECT_CLASS (swfdec_interval_parent_class)->mark (object);
}

static void
swfdec_interval_dispose (GObject *object)
{
  SwfdecInterval *interval = SWFDEC_INTERVAL (object);
  SwfdecAsContext *cx = swfdec_gc_object_get_context (interval);

  if (interval->args) {
    swfdec_as_context_unuse_mem (cx,
	(interval->n_args + 1) * sizeof (SwfdecAsValue));
    g_slice_free1 ((interval->n_args + 1) * sizeof (SwfdecAsValue), interval->args);
    interval->args = NULL;
    interval->n_args = 0;
  }
  /* needed here when GC'ed by closing the player */
  if (interval->timeout.callback != NULL) {
    swfdec_player_remove_timeout (SWFDEC_PLAYER (cx), &interval->timeout);
    interval->timeout.callback = NULL;
  }

  G_OBJECT_CLASS (swfdec_interval_parent_class)->dispose (object);
}

static void
swfdec_interval_class_init (SwfdecIntervalClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_interval_dispose;

  gc_class->mark = swfdec_interval_mark;
}

static void
swfdec_interval_init (SwfdecInterval *array)
{
}

static void
swfdec_interval_trigger (SwfdecTimeout *timeout)
{
  SwfdecAsValue ret;
  SwfdecInterval *interval = SWFDEC_INTERVAL ((void *) (((guchar *) timeout) 
      - G_STRUCT_OFFSET (SwfdecInterval, timeout)));
  SwfdecAsContext *context = swfdec_gc_object_get_context (interval);
  SwfdecPlayer *player = SWFDEC_PLAYER (context);

  if (interval->repeat) {
    timeout->timestamp += SWFDEC_MSECS_TO_TICKS (interval->msecs);
    swfdec_player_add_timeout (player, timeout);
  } else {
    player->priv->intervals = g_list_remove (player->priv->intervals, interval);
    interval->timeout.callback = NULL;
  }
  swfdec_sandbox_use (interval->sandbox);
  if (interval->fun_name) {
    SwfdecAsObject *object = swfdec_as_value_to_object (context, interval->args[0]);
    if (object) {
      swfdec_as_object_call (object,
	  interval->fun_name, interval->n_args, interval->args, &ret);
    }
  } else {
    /* we check that the relay's type is correct upon adding the interval */
    swfdec_as_function_call (SWFDEC_AS_FUNCTION (
	  SWFDEC_AS_VALUE_GET_OBJECT (interval->args[0])->relay),
	NULL, interval->n_args, &interval->args[1], &ret);
  }
  swfdec_sandbox_unuse (interval->sandbox);
}

static guint
swfdec_interval_new (SwfdecPlayer *player, guint msecs, gboolean repeat, 
    const SwfdecAsValue *src, const char *fun_name,
    guint n_args, const SwfdecAsValue *args)
{
  SwfdecAsContext *context;
  SwfdecInterval *interval;

  context = SWFDEC_AS_CONTEXT (player);
  if (!swfdec_as_context_try_use_mem (context, (n_args + 1) * sizeof (SwfdecAsValue))) {
    swfdec_as_context_abort (context,
	"Too many arguments passed to setInterval/setTimeout");
    return 0;
  }
  interval = g_object_new (SWFDEC_TYPE_INTERVAL, "context", context, NULL);

  interval->id = ++player->priv->interval_id;
  interval->sandbox = swfdec_sandbox_get (player);
  interval->msecs = msecs;
  interval->repeat = repeat;
  interval->fun_name = fun_name;
  interval->n_args = n_args;
  interval->args = g_slice_alloc ((n_args + 1) * sizeof (SwfdecAsValue));
  interval->args[0] = *src;
  if (n_args)
    memcpy (&interval->args[1], args, n_args * sizeof (SwfdecAsValue));
  interval->timeout.timestamp = player->priv->time + SWFDEC_MSECS_TO_TICKS (interval->msecs);
  interval->timeout.callback = swfdec_interval_trigger;
  swfdec_player_add_timeout (player, &interval->timeout);

  player->priv->intervals = 
    g_list_prepend (player->priv->intervals, interval);

  return interval->id;
}

guint
swfdec_interval_new_function (SwfdecPlayer *player, guint msecs, gboolean repeat,
    SwfdecAsFunction *fun, guint n_args, const SwfdecAsValue *args)
{
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), 0);
  g_return_val_if_fail (msecs > 0, 0);
  g_return_val_if_fail (SWFDEC_IS_AS_FUNCTION (fun), 0);
  g_return_val_if_fail (n_args == 0 || args != NULL, 0);

  SWFDEC_AS_VALUE_SET_OBJECT (&val,
      swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (fun)));
  return swfdec_interval_new (player, msecs, repeat, &val, NULL, n_args, args);
}

guint
swfdec_interval_new_object (SwfdecPlayer *player, guint msecs, gboolean repeat,
    const SwfdecAsValue *thisp, const char *fun_name,
    guint n_args, const SwfdecAsValue *args)
{
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), 0);
  g_return_val_if_fail (msecs > 0, 0);
  g_return_val_if_fail (thisp != NULL, 0);
  g_return_val_if_fail (fun_name != NULL, 0);
  g_return_val_if_fail (n_args == 0 || args != NULL, 0);

  return swfdec_interval_new (player, msecs, repeat, thisp, fun_name, n_args, args);
}

void
swfdec_interval_remove (SwfdecPlayer *player, guint id)
{
  GList *walk;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));

  for (walk = player->priv->intervals; walk; walk = walk->next) {
    SwfdecInterval *interval = walk->data;
    if (interval->id != id)
      continue;

    player->priv->intervals = g_list_delete_link (player->priv->intervals, walk);
    swfdec_player_remove_timeout (player, &interval->timeout);
    interval->timeout.callback = NULL;
    return;
  }
}

