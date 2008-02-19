/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_source.h"

static glong
my_time_val_difference (const GTimeVal *compare, const GTimeVal *now)
{
  return (compare->tv_sec - now->tv_sec) * 1000 + 
    (compare->tv_usec - now->tv_usec) / 1000;
}

/*** SwfdecIterateSource ***/

typedef struct _SwfdecIterateSource SwfdecIterateSource;
struct _SwfdecIterateSource {
  GSource		source;
  SwfdecPlayer *	player;		/* player we manage or NULL if player was deleted */
  double		speed;		/* inverse playback speed (so 0.5 means double speed) */
  gulong		notify;		/* set for iterate notifications (only valid when player != NULL) */
  GTimeVal		last;		/* last time */
};

static glong
swfdec_iterate_get_msecs_to_next_event (GSource *source_)
{
  SwfdecIterateSource *source = (SwfdecIterateSource *) source_;
  GTimeVal now;
  glong diff;

  g_assert (source->player);
  diff = swfdec_player_get_next_event (source->player);
  if (diff == -1)
    return G_MAXLONG;
  diff *= source->speed;
  g_source_get_current_time (source_, &now);
  /* should really add to source->last instead of subtracting from now */
  g_time_val_add (&now, -diff * 1000);
  diff = my_time_val_difference (&source->last, &now);

  return diff;
}

static gboolean
swfdec_iterate_prepare (GSource *source, gint *timeout)
{
  glong diff;
  
  diff = swfdec_iterate_get_msecs_to_next_event (source);
  if (((SwfdecIterateSource *) source)->player == NULL) {
    *timeout = 0;
    return TRUE;
  }

  if (diff == G_MAXLONG) {
    *timeout = -1;
    return FALSE;
  } else if (diff <= 0) {
    *timeout = 0;
    return TRUE;
  } else {
    *timeout = diff;
    return FALSE;
  }
}

static gboolean
swfdec_iterate_check (GSource *source)
{
  glong diff;
  
  if (((SwfdecIterateSource *) source)->player == NULL)
    return 0;
  diff = swfdec_iterate_get_msecs_to_next_event (source);
  return diff < 0;
}

static gboolean
swfdec_iterate_dispatch (GSource *source_, GSourceFunc callback, gpointer user_data)
{
  SwfdecIterateSource *source = (SwfdecIterateSource *) source_;
  glong diff;
  
  if (source->player == NULL)
    return FALSE;
  diff = swfdec_iterate_get_msecs_to_next_event (source_);
  if (diff > 0)
    return TRUE;
  diff = swfdec_player_get_next_event (source->player) - diff;
  swfdec_player_advance (source->player, diff);
  return TRUE;
}

static void
swfdec_iterate_finalize (GSource *source_)
{
  SwfdecIterateSource *source = (SwfdecIterateSource *) source_;

  if (source->notify && source->player) {
    g_signal_handler_disconnect (source->player, source->notify);
  }
  if (source->player) {
    g_object_remove_weak_pointer (G_OBJECT (source->player), (gpointer) &source->player);
  }
}

GSourceFuncs swfdec_iterate_funcs = {
  swfdec_iterate_prepare,
  swfdec_iterate_check,
  swfdec_iterate_dispatch,
  swfdec_iterate_finalize
};

static void
swfdec_iterate_source_advance_cb (SwfdecPlayer *player, guint msecs, 
    guint audio_frames, SwfdecIterateSource *source)
{
  g_time_val_add (&source->last, msecs * 1000 * source->speed);
}

GSource *
swfdec_iterate_source_new (SwfdecPlayer *player, double speed)
{
  SwfdecIterateSource *source;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (speed > 0.0, NULL);

  source = (SwfdecIterateSource *) g_source_new (&swfdec_iterate_funcs, 
      sizeof (SwfdecIterateSource));
  source->player = player;
  g_object_add_weak_pointer (G_OBJECT (source->player), (gpointer) &source->player);
  source->speed = 1.0 / speed;
  source->notify = g_signal_connect (player, "advance",
      G_CALLBACK (swfdec_iterate_source_advance_cb), source);
  g_get_current_time (&source->last);
  
  return (GSource *) source;
}

void
swfdec_iterate_source_set_speed (GSource *source, double speed)
{
  /* FIXME: need a return_if_fail if wrong source? */
  g_return_if_fail (speed > 0.0);

  ((SwfdecIterateSource *) source)->speed = 1.0 / speed;
}

guint
swfdec_iterate_add (SwfdecPlayer *player)
{
  GSource *source;
  guint id;
  
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), 0);

  source = swfdec_iterate_source_new (player, 1.0);

  id = g_source_attach (source, NULL);
  g_source_unref (source);

  return id;
}
