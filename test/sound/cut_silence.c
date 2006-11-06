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
#include <string.h>
#include <glib.h>

static void
usage (const char *app)
{
  g_print ("usage : %s INFILE OUTFILE\n\n", app);
}

int
main (int argc, char **argv)
{
  char *data;
  gsize length;
  GError *error = NULL;

  if (argc != 3) {
    usage (argv[0]);
    return 0;
  }

  if (!g_file_get_contents (argv[1], &data, &length, &error)) {
    g_printerr ("Error: %s\n", error->message);
    g_error_free (error);
    return 1;
  }
  while (length > 3) {
    if (data[length - 1] != 0 ||
	data[length - 2] != 0 ||
	data[length - 3] != 0 ||
	data[length - 4] != 0)
      break;
    length -= 4;
  }
  if (!g_file_set_contents (argv[2], data, length, &error)) {
    g_printerr ("Error: %s\n", error->message);
    g_error_free (error);
    g_free (data);
    return 1;
  }
  g_free (data);
  return 0;
}

