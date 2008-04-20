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

#include <swfdec/swfdec_player_internal.h>
#include <swfdec/swfdec_resource.h>
#include <swfdec/swfdec_script_internal.h>
#include <swfdec/swfdec_sprite_movie.h>
#include <swfdec/swfdec_swf_decoder.h>

#include "vivi_code_function.h"
#include "vivi_code_text_printer.h"
#include "vivi_decompiler.h"

static SwfdecDecoder *
swfdec_decoder_new_from_file (const char *filename, GError **error)
{
  SwfdecDecoder *dec;
  SwfdecBuffer *buffer;

  g_return_val_if_fail (filename != NULL, NULL);

  buffer = swfdec_buffer_new_from_file (filename, error);
  if (buffer == NULL)
    return NULL;

  dec = swfdec_decoder_new (buffer);
  if (dec == NULL) {
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_NOSYS, 
	"file cannot be decoded by Swfdec");
    swfdec_buffer_unref (buffer);
    return NULL;
  }

  swfdec_decoder_parse (dec, buffer);
  swfdec_decoder_eof (dec);

  return dec;
}

static void
decode_script (gpointer scriptp, gpointer use_asm)
{
  SwfdecScript *script = scriptp;
  ViviCodeToken *token;
  ViviCodePrinter *printer;

  g_print ("/* %s */\n", script->name);
  if (use_asm) {
    token = VIVI_CODE_TOKEN (vivi_disassemble_script (script));
  } else {
    token = VIVI_CODE_TOKEN (vivi_code_function_new_from_script (scriptp));
  }
  printer = vivi_code_text_printer_new ();

  vivi_code_printer_print_token (printer, token);
  g_print ("\n\n");

  g_object_unref (printer);
  g_object_unref (token);
}

static void
enqueue (gpointer offset, gpointer script, gpointer listp)
{
  GList **list = listp;

  *list = g_list_prepend (*list, script);
}

static int
script_compare (gconstpointer a, gconstpointer b)
{
  const SwfdecScript *scripta = a;
  const SwfdecScript *scriptb = b;

  return scripta->main - scriptb->main;
}

int 
main (int argc, char *argv[])
{
  SwfdecSwfDecoder *dec;
  GList *scripts;
  GError *error = NULL;
  gboolean use_asm = FALSE;

  GOptionEntry options[] = {
    { "asm", 'a', 0, G_OPTION_ARG_NONE, &use_asm, "output assembler instead of decompiled script", NULL },
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

  if (argc < 2) {
    g_print ("%s FILENAME\n", argv[0]);
    return 1;
  }

  swfdec_init ();

  dec = (SwfdecSwfDecoder *) swfdec_decoder_new_from_file (argv[1], &error);
  if (dec == NULL) {
    g_printerr ("%s\n", error->message);
    g_error_free (error);
    return 1;
  }
  if (!SWFDEC_IS_SWF_DECODER (dec)) {
    g_printerr ("\"%s\" is not a Flash file.\n", argv[1]);
    g_object_unref (dec);
    return 1;
  }

  g_print ("/* version: %d - size: %ux%u */\n", dec->version,
      SWFDEC_DECODER (dec)->width, SWFDEC_DECODER (dec)->height);
  g_print ("\n");
  scripts = NULL;
  g_hash_table_foreach (dec->scripts, enqueue, &scripts);
  scripts = g_list_sort (scripts, script_compare);
  
  g_list_foreach (scripts, decode_script, GINT_TO_POINTER (use_asm));

  g_list_free (scripts);
  g_object_unref (dec);
  return 0;
}

