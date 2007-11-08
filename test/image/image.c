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

/* Compare two buffers, returning the number of pixels that are
 * different and the maximum difference of any single color channel in
 * result_ret.
 *
 * This function should be rewritten to compare all formats supported by
 * cairo_format_t instead of taking a mask as a parameter.
 */
static gboolean
buffer_diff_core (unsigned char *buf_a,
		  unsigned char *buf_b,
		  unsigned char *buf_diff,
		  int		width,
		  int		height,
		  int		stride)
{
    int x, y;
    gboolean result = TRUE;
    guint32 *row_a, *row_b, *row;

    for (y = 0; y < height; y++) {
	row_a = (guint32 *) (buf_a + y * stride);
	row_b = (guint32 *) (buf_b + y * stride);
	row = (guint32 *) (buf_diff + y * stride);
	for (x = 0; x < width; x++) {
	    /* check if the pixels are the same */
	    if (row_a[x] != row_b[x]) {
		int channel;
		static const unsigned int threshold = 3;
		guint32 diff_pixel = 0;

		/* calculate a difference value for all 4 channels */
		for (channel = 0; channel < 4; channel++) {
		    int value_a = (row_a[x] >> (channel*8)) & 0xff;
		    int value_b = (row_b[x] >> (channel*8)) & 0xff;
		    unsigned int diff;
		    diff = ABS (value_a - value_b);
		    if (diff <= threshold)
		      continue;
		    diff *= 4;  /* emphasize */
		    diff += 128; /* make sure it's visible */
		    if (diff > 255)
		        diff = 255;
		    diff_pixel |= diff << (channel*8);
		}

		row[x] = diff_pixel;
		if (diff_pixel)
		  result = FALSE;
	    } else {
		row[x] = 0;
	    }
	    row[x] |= 0xff000000; /* Set ALPHA to 100% (opaque) */
	}
    }
    return result;
}

static gboolean
image_diff (cairo_surface_t *surface, const char *filename)
{
  cairo_surface_t *image, *diff = NULL;
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
  diff = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, w, h);
  g_assert (cairo_image_surface_get_stride (surface) == 4 * w);
  g_assert (cairo_image_surface_get_stride (image) == 4 * w);
  g_assert (cairo_image_surface_get_stride (diff) == 4 * w);
  if (!buffer_diff_core (cairo_image_surface_get_data (surface), 
	cairo_image_surface_get_data (image), 
	cairo_image_surface_get_data (diff), 
	w, h, 4 * w) != 0) {
    g_print ("  ERROR: images differ\n");
    goto dump;
  }

  cairo_surface_destroy (image);
  cairo_surface_destroy (diff);
  return TRUE;

dump:
  cairo_surface_destroy (image);
  if (g_getenv ("SWFDEC_TEST_DUMP")) {
    cairo_status_t status;
    char *dump;
    
    dump = g_strdup_printf ("%s.dump.png", filename);
    status = cairo_surface_write_to_png (surface, dump);
    if (status) {
      g_print ("  ERROR: failed to dump image to %s: %s\n", dump,
	  cairo_status_to_string (status));
    }
    g_free (dump);
    if (diff) {
      dump = g_strdup_printf ("%s.diff.png", filename);
      status = cairo_surface_write_to_png (diff, dump);
      if (status) {
	g_print ("  ERROR: failed to dump diff image to %s: %s\n", dump,
	    cairo_status_to_string (status));
      }
      g_free (dump);
      cairo_surface_destroy (diff);
    }
  }
  return FALSE;
}

static gboolean
run_test (const char *filename)
{
  SwfdecLoader *loader;
  SwfdecPlayer *player = NULL;
  guint i, msecs;
  guint w, h;
  cairo_surface_t *surface;
  cairo_t *cr;

  g_print ("Testing %s:\n", filename);

  loader = swfdec_file_loader_new (filename);
  if (loader->error) {
    g_print ("  ERROR: %s\n", loader->error);
    g_object_unref (loader);
    goto error;
  }
  player = swfdec_player_new (NULL);
  swfdec_player_set_loader (player, loader);

  for (i = 0; i < 10; i++) {
    msecs = swfdec_player_get_next_event (player);
    swfdec_player_advance (player, msecs);
  }
  swfdec_player_get_default_size (player, &w, &h);
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
  g_print ("  OK\n");
  return TRUE;

error:
  if (player)
    g_object_unref (player);
  return FALSE;
}

int
main (int argc, char **argv)
{
  GList *failed_tests = NULL;

  swfdec_init ();

  if (argc > 1) {
    int i;
    for (i = 1; i < argc; i++) {
      if (!run_test (argv[i]))
	failed_tests = g_list_prepend (failed_tests, g_strdup (argv[i]));
    }
  } else {
    GDir *dir;
    char *name;
    const char *path, *file;
    /* automake defines this */
    path = g_getenv ("srcdir");
    if (path == NULL)
      path = ".";
    dir = g_dir_open (path, 0, NULL);
    while ((file = g_dir_read_name (dir))) {
      if (!g_str_has_suffix (file, ".swf"))
	continue;
      name = g_build_filename (path, file, NULL);
      if (!run_test (name)) {
	failed_tests = g_list_prepend (failed_tests, name);
      } else {
	g_free (name);
      }
    }
    g_dir_close (dir);
  }

  if (failed_tests) {
    GList *walk;
    failed_tests = g_list_sort (failed_tests, (GCompareFunc) strcmp);
    g_print ("\nFAILURES: %u\n", g_list_length (failed_tests));
    for (walk = failed_tests; walk; walk = walk->next) {
      g_print ("          %s\n", (char *) walk->data);
      g_free (walk->data);
    }
    g_list_free (failed_tests);
    return 1;
  } else {
    g_print ("\nEVERYTHING OK\n");
    return 0;
  }
}

