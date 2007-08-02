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
#include "swfdec_loadertarget.h"
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
swfdec_file_loader_load (SwfdecLoader *loader, SwfdecLoaderRequest request, 
    const char *data, gsize data_len)
{
  const SwfdecURL *url;
  SwfdecBuffer *buffer;
  GError *error = NULL;
  char *real;

  url = swfdec_loader_get_url (loader);
  if (!g_str_equal (swfdec_url_get_protocol (url), "file")) {
    swfdec_loader_error (loader, "Don't know how to handle this protocol");
    return;
  }
  if (swfdec_url_get_host (url)) {
    swfdec_loader_error (loader, "filenames cannot have hostnames");
    return;
  }

  /* FIXME: append query string here? */
  real = g_strconcat ("/", swfdec_url_get_path (url), NULL);
  buffer = swfdec_buffer_new_from_file (real, &error);
  if (buffer == NULL) {
    swfdec_loader_error (loader, error->message);
    g_error_free (error);
  } else {
    swfdec_loader_set_size (loader, buffer->length);
    swfdec_loader_open (loader, 0);
    swfdec_loader_push (loader, buffer);
    swfdec_loader_eof (loader);
  }
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

/**
 * swfdec_file_loader_new:
 * @filename: name of the file to load
 *
 * Creates a new loader for local files. If an error occurred, the loader will
 * be in error.
 *
 * Returns: a new loader
 **/
SwfdecLoader *
swfdec_file_loader_new (const char *filename)
{
  SwfdecBuffer *buf;
  SwfdecLoader *loader;
  GError *error = NULL;
  char *url_string;
  SwfdecURL *url;

  g_return_val_if_fail (filename != NULL, NULL);

  buf = swfdec_buffer_new_from_file (filename, &error);

  if (g_path_is_absolute (filename)) {
    url_string = g_strconcat ("file://", filename, NULL);
  } else {
    char *abs, *cur;
    cur = g_get_current_dir ();
    abs = g_build_filename (cur, filename, NULL);
    g_free (cur);
    url_string = g_strconcat ("file://", abs, NULL);
    g_free (abs);
  }

  url = swfdec_url_new (url_string);
  g_free (url_string);
  loader = g_object_new (SWFDEC_TYPE_FILE_LOADER, "url", url, NULL);
  swfdec_url_free (url);
  if (buf == NULL) {
    swfdec_loader_error (loader, error->message);
    g_error_free (error);
  } else {
    swfdec_loader_set_size (loader, buf->length);
    swfdec_loader_open (loader, 0);
    swfdec_loader_push (loader, buf);
    swfdec_loader_eof (loader);
  }
  return loader;
}

