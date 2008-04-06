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
  ViviCodePrinter *printer;
  ViviCodeCompiler *compiler;
  SwfdecOut *out;
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

  printer = vivi_code_text_printer_new ();
  vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (statement));
  g_object_unref (printer);


  out = swfdec_out_open ();


  // header

  // magic
  swfdec_out_put_u8 (out, 'F');
  swfdec_out_put_u8 (out, 'W');
  swfdec_out_put_u8 (out, 'S');
  // version
  swfdec_out_put_u8 (out, 8);
  // length
  swfdec_out_put_u32 (out, 0);
  // frame size
  swfdec_out_put_rect (out, &rect);
  // frame rate
  swfdec_out_put_u16 (out, 15 * 256);
  // frame count
  swfdec_out_put_u16 (out, 1);


  // tags

  // doaction tag

  compiler = vivi_code_compiler_new ();
  vivi_code_compiler_compile_token (compiler, VIVI_CODE_TOKEN (statement));
  buffer = vivi_code_compiler_get_data (compiler);
  g_object_unref (compiler);

  swfdec_out_put_u16 (out, GUINT16_TO_LE ((12 << 6) + 0x3F));
  swfdec_out_put_u32 (out, buffer->length + 1);
  swfdec_out_put_buffer (out, buffer);
  swfdec_buffer_unref (buffer);
  // end action
  swfdec_out_put_u8 (out, 0);

  // showframe tag
  swfdec_out_put_u16 (out, GUINT16_TO_LE (1 << 6));

  // end tag
  swfdec_out_put_u16 (out, 0);


  // write it

  buffer = swfdec_out_close (out);

  // fix length
  length_ptr = buffer->data + 4;
  *(guint32 *)length_ptr = GUINT32_TO_LE (buffer->length);

  target_name = g_strdup_printf ("%s.swf", argv[1]);
  target = fopen (target_name, "w+");
  g_free (target_name);

  if (fwrite (buffer->data, buffer->length, 1, target) != 1) {
    g_printerr ("Failed to write the output file\n");
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
