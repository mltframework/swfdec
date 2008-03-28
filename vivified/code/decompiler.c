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

static void
decode_script (gpointer scriptp, gpointer unused)
{
  SwfdecScript *script = scriptp;
  ViviCodeValue *fun;
  ViviCodePrinter *printer;

  g_print ("/* %s */\n", script->name);
  fun = vivi_code_function_new (scriptp);
  printer = vivi_code_text_printer_new ();

  vivi_code_printer_print_token (printer, VIVI_CODE_TOKEN (fun));
  g_print ("\n\n");

  g_object_unref (printer);
  g_object_unref (fun);
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
  SwfdecPlayer *player;
  SwfdecSwfDecoder *dec;
  SwfdecURL *url;
  GList *scripts;

  if (argc < 2) {
    g_print ("%s FILENAME\n", argv[0]);
    return 1;
  }

  player = swfdec_player_new (NULL);
  url = swfdec_url_new_from_input (argv[1]);
  swfdec_player_set_url (player, url);
  swfdec_url_free (url);
  /* FIXME: HACK! */
  swfdec_player_advance (player, 0);
  if (player->priv->roots == NULL ||
      !SWFDEC_IS_SPRITE_MOVIE (player->priv->roots->data) ||
      (dec = SWFDEC_SWF_DECODER (SWFDEC_MOVIE (player->priv->roots->data)->resource->decoder)) == NULL) {
    g_printerr ("Error parsing file \"%s\"\n", argv[1]);
    g_object_unref (player);
    player = NULL;
    return 1;
  }

  g_print ("/* version: %d - size: %ux%u */\n", dec->version,
      player->priv->width, player->priv->height);
  g_print ("\n");
  scripts = NULL;
  g_hash_table_foreach (dec->scripts, enqueue, &scripts);
  scripts = g_list_sort (scripts, script_compare);
  
  g_list_foreach (scripts, decode_script, NULL);

  g_list_free (scripts);
  g_object_unref (player);
  return 0;
}

