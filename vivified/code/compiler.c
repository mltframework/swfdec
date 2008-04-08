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
#include "vivi_code_compiler.h"

int
main (int argc, char *argv[])
{
  SwfdecPlayer *player;
  char *target_name;
  FILE *source, *target;
  ViviCodeStatement *statement;
  //ViviCodePrinter *printer;
  ViviCodeCompiler *compiler;
  SwfdecBots *bots;
  SwfdecBuffer *buffer;
  unsigned char *length_ptr;
  SwfdecRect rect = { 0, 0, 2000, 3000 };

  player = swfdec_player_new (NULL);

  if (argc != 2) {
    g_printerr ("Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  source = fopen (argv[1], "r");
  if (source == NULL) {
    g_printerr ("Couldn't open source %s", argv[1]);
    return -1;
  }

  statement = vivi_compile_file (source, argv[1]);

  fclose (source);

  if (statement == NULL) {
    g_printerr ("Compilation failed\n");
    g_object_unref (player);
    return 1;
  }

  /*printer = vivi_code_text_printer_new ();
  vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (statement));
  g_object_unref (printer);*/


  bots = swfdec_bots_open ();


  // header

  // magic
  swfdec_bots_put_u8 (bots, 'F');
  swfdec_bots_put_u8 (bots, 'W');
  swfdec_bots_put_u8 (bots, 'S');
  // version
  swfdec_bots_put_u8 (bots, 8);
  // length
  swfdec_bots_put_u32 (bots, 0);
  // frame size
  swfdec_bots_put_rect (bots, &rect);
  // frame rate
  swfdec_bots_put_u16 (bots, 15 * 256);
  // frame count
  swfdec_bots_put_u16 (bots, 1);


  // tags

  // doaction tag

  compiler = vivi_code_compiler_new ();
  vivi_code_compiler_compile_token (compiler, VIVI_CODE_TOKEN (statement));
  buffer = vivi_code_compiler_get_data (compiler);
  g_object_unref (compiler);

  swfdec_bots_put_u16 (bots, GUINT16_TO_LE ((12 << 6) + 0x3F));
  swfdec_bots_put_u32 (bots, buffer->length + 1);
  swfdec_bots_put_buffer (bots, buffer);
  swfdec_buffer_unref (buffer);
  // end action
  swfdec_bots_put_u8 (bots, 0);

  // showframe tag
  swfdec_bots_put_u16 (bots, GUINT16_TO_LE (1 << 6));

  // end tag
  swfdec_bots_put_u16 (bots, 0);


  // write it

  buffer = swfdec_bots_close (bots);

  // fix length
  length_ptr = buffer->data + 4;
  *(guint32 *)length_ptr = GUINT32_TO_LE (buffer->length);

  target_name = g_strdup_printf ("%s.swf", argv[1]);
  target = fopen (target_name, "w+");
  g_free (target_name);

  if (fwrite (buffer->data, buffer->length, 1, target) != 1) {
    g_printerr ("Failed to write the botsput file\n");
    fclose (target);
    swfdec_buffer_unref (buffer);
    g_object_unref (player);
    return 1;
  }

  fclose (target);
  swfdec_buffer_unref (buffer);

  g_object_unref (player);

  return (statement == NULL ? -1 : 0);
}
