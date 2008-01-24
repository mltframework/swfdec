/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_GRADIENT_PATTERN_H_
#define _SWFDEC_GRADIENT_PATTERN_H_

#include <swfdec/swfdec_pattern.h>

G_BEGIN_DECLS

typedef struct _SwfdecGradientPattern SwfdecGradientPattern;
typedef struct _SwfdecGradientPatternClass SwfdecGradientPatternClass;

typedef struct _SwfdecGradientEntry SwfdecGradientEntry;

#define SWFDEC_TYPE_GRADIENT_PATTERN                    (swfdec_gradient_pattern_get_type())
#define SWFDEC_IS_GRADIENT_PATTERN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_GRADIENT_PATTERN))
#define SWFDEC_IS_GRADIENT_PATTERN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_GRADIENT_PATTERN))
#define SWFDEC_GRADIENT_PATTERN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_GRADIENT_PATTERN, SwfdecGradientPattern))
#define SWFDEC_GRADIENT_PATTERN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_GRADIENT_PATTERN, SwfdecGradientPatternClass))
#define SWFDEC_GRADIENT_PATTERN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_GRADIENT_PATTERN, SwfdecGradientPatternClass))

struct _SwfdecGradientEntry {
  guint ratio;
  SwfdecColor color;
};

struct _SwfdecGradientPattern
{
  SwfdecPattern		pattern;

  SwfdecGradientEntry	gradient[16];		/* gradient to paint */
  SwfdecGradientEntry	end_gradient[16];     	/* end gradient for morphs */
  guint			n_gradients;		/* number of gradients */
  cairo_extend_t	extend;			/* extend of gradient */
  gboolean		radial;			/* TRUE for radial gradient, FALSE for linear gradient */
  double		focus;			/* focus point */
};

struct _SwfdecGradientPatternClass
{
  SwfdecPatternClass	pattern_class;
};

GType		swfdec_gradient_pattern_get_type	(void);

SwfdecDraw *	swfdec_gradient_pattern_new		(void);
							 

G_END_DECLS
#endif
