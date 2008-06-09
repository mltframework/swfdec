/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_loader_internal.h"
#include "swfdec_buffer.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"

/**
 * SwfdecFileLoader:
 *
 * This is a #SwfdecLoader that can load content from files. This symbol is
 * exported so you can subclass your own loaders from it and have automatic
 * file access.
 */

G_DEFINE_TYPE (SwfdecFileLoader, swfdec_file_loader, SWFDEC_TYPE_LOADER)

static void
swfdec_file_loader_load (SwfdecLoader *loader, SwfdecPlayer *player,
    const char *url_string, SwfdecBuffer *buffer, guint header_count,
    const char **header_names, const char **header_values)
{
  SwfdecStream *stream = SWFDEC_STREAM (loader);
  GError *error = NULL;
  char *real;
  SwfdecURL *url;

  if (swfdec_url_path_is_relative (url_string)) {
    url = swfdec_url_new_relative (swfdec_player_get_base_url (player), url_string);
  } else {
    url = swfdec_url_new (url_string);
  }
  if (url == NULL) {
    swfdec_stream_error (stream, "%s is an invalid URL", url_string);
    return;
  }
  swfdec_loader_set_url (loader, swfdec_url_get_url (url));
  if (!g_str_equal (swfdec_url_get_protocol (url), "file")) {
    swfdec_stream_error (stream, "Don't know how to handle this protocol");
    swfdec_url_free (url);
    return;
  }
  if (swfdec_url_get_host (url)) {
    swfdec_stream_error (stream, "filenames cannot have hostnames");
    swfdec_url_free (url);
    return;
  }

  /* FIXME: append query string here? */
  real = g_strconcat ("/", swfdec_url_get_path (url), NULL);
  buffer = swfdec_buffer_new_from_file (real, &error);
  g_free (real);
  if (buffer == NULL) {
    swfdec_stream_error (stream, "%s", error->message);
    g_error_free (error);
  } else {
    swfdec_loader_set_size (loader, buffer->length);
    swfdec_stream_open (stream);
    swfdec_stream_push (stream, buffer);
    swfdec_stream_eof (stream);
  }
  swfdec_url_free (url);
}

static void
swfdec_file_loader_class_init (SwfdecFileLoaderClass *klass)
{
  SwfdecLoaderClass *loader_class = SWFDEC_LOADER_CLASS (klass);

  loader_class->load = swfdec_file_loader_load;
}

static void
swfdec_file_loader_init (SwfdecFileLoader *loader)
{
}

