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
#include <swfdec/swfdec.h>
#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_bots.h>
#include <swfdec/swfdec_script_internal.h>
#include <swfdec/swfdec_tag.h>

#include <vivified/code/vivi_code_assembler.h>
#include <vivified/code/vivi_decompiler.h>

static SwfdecBuffer *
do_script (SwfdecBuffer *buffer, guint version)
{
  ViviCodeAssembler *assembler;
  SwfdecScript *script;
  GError *error = NULL;

  script = swfdec_script_new (buffer, "script", version);

  assembler = VIVI_CODE_ASSEMBLER (vivi_disassemble_script (script));
  swfdec_script_unref (script);

  /* FIXME: modify script here! */

  script = vivi_code_assembler_assemble_script (assembler, version, &error);
  g_object_unref (assembler);
  if (script == NULL) {
    g_print ("error: %s\n", error->message);
    g_error_free (error);
    return NULL;
  }

  /* FIXME: We want swfdec_script_get_buffer() */
  g_assert (script->main == script->buffer->data);
  buffer = swfdec_buffer_ref (script->buffer);
  swfdec_script_unref (script);

  return buffer;
}

static SwfdecBuffer *
process_buffer (SwfdecBuffer *original)
{
  SwfdecRect rect;
  SwfdecBits bits;
  SwfdecBots *bots, *full;
  SwfdecBuffer *buffer;
  guint version, tag, len;
  gboolean long_header;

  bots = swfdec_bots_open ();
  swfdec_bits_init (&bits, original);

  /* copy header */
  swfdec_bits_get_u8 (&bits);
  swfdec_bits_get_u8 (&bits);
  swfdec_bits_get_u8 (&bits);
  version = swfdec_bits_get_u8 (&bits);
  swfdec_bits_get_u32 (&bits);
  swfdec_bits_get_rect (&bits, &rect);
  swfdec_bots_put_rect (bots, &rect);
  swfdec_bots_put_u16 (bots, swfdec_bits_get_u16 (&bits));
  swfdec_bots_put_u16 (bots, swfdec_bits_get_u16 (&bits));

  while (swfdec_bits_left (&bits)) {
    len = swfdec_bits_get_u16 (&bits);
    tag = (len >> 6);
    len = len & 0x3f;
    long_header = (len == 0x3f);
    if (long_header)
      len = swfdec_bits_get_u32 (&bits);

    buffer = swfdec_bits_get_buffer (&bits, len);
    if (buffer == NULL) {
      swfdec_bots_free (bots);
      swfdec_buffer_unref (original);
      return NULL;
    }

    switch (tag) {
      case SWFDEC_TAG_DOINITACTION:
	{
	  SwfdecBuffer *sub = swfdec_buffer_new_subbuffer (buffer, 2, buffer->length - 2);
	  SwfdecBots *bots2 = swfdec_bots_open ();
	  swfdec_bots_put_u16 (bots2, buffer->data[0] | buffer->data[1] << 8);
	  sub = do_script (sub, version);
	  if (sub == NULL) {
	    swfdec_bots_free (bots2);
	    swfdec_bots_free (bots);
	    swfdec_buffer_unref (original);
	    return NULL;
	  }
	  swfdec_bots_put_buffer (bots2, sub);
	  swfdec_buffer_unref (sub);
	  swfdec_buffer_unref (buffer);
	  buffer = swfdec_bots_close (bots2);
	}
	break;
      case SWFDEC_TAG_DOACTION:
	buffer = do_script (buffer, version);
	if (buffer == NULL) {
	  swfdec_bots_free (bots);
	  swfdec_buffer_unref (original);
	  return NULL;
	}
	break;
      default:
	break;
    }
    long_header |= buffer->length >= 0x3f;
    swfdec_bots_put_u16 (bots, tag << 6 | (long_header ? 0x3f : buffer->length));
    if (long_header)
      swfdec_bots_put_u32 (bots, buffer->length);
    swfdec_bots_put_buffer (bots, buffer);
    swfdec_buffer_unref (buffer);
  }

  swfdec_buffer_unref (original);
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

static SwfdecBuffer *
buffer_decode (SwfdecBuffer *buffer)
{
  SwfdecBits bits;
  SwfdecBots *bots;
  SwfdecBuffer *decoded;
  guint u;

  swfdec_bits_init (&bits, buffer);
  u = swfdec_bits_get_u8 (&bits);
  if ((u != 'C' && u != 'F') ||
      swfdec_bits_get_u8 (&bits) != 'W' ||
      swfdec_bits_get_u8 (&bits) != 'S') {
    swfdec_buffer_unref (buffer);
    return NULL;
  }
  
  if (u == 'F')
    return buffer;

  bots = swfdec_bots_open ();
  swfdec_bots_put_u8 (bots, 'F');
  swfdec_bots_put_u8 (bots, 'W');
  swfdec_bots_put_u8 (bots, 'S');
  swfdec_bots_put_u8 (bots, swfdec_bits_get_u8 (&bits));
  u = swfdec_bits_get_u32 (&bits);
  g_assert (u <= G_MAXINT32);
  decoded = swfdec_bits_decompress (&bits, -1, u);
  swfdec_buffer_unref (buffer);
  if (decoded == NULL)
    return NULL;
  swfdec_bots_put_u32 (bots, u);
  swfdec_bots_put_buffer (bots, decoded);
  swfdec_buffer_unref (decoded);
  buffer = swfdec_bots_close (bots);
  return buffer;
}

int 
main (int argc, char *argv[])
{
  GError *error = NULL;
  SwfdecBuffer *buffer;

  GOptionEntry options[] = {
    //{ "asm", 'a', 0, G_OPTION_ARG_NONE, &use_asm, "output assembler instead of decompiled script", NULL },
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

  if (argc < 3) {
    g_printerr ("%s [OPTIONS] INFILE OUTFILE\n", argv[0]);
    return 1;
  }

  swfdec_init ();

  buffer = swfdec_buffer_new_from_file (argv[1], &error);
  if (buffer == NULL) {
    g_printerr ("Error loading: %s\n", error->message);
    g_error_free (error);
    return 1;
  }
  buffer = buffer_decode (buffer);
  if (buffer == NULL) {
    g_printerr ("\"%s\" is not a Flash file\n", argv[1]);
    return 1;
  }

  buffer = process_buffer (buffer);
  if (buffer == NULL) {
    g_printerr ("\"%s\": Broken Flash file\n", argv[1]);
    return 1;
  }

  if (!g_file_set_contents (argv[2], (char *) buffer->data, buffer->length, &error)) {
    swfdec_buffer_unref (buffer);
    g_printerr ("Error saving: %s\n", error->message);
    return 1;
  }
  
  swfdec_buffer_unref (buffer);
  return 0;
}

