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

#include <stdlib.h>
#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_player_internal.h>
#include <libswfdec/swfdec_root_movie.h>
#include <libswfdec/swfdec_sound.h>
#include <libswfdec/swfdec_swf_decoder.h>

static void
usage (const char *app)
{
  g_print ("usage: %s SWFFILE ID OUTFILE\n\n", app);
}

int
main (int argc, char *argv[])
{
  SwfdecSound *sound;
  SwfdecPlayer *player;
  GError *error = NULL;
  guint id;

  swfdec_init ();

  if (argc != 4) {
    usage (argv[0]);
    return 0;
  }

  player = swfdec_player_new_from_file (argv[1], &error);
  if (player == NULL) {
    g_printerr ("Couldn't open file \"%s\": %s\n", argv[1], error->message);
    g_error_free (error);
    return 1;
  }
  if (swfdec_player_get_rate (player) == 0) {
    g_printerr ("Error parsing file \"%s\"\n", argv[1]);
    g_object_unref (player);
    player = NULL;
    return 1;
  }
  id = strtoul (argv[2], NULL, 0);
  sound = (SwfdecSound *) swfdec_swf_decoder_get_character (
      SWFDEC_SWF_DECODER (SWFDEC_ROOT_MOVIE (player->roots->data)->decoder),
      id);
  if (!SWFDEC_IS_SOUND (sound) || sound->decoded == NULL) {
    g_printerr ("id %u does not reference a sound object", id);
    g_object_unref (player);
    player = NULL;
    return 1;
  }
  if (!g_file_set_contents (argv[3], (char *) sound->decoded->data, 
	sound->decoded->length, &error)) {
    g_printerr ("Couldn't save sound to file \"%s\": %s\n", argv[1], error->message);
    g_error_free (error);
    g_object_unref (player);
    player = NULL;
    return 1;
  }
  g_object_unref (player);
  player = NULL;

  return 0;
}

