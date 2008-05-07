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
create_file (SwfdecBuffer *actions, guint version, guint rate, SwfdecRect rect)
{
  SwfdecBots *bots, *full;

  g_return_val_if_fail (actions != NULL, NULL);
  g_return_val_if_fail (version <= 255, NULL);
  g_return_val_if_fail (rate <= 255, NULL);

  bots = swfdec_bots_open ();

  // Frame size
  swfdec_bots_put_rect (bots, &rect);
  // Frame rate
  swfdec_bots_put_u16 (bots, rate * 256);
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
  swfdec_bots_put_u8 (full, version);
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
  int version = 8;
  int rate = 15;
  char *size_string = NULL;
  SwfdecRect size_rect = { 0, 0, 2000, 3000 };
  const char *output_filename = NULL;
  gboolean use_asm;
  GError *error = NULL;

  GOptionEntry options[] = {
    { "version", 'v', 0, G_OPTION_ARG_INT, &version, "target version", NULL },
    { "rate", 'r', 0, G_OPTION_ARG_INT, &rate, "the frame rate of the resulting Flash file", NULL },
    { "size", 's', 0, G_OPTION_ARG_STRING, &size_string, "the size give as WxH of the resulting Flash file", NULL },
    { "output", 'o', 0, G_OPTION_ARG_FILENAME, &output_filename, "output filename", NULL },
    { "asm", 'a', 0, G_OPTION_ARG_NONE, &use_asm, "output assembler instead of a Flash file", NULL },
    { NULL }
  };
  GOptionContext *ctx;

  ctx = g_option_context_new ("");
  g_option_context_add_main_entries (ctx, options, "options");
  g_option_context_parse (ctx, &argc, &argv, &error);
  g_option_context_free (ctx);

  if (error) {
    g_printerr ("Error parsing command line arguments: %s\n", error->message);
    g_error_free (error);
    return 1;
  }

  if (argc != 2) {
    g_printerr ("Usage: %s [OPTIONS] INFILE\n", argv[0]);
    return 1;
  }

  if (version < 0 || version > 255) {
    g_printerr ("Invalid version number\n");
    return 1;
  }

  if (rate < 0 || rate > 255) {
    g_printerr ("Invalid frame rate\n");
    return 1;
  }

  if (size_string != NULL) {
    char **parts, *end;
    guint64 width, height;

    parts = g_strsplit (size_string, "x", 2);
    if (parts[0] == NULL || parts[1] == NULL) {
      g_printerr ("Invalid size, use <width>x<height>\n");
      return 1;
    }

    width = g_ascii_strtoull (parts[0], &end, 10);
    if (*end != 0) {
      g_printerr ("Invalid size, use <width>x<height>\n");
      return 1;
    }

    height = g_ascii_strtoull (parts[1], &end, 10);
    if (*end != 0) {
      g_printerr ("Invalid size, use <width>x<height>\n");
      return 1;
    }

    // FIXME: size limit?

    size_rect.x1 = 20 * width;
    size_rect.y1 = 20 * height;
  }

  if (use_asm) {
    if (output_filename != NULL) {
      g_printerr ("Can't use output file when using -a\n");
      return 1;
    }
  } else {
    if (output_filename == NULL)
      output_filename = "out.swf";
  }

  swfdec_init ();

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
  g_object_unref (statement);
  vivi_code_assembler_pool (VIVI_CODE_ASSEMBLER (assembler));

  if (use_asm) {
    ViviCodePrinter *printer = vivi_code_text_printer_new ();
    vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (assembler));
    g_object_unref (printer);
    g_object_unref (assembler);
  } else {
    SwfdecScript *script = vivi_code_assembler_assemble_script (
	VIVI_CODE_ASSEMBLER (assembler), version, NULL);
    g_object_unref (assembler);

    if (script == NULL) {
      g_printerr ("Script assembling failed\n");
      return 1;
    }

    output = create_file (script->buffer, version, rate, size_rect);
    swfdec_script_unref (script);

    if (!g_file_set_contents (output_filename, (char *) output->data,
	  output->length, &error)) {
      swfdec_buffer_unref (output);
      g_printerr ("Error saving: %s\n", error->message);
      return 1;
    }
  }

  return 0;
}
