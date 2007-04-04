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

#include "swfdec_morphshape.h"
#include "swfdec_debug.h"
#include "swfdec_image.h"
#include "swfdec_morph_movie.h"

G_DEFINE_TYPE (SwfdecMorphShape, swfdec_morph_shape, SWFDEC_TYPE_SHAPE)

static SwfdecMovie *
swfdec_graphic_create_movie (SwfdecGraphic *graphic, gsize *size)
{
  guint i;
  SwfdecMorphShape *morph = SWFDEC_MORPH_SHAPE (graphic);
  SwfdecMorphMovie *movie = g_object_new (SWFDEC_TYPE_MORPH_MOVIE, NULL);

  movie->morph = morph;
  g_object_ref (morph);

  *size = sizeof (SwfdecMorphMovie) + sizeof (cairo_path_t) * morph->end_vecs->len;
  movie->paths = g_new0 (cairo_path_t, morph->end_vecs->len);
  for (i = 0; i < morph->end_vecs->len; i++) {
    movie->paths[i].num_data = g_array_index (morph->end_vecs, SwfdecShapeVec, i).path.num_data;
    movie->paths[i].data = g_new (cairo_path_data_t, movie->paths[i].num_data);
    *size += sizeof (cairo_path_data_t) * movie->paths[i].num_data;
  }

  return SWFDEC_MOVIE (movie);
}

static void
swfdec_morph_shape_dispose (GObject *object)
{
  SwfdecMorphShape *morph = SWFDEC_MORPH_SHAPE (object);
  guint i;

  if (morph->end_vecs != NULL) {
    for (i = 0; i < morph->end_vecs->len; i++)
      g_free (g_array_index (morph->end_vecs, SwfdecShapeVec, i).path.data);
    g_array_free (morph->end_vecs, TRUE);
    morph->end_vecs = NULL;
  }

  G_OBJECT_CLASS (swfdec_morph_shape_parent_class)->dispose (object);
}

static void
swfdec_morph_shape_class_init (SwfdecMorphShapeClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);
  
  object_class->dispose = swfdec_morph_shape_dispose;

  graphic_class->create_movie = swfdec_graphic_create_movie;
}

static void
swfdec_morph_shape_init (SwfdecMorphShape * morph)
{
  morph->end_vecs = g_array_new (FALSE, TRUE, sizeof (SwfdecShapeVec));
}


