/* Vivified
 * Copyright (C) 2008 Pekka Lampila <pekka.lampila@iki.fi>
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

#include <swfdec/swfdec.h>

#include "vivi_compiler.h"
#include "vivi_code_text_printer.h"

int
main (int argc, char *argv[])
{
  SwfdecPlayer *player;
  FILE *file;
  ViviCodeStatement *statement;

  player = swfdec_player_new (NULL);

  if (argc != 2) {
    g_printerr ("Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  file = fopen (argv[1], "r");
  if (file == NULL) {
    g_printerr ("Couldn't open file %s", argv[1]);
    return -1;
  }

  statement = vivi_compile_file (file, argv[1]);

  fclose (file);

  if (statement == NULL) {
    g_printerr ("Compilation failed\n");
  } else {
    ViviCodePrinter *printer = vivi_code_text_printer_new ();
    vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (statement));
    g_object_unref (printer);
  }

  g_object_unref (player);

  return (statement == NULL ? -1 : 0);
}
