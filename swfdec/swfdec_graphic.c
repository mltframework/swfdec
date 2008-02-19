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

#include "swfdec_graphic.h"
#include "swfdec_graphic_movie.h"


G_DEFINE_ABSTRACT_TYPE (SwfdecGraphic, swfdec_graphic, SWFDEC_TYPE_CHARACTER)

static SwfdecMovie *
swfdec_graphic_create_movie (SwfdecGraphic *graphic, gsize *size)
{
  SwfdecGraphicMovie *movie = g_object_new (SWFDEC_TYPE_GRAPHIC_MOVIE, NULL);

  *size = sizeof (SwfdecGraphicMovie);
  return SWFDEC_MOVIE (movie);
}

static void
swfdec_graphic_class_init (SwfdecGraphicClass *klass)
{
  klass->create_movie = swfdec_graphic_create_movie;
}

static void
swfdec_graphic_init (SwfdecGraphic *graphic)
{
}

void
swfdec_graphic_render (SwfdecGraphic *graphic, cairo_t *cr,
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  SwfdecGraphicClass *klass = SWFDEC_GRAPHIC_GET_CLASS (graphic);

  if (klass->render)
    klass->render (graphic, cr, trans, inval);
}

gboolean
swfdec_graphic_mouse_in (SwfdecGraphic *graphic, double x, double y)
{
  SwfdecGraphicClass *klass = SWFDEC_GRAPHIC_GET_CLASS (graphic);

  if (klass->mouse_in)
    return klass->mouse_in (graphic, x, y);
  else
    return FALSE;
}
