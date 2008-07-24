/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_shape_parser.h"

G_DEFINE_TYPE (SwfdecMorphShape, swfdec_morph_shape, SWFDEC_TYPE_SHAPE)

static void
swfdec_morph_shape_class_init (SwfdecMorphShapeClass * g_class)
{
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);
  
  graphic_class->movie_type = SWFDEC_TYPE_MORPH_MOVIE;
}

static void
swfdec_morph_shape_init (SwfdecMorphShape * morph)
{
}


int
tag_define_morph_shape (SwfdecSwfDecoder * s, guint tag)
{
  SwfdecShapeParser *parser;
  SwfdecBits bits2;
  SwfdecBits *bits = &s->b;
  SwfdecMorphShape *morph;
  guint offset;
  int id;

  id = swfdec_bits_get_u16 (bits);

  morph = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_MORPH_SHAPE);
  if (!morph)
    return SWFDEC_STATUS_OK;

  SWFDEC_INFO ("id=%d", id);

  swfdec_bits_get_rect (bits, &SWFDEC_GRAPHIC (morph)->extents);
  swfdec_bits_get_rect (bits, &morph->end_extents);
  offset = swfdec_bits_get_u32 (bits);
  swfdec_bits_init_bits (&bits2, bits, offset);

  parser = swfdec_shape_parser_new ((SwfdecParseDrawFunc) swfdec_pattern_parse_morph, 
      (SwfdecParseDrawFunc) swfdec_stroke_parse_morph, s);
  swfdec_shape_parser_parse_morph (parser, &bits2, bits);
  SWFDEC_SHAPE (morph)->draws = swfdec_shape_parser_free (parser);

  if (swfdec_bits_left (&bits2)) {
    SWFDEC_WARNING ("early finish when parsing start shapes: %u bytes",
        swfdec_bits_left (&bits2));
  }

  return SWFDEC_STATUS_OK;
}
