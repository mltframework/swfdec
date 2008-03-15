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

#include "vivi_decompiler.h"

static void
decode_script (gpointer offset, gpointer scriptp, gpointer unused)
{
  SwfdecScript *script = scriptp;
  ViviDecompiler *dec = vivi_decompiler_new (scriptp);
  guint i;

  g_print ("%s:\n", script->name);
  for (i = 0; i < vivi_decompiler_get_n_lines (dec); i++) {
    g_print ("  %s\n", vivi_decompiler_get_line (dec, i));
  }
}

int 
main (int argc, char *argv[])
{
  SwfdecPlayer *player;
  SwfdecSwfDecoder *dec;
  SwfdecURL *url;

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
  if (!SWFDEC_IS_SPRITE_MOVIE (player->priv->roots->data)) {
    g_printerr ("Error parsing file \"%s\"\n", argv[1]);
    g_object_unref (player);
    player = NULL;
    return 1;
  }
  dec = SWFDEC_SWF_DECODER (SWFDEC_MOVIE (player->priv->roots->data)->resource->decoder);

  g_hash_table_foreach (dec->scripts, decode_script, NULL);

  g_object_unref (player);
  return 0;
}

