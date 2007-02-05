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
#include <string.h>
#include <libswfdec/swfdec.h>

static gboolean
image_diff (cairo_surface_t *surface, const char *filename)
{
  cairo_surface_t *image;
  int w, h;
  char *real;

  real = g_strdup_printf ("%s.png", filename);
  image = cairo_image_surface_create_from_png (real);
  if (cairo_surface_status (image)) {
    g_print ("  ERROR: Could not load %s: %s\n", real,
	cairo_status_to_string (cairo_surface_status (image)));
    g_free (real);
    goto dump;
  }
  g_free (real);
  g_assert (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE);
  w = cairo_image_surface_get_width (surface);
  h = cairo_image_surface_get_height (surface);
  if (w != cairo_image_surface_get_width (image) ||
      h != cairo_image_surface_get_height (image)) {
    g_print ("  ERROR: sizes don't match. Should be %ux%u, but is %ux%u\n",
	cairo_image_surface_get_width (image), cairo_image_surface_get_height (image),
	w, h);
    goto dump;
  }
  g_assert (cairo_image_surface_get_stride (surface) == 4 * w);
  g_assert (cairo_image_surface_get_stride (image) == 4 * w);
  if (memcmp (cairo_image_surface_get_data (surface), 
	cairo_image_surface_get_data (image), 4 * w * h) != 0) {
    g_print ("  ERROR: images differ\n");
    goto dump;
  }

  cairo_surface_destroy (image);
  return TRUE;

dump:
  cairo_surface_destroy (image);
  if (g_getenv ("SWFDEC_TEST_DUMP")) {
    cairo_status_t status;
    char *dump = g_strdup_printf ("%s.dump.png", filename);
    status = cairo_surface_write_to_png (surface, dump);
    if (status) {
      g_print ("  ERROR: failed to dump image to %s: %s\n", dump,
	  cairo_status_to_string (status));
    }
    g_free (dump);
  }
  return FALSE;
}

static gboolean
run_test (const char *filename)
{
  SwfdecLoader *loader;
  SwfdecPlayer *player = NULL;
  guint i, msecs;
  GError *error = NULL;
  int w, h;
  cairo_surface_t *surface;
  cairo_t *cr;

  g_print ("Testing %s:\n", filename);

  loader = swfdec_loader_new_from_file (filename, &error);
  if (loader == NULL) {
    g_print ("  ERROR: %s\n", error->message);
    goto error;
  }
  player = swfdec_player_new ();
  swfdec_player_set_loader (player, loader);

  for (i = 0; i < 10; i++) {
    msecs = swfdec_player_get_next_event (player);
    swfdec_player_advance (player, msecs);
  }
  swfdec_player_get_image_size (player, &w, &h);
  if (w == 0 || h == 0) {
    g_print ("  ERROR: width and height not set\n");
    goto error;
  }
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
  cr = cairo_create (surface);
  swfdec_player_render (player, cr, 0, 0, w, h); 
  cairo_destroy (cr);
  if (!image_diff (surface, filename)) {
    cairo_surface_destroy (surface);
    goto error;
  }
  cairo_surface_destroy (surface);
  g_object_unref (player);
  return TRUE;

error:
  if (error)
    g_error_free (error);
  if (player)
    g_object_unref (player);
  return FALSE;
}

int
main (int argc, char **argv)
{
  guint failed_tests = 0;

  swfdec_init ();

  if (argc > 1) {
    int i;
    for (i = 1; i < argc; i++) {
      if (!run_test (argv[i]))
	failed_tests++;
    }
  } else {
    GDir *dir;
    const char *file;
    dir = g_dir_open (".", 0, NULL);
    while ((file = g_dir_read_name (dir))) {
      if (!g_str_has_suffix (file, ".swf"))
	continue;
      if (!run_test (file))
	failed_tests++;
    }
    g_dir_close (dir);
  }

  if (failed_tests) {
    g_print ("\nFAILURES: %u\n", failed_tests);
  } else {
    g_print ("\nEVERYTHING OK\n");
  }
  return failed_tests;
}

