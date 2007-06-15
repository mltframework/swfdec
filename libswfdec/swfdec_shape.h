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

#ifndef _SWFDEC_SHAPE_H_
#define _SWFDEC_SHAPE_H_

#include <libswfdec/swfdec_types.h>
#include <libswfdec/swfdec_color.h>
#include <libswfdec/swfdec_bits.h>
#include <libswfdec/swfdec_graphic.h>
#include <libswfdec/swfdec_pattern.h>
#include <libswfdec/swfdec_stroke.h>
#include <libswfdec/swfdec_swf_decoder.h>

G_BEGIN_DECLS

//typedef struct _SwfdecShape SwfdecShape;
typedef struct _SwfdecShapeClass SwfdecShapeClass;

typedef SwfdecPattern * (* SwfdecPatternFunc) (SwfdecSwfDecoder * s);
typedef SwfdecStroke * (* SwfdecStrokeFunc) (SwfdecSwfDecoder * s);

#define SWFDEC_TYPE_SHAPE                    (swfdec_shape_get_type())
#define SWFDEC_IS_SHAPE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SHAPE))
#define SWFDEC_IS_SHAPE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SHAPE))
#define SWFDEC_SHAPE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SHAPE, SwfdecShape))
#define SWFDEC_SHAPE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SHAPE, SwfdecShapeClass))

struct _SwfdecShapeVec
{
  gpointer pattern;			/* pattern or stroke to display */
  cairo_path_t path;			/* accumulated path */
  SwfdecRect extents;			/* extents of path */
  guint last_index;			/* index of last segment that was added */
  /* cache */
  cairo_t *fill_cr;			/* use for mouse hit tests */
};

struct _SwfdecShape
{
  SwfdecGraphic graphic;

  GArray *vecs;
  GPtrArray *lines;
  GPtrArray *fills;

  /* used while defining */
  guint fills_offset;
  guint lines_offset;
  guint n_fill_bits;
  guint n_line_bits;
};

struct _SwfdecShapeClass
{
  SwfdecGraphicClass graphic_class;
};

GType swfdec_shape_get_type (void);

int tag_define_shape (SwfdecSwfDecoder * s, guint tag);
int tag_define_shape_3 (SwfdecSwfDecoder * s, guint tag);
int tag_define_shape_4 (SwfdecSwfDecoder * s, guint tag);
void swfdec_shape_get_recs (SwfdecSwfDecoder * s, SwfdecShape * shape,
    SwfdecPatternFunc pattern_func, SwfdecStrokeFunc stroke_func);


G_END_DECLS
#endif
