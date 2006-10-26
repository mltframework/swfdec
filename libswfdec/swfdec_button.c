/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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
#include <string.h>
#include <js/jsapi.h>
#include "swfdec_button.h"
#include "swfdec_button_movie.h"
#include "swfdec_sound.h"
#include "swfdec_sprite.h"


G_DEFINE_TYPE (SwfdecButton, swfdec_button, SWFDEC_TYPE_GRAPHIC)

static void
swfdec_button_init (SwfdecButton * button)
{
}

static void
swfdec_button_dispose (GObject *object)
{
  guint i;
  SwfdecButton *button = SWFDEC_BUTTON (object);

  g_list_foreach (button->records, (GFunc) swfdec_content_free, NULL);
  g_list_free (button->records);
  if (button->events != NULL) {
    swfdec_event_list_free (button->events);
    button->events = NULL;
  }
  for (i = 0; i < 4; i++) {
    if (button->sounds[i]) {
      swfdec_sound_chunk_free (button->sounds[i]);
      button->sounds[i] = NULL;
    }
  }

  G_OBJECT_CLASS (swfdec_button_parent_class)->dispose (G_OBJECT (button));
}

static SwfdecMovie *
swfdec_button_create_movie (SwfdecGraphic *graphic)
{
  SwfdecButtonMovie *movie = g_object_new (SWFDEC_TYPE_BUTTON_MOVIE, NULL);

  movie->button = SWFDEC_BUTTON (graphic);

  return SWFDEC_MOVIE (movie);
}

static void
swfdec_button_class_init (SwfdecButtonClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);

  object_class->dispose = swfdec_button_dispose;

  graphic_class->create_movie = swfdec_button_create_movie;
}

