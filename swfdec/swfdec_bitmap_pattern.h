/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_BITMAP_PATTERN_H_
#define _SWFDEC_BITMAP_PATTERN_H_

#include <swfdec/swfdec_bitmap_data.h>
#include <swfdec/swfdec_pattern.h>

G_BEGIN_DECLS


typedef struct _SwfdecBitmapPattern SwfdecBitmapPattern;
typedef struct _SwfdecBitmapPatternClass SwfdecBitmapPatternClass;

#define SWFDEC_TYPE_BITMAP_PATTERN                    (swfdec_bitmap_pattern_get_type())
#define SWFDEC_IS_BITMAP_PATTERN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_BITMAP_PATTERN))
#define SWFDEC_IS_BITMAP_PATTERN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_BITMAP_PATTERN))
#define SWFDEC_BITMAP_PATTERN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_BITMAP_PATTERN, SwfdecBitmapPattern))
#define SWFDEC_BITMAP_PATTERN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_BITMAP_PATTERN, SwfdecBitmapPatternClass))

struct _SwfdecBitmapPattern {
  SwfdecPattern		pattern;

  SwfdecBitmapData *	bitmap;		/* the bitmap we are attached to */
  cairo_extend_t	extend;
  cairo_filter_t	filter;
};

struct _SwfdecBitmapPatternClass {
  SwfdecPatternClass	pattern_class;
};

GType		swfdec_bitmap_pattern_get_type		(void);

SwfdecPattern *	swfdec_bitmap_pattern_new		(SwfdecBitmapData *	bitmap);


G_END_DECLS
#endif
