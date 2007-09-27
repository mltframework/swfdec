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

#include "swfdec_slow_loader.h"
#include <libswfdec-gtk/swfdec-gtk.h>

/*** SwfdecSlowLoader ***/

G_DEFINE_TYPE (SwfdecSlowLoader, swfdec_slow_loader, SWFDEC_TYPE_LOADER)

static void
swfdec_slow_loader_notify_cb (SwfdecLoader *child, GParamSpec *pspec, SwfdecLoader *loader)
{
  if (g_str_equal (pspec->name, "size")) {
    swfdec_loader_set_size (loader, swfdec_loader_get_size (child));
  }
}

static void
swfdec_slow_loader_dispose (GObject *object)
{
  SwfdecSlowLoader *slow = SWFDEC_SLOW_LOADER (object);

  g_signal_handlers_disconnect_by_func (slow->loader, swfdec_slow_loader_notify_cb, slow);
  g_object_unref (slow->loader);
  if (slow->timeout_id) {
    g_source_remove (slow->timeout_id);
    slow->timeout_id = 0;
  }

  G_OBJECT_CLASS (swfdec_slow_loader_parent_class)->dispose (object);
}

static gboolean
swfdec_slow_loader_tick (gpointer data)
{
  SwfdecSlowLoader *slow = data;
  SwfdecBuffer *buffer;
  guint total, amount;

  amount = swfdec_buffer_queue_get_depth (slow->loader->queue);
  if (amount > 0) {
    total = swfdec_buffer_queue_get_offset (slow->loader->queue);
    total += amount;
    total *= slow->tick_time;
    total += slow->duration - 1; /* rounding */
    amount = MIN (amount, total / slow->duration);
    buffer = swfdec_buffer_queue_pull (slow->loader->queue, amount);
#if 0
    g_print ("pushing %u bytes (%u/%u total)\n",
	amount, swfdec_buffer_queue_get_offset (slow->loader->queue),
	swfdec_buffer_queue_get_offset (slow->loader->queue) + 
	swfdec_buffer_queue_get_depth (slow->loader->queue));
#endif
    swfdec_loader_push (SWFDEC_LOADER (slow), buffer);
    if (swfdec_buffer_queue_get_depth (slow->loader->queue) > 0)
      return TRUE;
  }

  if (slow->loader->error) {
    swfdec_loader_error (SWFDEC_LOADER (slow), slow->loader->error);
    slow->timeout_id = 0;
    return FALSE;
  } else {
    gboolean eof;
    g_object_get (slow->loader, "eof", &eof, NULL);
    if (eof) {
      swfdec_loader_eof (SWFDEC_LOADER (slow));
      slow->timeout_id = 0;
      return FALSE;
    } else {
      return TRUE;
    }
  }
}

static void
swfdec_slow_loader_initialize (SwfdecSlowLoader *slow, SwfdecLoader *loader, guint duration)
{
  gulong size;

  slow->tick_time = 100;
  slow->duration = duration * 1000;
  slow->loader = loader;
  g_signal_connect (loader, "notify", G_CALLBACK (swfdec_slow_loader_notify_cb), slow);
  size = swfdec_loader_get_size (loader);
  if (size)
    swfdec_loader_set_size (SWFDEC_LOADER (slow), size);
  slow->timeout_id = g_timeout_add (slow->tick_time, swfdec_slow_loader_tick, slow);
  swfdec_loader_open (SWFDEC_LOADER (slow), 0);
}

static void
swfdec_slow_loader_load (SwfdecLoader *loader, SwfdecLoader *parent,
    SwfdecLoaderRequest request, const char *data, gsize data_len)
{
  SwfdecSlowLoader *slow = SWFDEC_SLOW_LOADER (loader);
  SwfdecLoader *new;

  /* FIXME: include request and data */
  new = swfdec_gtk_loader_new (swfdec_url_get_url (swfdec_loader_get_url (loader)));
  swfdec_slow_loader_initialize (slow, new, slow->duration / 1000);
}

static void
swfdec_slow_loader_class_init (SwfdecSlowLoaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecLoaderClass *loader_class = SWFDEC_LOADER_CLASS (klass);

  object_class->dispose = swfdec_slow_loader_dispose;

  loader_class->load = swfdec_slow_loader_load;
}

static void
swfdec_slow_loader_init (SwfdecSlowLoader *slow_loader)
{
}

SwfdecLoader *
swfdec_slow_loader_new (SwfdecLoader *loader, guint duration)
{
  SwfdecSlowLoader *ret;

  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);
  g_return_val_if_fail (duration > 0, NULL);

  ret = g_object_new (SWFDEC_TYPE_SLOW_LOADER, "url", loader->url, NULL);
  swfdec_slow_loader_initialize (ret, loader, duration);
  return SWFDEC_LOADER (ret);
}

