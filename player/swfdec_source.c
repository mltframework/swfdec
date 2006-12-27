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

/*** SwfdecTime ***/

static void
swfdec_time_compute_next (SwfdecTime *time)
{
  /* must be 64bit to avoid overflow */
  glong add = (gint64) time->iterated * 256 * G_USEC_PER_SEC / time->rate;

  time->next = time->start;
  g_time_val_add (&time->next, add);
  if (time->iterated == time->rate) {
    time->start = time->next;
    time->iterated = 0;
  }
}

void
swfdec_time_init (SwfdecTime *time, GTimeVal *now, double rate)
{
  g_return_if_fail (time != NULL);
  g_return_if_fail (now != NULL);
  g_return_if_fail (rate > 0);

  time->start = *now;
  time->iterated = 1;
  time->rate = rate * 256;
  swfdec_time_compute_next (time);
}

/**
 * swfdec_time_tick:
 * @time: a @SwfdecTime
 *
 * Advances @time by one tick.
 **/
void
swfdec_time_tick (SwfdecTime *time)
{
  g_return_if_fail (time != NULL);

  time->iterated++;
  swfdec_time_compute_next (time);
}

/**
 * swfdec_time_get_difference:
 * @time: #SwfdecTime to compare to
 * @tv: a timeval to check
 *
 * Compares the given @time to @tv. If the result is positive, @time happens
 * before @tv, if it's negative, it happens after.
 *
 * Returns: The difference in milliseconds between @time and @tv
 **/
glong
swfdec_time_get_difference (SwfdecTime *time, GTimeVal *tv)
{
  g_return_val_if_fail (time != NULL, 0);
  g_return_val_if_fail (tv != NULL, 0);

  return (tv->tv_sec - time->next.tv_sec) * 1000 + 
    (tv->tv_usec - time->next.tv_usec) / 1000;
}

/*** SwfdecIterateSource ***/

typedef struct _SwfdecIterateSource SwfdecIterateSource;
struct _SwfdecIterateSource {
  GSource		source;
  SwfdecPlayer *	player;
  double		speed;		/* playback speed */
  gulong		notify;		/* set for iterate notifications */
  SwfdecTime		time;		/* time manager */
};

static gboolean
swfdec_iterate_prepare (GSource *source_, gint *timeout)
{
  SwfdecIterateSource *source = (SwfdecIterateSource *) source_;
  GTimeVal now;
  glong diff;

  g_source_get_current_time (source_, &now);
  diff = swfdec_time_get_difference (&source->time, &now);
  if (diff >= 0) {
    *timeout = 0;
    return TRUE;
  } else {
    *timeout = -diff;
    return FALSE;
  }
}

static gboolean
swfdec_iterate_check (GSource *source)
{
  gint timeout;

  return swfdec_iterate_prepare (source, &timeout);
}

static gboolean
swfdec_iterate_dispatch (GSource *source_, GSourceFunc callback, gpointer user_data)
{
  SwfdecIterateSource *source = (SwfdecIterateSource *) source_;

  swfdec_player_iterate (source->player);
  swfdec_time_tick (&source->time);
  return TRUE;
}

static void
swfdec_iterate_finalize (GSource *source_)
{
  SwfdecIterateSource *source = (SwfdecIterateSource *) source_;

  if (source->notify) {
    g_signal_handler_disconnect (source->player, source->notify);
  }
  g_object_unref (source->player);
}

GSourceFuncs swfdec_iterate_funcs = {
  swfdec_iterate_prepare,
  swfdec_iterate_check,
  swfdec_iterate_dispatch,
  swfdec_iterate_finalize
};

static gboolean
swfdec_iterate_source_handle_mouse (SwfdecPlayer *player, double x, double y, 
    int button, SwfdecIterateSource *source)
{
  guint samples = swfdec_player_get_audio_samples (player);
  glong delay;
  GTimeVal now;

  g_get_current_time (&now);
  delay = swfdec_time_get_difference (&source->time, &now);

  delay = samples + delay * 44100 / 1000;
  if (delay < 0)
    samples = 0;
  else
    samples = MIN ((guint) delay, samples);
  swfdec_player_set_audio_advance (player, samples);

  return FALSE;
}

static void
swfdec_iterate_source_notify_cb (SwfdecPlayer *player, GParamSpec *pspec,
    SwfdecIterateSource *source)
{
  GTimeVal now;
  double rate = swfdec_player_get_rate (player);
  g_assert (rate > 0);
  g_get_current_time (&now);
  rate *= source->speed;
  swfdec_time_init (&source->time, &now, rate);
  g_signal_handler_disconnect (player, source->notify);
  source->notify = 0;
}

GSource *
swfdec_iterate_source_new (SwfdecPlayer *player, double speed)
{
  SwfdecIterateSource *source;
  GTimeVal now;
  double rate;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);
  g_return_val_if_fail (speed > 0.0, NULL);

  source = (SwfdecIterateSource *) g_source_new (&swfdec_iterate_funcs, 
      sizeof (SwfdecIterateSource));
  source->player = g_object_ref (player);
  source->speed = speed;
  g_signal_connect (player, "handle-mouse", 
      G_CALLBACK (swfdec_iterate_source_handle_mouse), source);
  rate = swfdec_player_get_rate (player);
  if (rate > 0) {
    g_get_current_time (&now);
    rate *= speed;
    swfdec_time_init (&source->time, &now, rate);
  } else {
    source->notify = g_signal_connect (player, "notify::initialized",
	G_CALLBACK (swfdec_iterate_source_notify_cb), source);
  }
  
  return (GSource *) source;
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
