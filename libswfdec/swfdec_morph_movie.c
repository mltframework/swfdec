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

#include "swfdec_morph_movie.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecMorphMovie, swfdec_morph_movie, SWFDEC_TYPE_MOVIE)

static void
swfdec_morph_movie_update_extents (SwfdecMovie *movie,
    SwfdecRect *extents)
{
  guint ratio = movie->content->ratio;
  SwfdecMorphShape *morph = SWFDEC_MORPH_MOVIE (movie)->morph;
  SwfdecGraphic *graphic = SWFDEC_GRAPHIC (morph);
  extents->x0 = ((65535 - ratio) * graphic->extents.x0 + ratio * morph->end_extents.x0) / 65535;
  extents->x1 = ((65535 - ratio) * graphic->extents.x1 + ratio * morph->end_extents.x1) / 65535;
  extents->y0 = ((65535 - ratio) * graphic->extents.y0 + ratio * morph->end_extents.y0) / 65535;
  extents->y1 = ((65535 - ratio) * graphic->extents.y1 + ratio * morph->end_extents.y1) / 65535;
}

static void
swfdec_morph_movie_render (SwfdecMovie *movie, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  SwfdecMorphMovie *morph = SWFDEC_MORPH_MOVIE (movie);
  SwfdecShape *shape = SWFDEC_SHAPE (morph->morph);
  unsigned int i;

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_join (cr, CAIRO_LINE_JOIN_ROUND);

  for (i = 0; i < shape->vecs->len; i++) {
    SwfdecShapeVec *vec = &g_array_index (shape->vecs, SwfdecShapeVec, i);
    cairo_path_t *path = &morph->paths[i];

    /* FIXME: catch these two earlier */
    if (vec->pattern == NULL)
      continue;
    if (path->num_data == 0)
      continue;
    
    /* hack to not append paths for lines */
    if (!fill && vec->last_index % 2 != 0) 
      continue;

    cairo_append_path (cr, path);
    if (fill)
      swfdec_pattern_fill (vec->pattern, cr, trans, movie->content->ratio);
  }
}

static void
swfdec_cairo_path_merge (cairo_path_t *dest, const cairo_path_t *start, const cairo_path_t *end, double ratio)
{
  guint i;
  cairo_path_data_t *ddata, *sdata, *edata;
  double inv = 1.0 - ratio;

  g_assert (dest->num_data == start->num_data);
  g_assert (dest->num_data == end->num_data);

  ddata = dest->data;
  sdata = start->data;
  edata = end->data;
  for (i = 0; i < dest->num_data; i++) {
    g_assert (sdata[i].header.type == edata[i].header.type);
    ddata[i] = sdata[i];
    switch (sdata[i].header.type) {
      case CAIRO_PATH_CURVE_TO:
	ddata[i+1].point.x = sdata[i+1].point.x * inv + edata[i+1].point.x * ratio;
	ddata[i+1].point.y = sdata[i+1].point.y * inv + edata[i+1].point.y * ratio;
	ddata[i+2].point.x = sdata[i+2].point.x * inv + edata[i+2].point.x * ratio;
	ddata[i+2].point.y = sdata[i+2].point.y * inv + edata[i+2].point.y * ratio;
	i += 2;
      case CAIRO_PATH_MOVE_TO:
      case CAIRO_PATH_LINE_TO:
	ddata[i+1].point.x = sdata[i+1].point.x * inv + edata[i+1].point.x * ratio;
	ddata[i+1].point.y = sdata[i+1].point.y * inv + edata[i+1].point.y * ratio;
	i++;
      case CAIRO_PATH_CLOSE_PATH:
	break;
      default:
	g_assert_not_reached ();
    }
  }
}

static void
swfdec_morph_movie_content_changed (SwfdecMovie *movie, const SwfdecContent *content)
{
  guint i;
  SwfdecMorphMovie *morph = SWFDEC_MORPH_MOVIE (movie);
  SwfdecShape *shape = SWFDEC_SHAPE (morph->morph);

  for (i = 0; i < shape->vecs->len; i++) {
    swfdec_cairo_path_merge (&morph->paths[i], 
	&g_array_index (SWFDEC_SHAPE (morph->morph)->vecs, SwfdecShapeVec, i).path,
	&g_array_index (morph->morph->end_vecs, SwfdecShapeVec, i).path, content->ratio / 65535.);
  }
}

static void
swfdec_morph_movie_dispose (GObject *object)
{
  guint i;
  SwfdecMorphMovie *morph = SWFDEC_MORPH_MOVIE (object);

  for (i = 0; i < morph->morph->end_vecs->len; i++) {
    g_free (morph->paths[i].data);
  }
  g_free (morph->paths);
  g_object_unref (morph->morph);

  G_OBJECT_CLASS (swfdec_morph_movie_parent_class)->dispose (object);
}

static void
swfdec_morph_movie_class_init (SwfdecMorphMovieClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecMovieClass *movie_class = SWFDEC_MOVIE_CLASS (g_class);

  object_class->dispose = swfdec_morph_movie_dispose;

  movie_class->update_extents = swfdec_morph_movie_update_extents;
  movie_class->content_changed = swfdec_morph_movie_content_changed;
  movie_class->render = swfdec_morph_movie_render;
  /* FIXME */
  //movie_class->handle_mouse = swfdec_morph_movie_handle_mouse;
}

static void
swfdec_morph_movie_init (SwfdecMorphMovie *movie)
{
}

