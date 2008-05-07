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
#include <swfdec/swfdec_bots.h>
#include <swfdec/swfdec_script_internal.h>
#include <swfdec/swfdec_tag.h>

#include "vivi_parser.h"
#include "vivi_code_text_printer.h"
#include "vivi_code_assembler.h"

static SwfdecBuffer *
create_file (SwfdecBuffer *actions)
{
  SwfdecBots *bots, *full;
  SwfdecRect rect = { 0, 0, 2000, 3000 };

  bots = swfdec_bots_open ();

  // Frame size
  swfdec_bots_put_rect (bots, &rect);
  // Frame rate
  swfdec_bots_put_u16 (bots, 15 * 256);
  // Frame count
  swfdec_bots_put_u16 (bots, 1);

  // DoAction
  swfdec_bots_put_u16 (bots,
      GUINT16_TO_LE ((SWFDEC_TAG_DOACTION << 6) + 0x3F));
  swfdec_bots_put_u32 (bots, actions->length);
  swfdec_bots_put_buffer (bots, actions);

  // ShowFrame
  swfdec_bots_put_u16 (bots, GUINT16_TO_LE (SWFDEC_TAG_SHOWFRAME << 6));

  // End
  swfdec_bots_put_u16 (bots, 0);

  // Header
  full = swfdec_bots_open ();
  swfdec_bots_put_u8 (full, 'F');
  swfdec_bots_put_u8 (full, 'W');
  swfdec_bots_put_u8 (full, 'S');
  swfdec_bots_put_u8 (full, 8);
  swfdec_bots_put_u32 (full, swfdec_bots_get_bytes (bots) + 8);
  swfdec_bots_put_bots (full, bots);
  swfdec_bots_free (bots);

  return swfdec_bots_close (full);
}

int
main (int argc, char *argv[])
{
  SwfdecBuffer *source, *output;
  ViviCodeStatement *statement, *assembler;
  SwfdecScript *script;
  GError *error = NULL;

  swfdec_init ();

  if (argc != 3) {
    g_printerr ("Usage: %s <input file> <output file>\n", argv[0]);
    return 1;
  }

  source = swfdec_buffer_new_from_file (argv[1], &error);
  if (source == NULL) {
    g_printerr ("Couldn't open: %s", error->message);
    g_error_free (error);
    return -1;
  }

  statement = vivi_parse_buffer (source);
  swfdec_buffer_unref (source);

  if (statement == NULL) {
    g_printerr ("Compilation failed\n");
    return 1;
  }

  assembler = vivi_code_assembler_new ();
  vivi_code_statement_compile (statement, VIVI_CODE_ASSEMBLER (assembler));
  script = vivi_code_assembler_assemble_script (
      VIVI_CODE_ASSEMBLER (assembler), 8, NULL);
  g_object_unref (assembler);
  g_object_unref (statement);

  if (script == NULL) {
    g_printerr ("Script assembling failed\n");
    return 1;
  }

  output = create_file (script->buffer);
  swfdec_script_unref (script);

  if (!g_file_set_contents (argv[2], (char *) output->data, output->length,
	&error)) {
    swfdec_buffer_unref (output);
    g_printerr ("Error saving: %s\n", error->message);
    return 1;
  }

  return 0;
}
