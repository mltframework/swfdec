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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include "swfdec_pattern.h"
#include "swfdec_bits.h"
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_image.h"
#include "swfdec_stroke.h"

/*** PATTERN ***/

G_DEFINE_ABSTRACT_TYPE (SwfdecPattern, swfdec_pattern, G_TYPE_OBJECT);

static void
swfdec_pattern_class_init (SwfdecPatternClass *klass)
{
}

static void
swfdec_pattern_init (SwfdecPattern *pattern)
{
  cairo_matrix_init_identity (&pattern->start_transform);
  cairo_matrix_init_identity (&pattern->end_transform);
}

/*** COLOR PATTERN ***/

typedef struct _SwfdecColorPattern SwfdecColorPattern;
typedef struct _SwfdecColorPatternClass SwfdecColorPatternClass;

#define SWFDEC_TYPE_COLOR_PATTERN                    (swfdec_color_pattern_get_type())
#define SWFDEC_IS_COLOR_PATTERN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_COLOR_PATTERN))
#define SWFDEC_IS_COLOR_PATTERN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_COLOR_PATTERN))
#define SWFDEC_COLOR_PATTERN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_COLOR_PATTERN, SwfdecColorPattern))
#define SWFDEC_COLOR_PATTERN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_COLOR_PATTERN, SwfdecColorPatternClass))
#define SWFDEC_COLOR_PATTERN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_COLOR_PATTERN, SwfdecColorPatternClass))

struct _SwfdecColorPattern
{
  SwfdecPattern		pattern;

  SwfdecColor		start_color;		/* color to paint with at the beginning */
  SwfdecColor		end_color;		/* color to paint with in the end */
};

struct _SwfdecColorPatternClass
{
  SwfdecPatternClass	pattern_class;
};

G_DEFINE_TYPE (SwfdecColorPattern, swfdec_color_pattern, SWFDEC_TYPE_PATTERN);

static cairo_pattern_t *
swfdec_color_pattern_get_pattern (SwfdecPattern *pat, 
    const SwfdecColorTransform *trans, guint ratio)
{
  SwfdecColorPattern *pattern = SWFDEC_COLOR_PATTERN (pat);
  SwfdecColor color;

  color = swfdec_color_apply_morph (pattern->start_color, pattern->end_color, ratio);
  color = swfdec_color_apply_transform (color, trans);
  return cairo_pattern_create_rgba ( 
      SWFDEC_COLOR_R (color) / 255.0, SWFDEC_COLOR_G (color) / 255.0,
      SWFDEC_COLOR_B (color) / 255.0, SWFDEC_COLOR_A (color) / 255.0);
}

static void
swfdec_color_pattern_class_init (SwfdecColorPatternClass *klass)
{
  SWFDEC_PATTERN_CLASS (klass)->get_pattern = swfdec_color_pattern_get_pattern;
}

static void
swfdec_color_pattern_init (SwfdecColorPattern *pattern)
{
}

/*** IMAGE PATTERN ***/

typedef struct _SwfdecImagePattern SwfdecImagePattern;
typedef struct _SwfdecImagePatternClass SwfdecImagePatternClass;

#define SWFDEC_TYPE_IMAGE_PATTERN                    (swfdec_image_pattern_get_type())
#define SWFDEC_IS_IMAGE_PATTERN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_IMAGE_PATTERN))
#define SWFDEC_IS_IMAGE_PATTERN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_IMAGE_PATTERN))
#define SWFDEC_IMAGE_PATTERN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_IMAGE_PATTERN, SwfdecImagePattern))
#define SWFDEC_IMAGE_PATTERN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_IMAGE_PATTERN, SwfdecImagePatternClass))
#define SWFDEC_IMAGE_PATTERN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_IMAGE_PATTERN, SwfdecImagePatternClass))

struct _SwfdecImagePattern
{
  SwfdecPattern		pattern;

  SwfdecImage *		image;		/* image to paint */
  cairo_extend_t	extend;
  cairo_filter_t	filter;
};

struct _SwfdecImagePatternClass
{
  SwfdecPatternClass	pattern_class;
};

G_DEFINE_TYPE (SwfdecImagePattern, swfdec_image_pattern, SWFDEC_TYPE_PATTERN);

static cairo_pattern_t *
swfdec_image_pattern_get_pattern (SwfdecPattern *pat, 
    const SwfdecColorTransform *trans, guint ratio)
{
  SwfdecImagePattern *image = SWFDEC_IMAGE_PATTERN (pat);
  cairo_pattern_t *pattern;
  cairo_matrix_t mat;
  cairo_surface_t *surface;
  
  surface = swfdec_image_create_surface_transformed (image->image, trans);
  if (surface == NULL)
    return NULL;
  pattern = cairo_pattern_create_for_surface (surface);
  cairo_surface_destroy (surface);
  swfdec_matrix_morph (&mat, &pat->start_transform, &pat->end_transform, ratio);
  cairo_pattern_set_matrix (pattern, &mat);
  cairo_pattern_set_extend (pattern, image->extend);
  cairo_pattern_set_filter (pattern, image->filter);
  return pattern;
}

static void
swfdec_image_pattern_class_init (SwfdecImagePatternClass *klass)
{
  SWFDEC_PATTERN_CLASS (klass)->get_pattern = swfdec_image_pattern_get_pattern;
}

static void
swfdec_image_pattern_init (SwfdecImagePattern *pattern)
{
}

/*** GRADIENT PATTERN ***/

typedef struct _SwfdecGradientPattern SwfdecGradientPattern;
typedef struct _SwfdecGradientPatternClass SwfdecGradientPatternClass;

#define SWFDEC_TYPE_GRADIENT_PATTERN                    (swfdec_gradient_pattern_get_type())
#define SWFDEC_IS_GRADIENT_PATTERN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_GRADIENT_PATTERN))
#define SWFDEC_IS_GRADIENT_PATTERN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_GRADIENT_PATTERN))
#define SWFDEC_GRADIENT_PATTERN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_GRADIENT_PATTERN, SwfdecGradientPattern))
#define SWFDEC_GRADIENT_PATTERN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_GRADIENT_PATTERN, SwfdecGradientPatternClass))
#define SWFDEC_GRADIENT_PATTERN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_GRADIENT_PATTERN, SwfdecGradientPatternClass))

struct _SwfdecGradientPattern
{
  SwfdecPattern		pattern;

  SwfdecGradient *	gradient;		/* gradient to paint */
  gboolean		radial;			/* TRUE for radial gradient, FALSE for linear gradient */
  gboolean		morph;			/* TRUE for morph gradients */
};

struct _SwfdecGradientPatternClass
{
  SwfdecPatternClass	pattern_class;
};

G_DEFINE_TYPE (SwfdecGradientPattern, swfdec_gradient_pattern, SWFDEC_TYPE_PATTERN);

static cairo_pattern_t *
swfdec_gradient_pattern_get_pattern (SwfdecPattern *pat, 
    const SwfdecColorTransform *trans, guint ratio)
{
  guint i;
  cairo_pattern_t *pattern;
  SwfdecColor color;
  double offset;
  SwfdecGradientPattern *gradient = SWFDEC_GRADIENT_PATTERN (pat);

#if 0
  /* use this when https://bugs.freedesktop.org/show_bug.cgi?id=8341 is fixed */
  if (gradient->radial)
    pattern = cairo_pattern_create_radial (0, 0, 0, 0, 0, 16384);
  else
    pattern = cairo_pattern_create_linear (-16384.0, 0, 16384.0, 0);
  cairo_pattern_set_matrix (pattern, &pat->transform);
#else
  {
    cairo_matrix_t mat;
    swfdec_matrix_morph (&mat, &pat->start_transform, &pat->end_transform, ratio);
    if (gradient->radial)
      pattern = cairo_pattern_create_radial (0, 0, 0, 0, 0, 16384 / 256.0);
    else
      pattern = cairo_pattern_create_linear (-16384.0 / 256.0, 0, 16384.0 / 256.0, 0);
    cairo_matrix_scale (&mat, 1 / 256.0, 1 / 256.0);
    mat.x0 /= 256.0;
    mat.y0 /= 256.0;
    cairo_pattern_set_matrix (pattern, &mat);
  }
#endif
  if (gradient->morph) {
    for (i = 0; i < gradient->gradient->n_gradients; i += 2){
      color = swfdec_color_apply_morph (gradient->gradient->array[i].color,
	  gradient->gradient->array[i + 1].color, ratio);
      color = swfdec_color_apply_transform (color, trans);
      offset = gradient->gradient->array[i].ratio * (65535 - ratio) +
	      gradient->gradient->array[i + 1].ratio * ratio;
      offset /= 65535 * 255;
      cairo_pattern_add_color_stop_rgba (pattern, offset,
	  SWFDEC_COLOR_R(color) / 255.0, SWFDEC_COLOR_G(color) / 255.0,
	  SWFDEC_COLOR_B(color) / 255.0, SWFDEC_COLOR_A(color) / 255.0);
    }
  } else {
    for (i = 0; i < gradient->gradient->n_gradients; i++){
      color = swfdec_color_apply_transform (gradient->gradient->array[i].color,
	  trans);
      offset = gradient->gradient->array[i].ratio / 255.0;
      cairo_pattern_add_color_stop_rgba (pattern, offset,
	  SWFDEC_COLOR_R(color) / 255.0, SWFDEC_COLOR_G(color) / 255.0,
	  SWFDEC_COLOR_B(color) / 255.0, SWFDEC_COLOR_A(color) / 255.0);
    }
  }
  return pattern;
}

static void
swfdec_gradient_pattern_dispose (GObject *object)
{
  SwfdecGradientPattern *gradient = SWFDEC_GRADIENT_PATTERN (object);

  g_free (gradient->gradient);
  gradient->gradient = NULL;

  G_OBJECT_CLASS (swfdec_gradient_pattern_parent_class)->dispose (object);
}

static void
swfdec_gradient_pattern_class_init (SwfdecGradientPatternClass *klass)
{
  G_OBJECT_CLASS (klass)->dispose = swfdec_gradient_pattern_dispose;

  SWFDEC_PATTERN_CLASS (klass)->get_pattern = swfdec_gradient_pattern_get_pattern;
}

static void
swfdec_gradient_pattern_init (SwfdecGradientPattern *pattern)
{
}

/*** EXPORTED API ***/

/**
 * swfdec_pattern_parse:
 * @dec: a #SwfdecDecoder to parse from
 * @rgba: TRUE if colors are RGBA, FALSE if they're just RGB
 *
 * Continues parsing @dec into a new #SwfdecPattern
 *
 * Returns: a new #SwfdecPattern or NULL on error
 **/
SwfdecPattern *
swfdec_pattern_parse (SwfdecSwfDecoder *dec, gboolean rgba)
{
  guint paint_style_type;
  SwfdecBits *bits;
  SwfdecPattern *pattern;

  g_return_val_if_fail (dec != NULL, NULL);

  bits = &dec->b;
  paint_style_type = swfdec_bits_get_u8 (bits);
  SWFDEC_LOG ("    type 0x%02x", paint_style_type);

  if (paint_style_type == 0x00) {
    pattern = g_object_new (SWFDEC_TYPE_COLOR_PATTERN, NULL);
    if (rgba) {
      SWFDEC_COLOR_PATTERN (pattern)->start_color = swfdec_bits_get_rgba (bits);
    } else {
      SWFDEC_COLOR_PATTERN (pattern)->start_color = swfdec_bits_get_color (bits);
    }
    SWFDEC_COLOR_PATTERN (pattern)->end_color = SWFDEC_COLOR_PATTERN (pattern)->start_color;
    SWFDEC_LOG ("    color %08x", SWFDEC_COLOR_PATTERN (pattern)->start_color);
  } else if (paint_style_type == 0x10 || paint_style_type == 0x12) {
    pattern = g_object_new (SWFDEC_TYPE_GRADIENT_PATTERN, NULL);
    swfdec_bits_get_matrix (bits, &pattern->start_transform, NULL);
    pattern->end_transform = pattern->start_transform;
    if (rgba) {
      SWFDEC_GRADIENT_PATTERN (pattern)->gradient = swfdec_bits_get_gradient_rgba (bits);
    } else {
      SWFDEC_GRADIENT_PATTERN (pattern)->gradient = swfdec_bits_get_gradient (bits);
    }
    SWFDEC_GRADIENT_PATTERN (pattern)->radial = (paint_style_type == 0x12);
  } else if (paint_style_type >= 0x40 && paint_style_type <= 0x43) {
    guint paint_id = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("   background paint id = %d (type 0x%02x)",
	paint_id, paint_style_type);
    if (paint_id == 65535) {
      /* FIXME: someone explain this magic paint id here */
      pattern = g_object_new (SWFDEC_TYPE_COLOR_PATTERN, NULL);
      SWFDEC_COLOR_PATTERN (pattern)->start_color = SWFDEC_COLOR_COMBINE (0, 255, 255, 255);
      SWFDEC_COLOR_PATTERN (pattern)->end_color = SWFDEC_COLOR_PATTERN (pattern)->start_color;
      swfdec_bits_get_matrix (bits, &pattern->start_transform, NULL);
      pattern->end_transform = pattern->start_transform;
    } else {
      pattern = g_object_new (SWFDEC_TYPE_IMAGE_PATTERN, NULL);
      swfdec_bits_get_matrix (bits, &pattern->start_transform, NULL);
      pattern->end_transform = pattern->start_transform;
      SWFDEC_IMAGE_PATTERN (pattern)->image = swfdec_swf_decoder_get_character (dec, paint_id);
      if (!SWFDEC_IS_IMAGE (SWFDEC_IMAGE_PATTERN (pattern)->image)) {
	g_object_unref (pattern);
	SWFDEC_ERROR ("could not find image with id %u for pattern", paint_id);
	return NULL;
      }
      if (paint_style_type == 0x40 || paint_style_type == 0x42) {
	SWFDEC_IMAGE_PATTERN (pattern)->extend = CAIRO_EXTEND_REPEAT;
      } else {
	SWFDEC_IMAGE_PATTERN (pattern)->extend = CAIRO_EXTEND_NONE;
      }
      if (paint_style_type == 0x40 || paint_style_type == 0x41) {
	SWFDEC_IMAGE_PATTERN (pattern)->filter = CAIRO_FILTER_BILINEAR;
      } else {
	SWFDEC_IMAGE_PATTERN (pattern)->filter = CAIRO_FILTER_NEAREST;
      }
    }
  } else {
    SWFDEC_ERROR ("unknown paint style type 0x%02x", paint_style_type);
    return NULL;
  }
  if (cairo_matrix_invert (&pattern->start_transform)) {
    SWFDEC_ERROR ("paint transform matrix not invertible, resetting");
    cairo_matrix_init_identity (&pattern->start_transform);
    cairo_matrix_init_identity (&pattern->end_transform);
  }
  swfdec_bits_syncbits (bits);
  return pattern;
}

/**
 * swfdec_pattern_parse_morph:
 * @dec: a #SwfdecDecoder to parse from
 *
 * Continues parsing @dec into a new #SwfdecPattern. This function is used by
 * morph shapes.
 *
 * Returns: a new #SwfdecPattern or NULL on error
 **/
SwfdecPattern *
swfdec_pattern_parse_morph (SwfdecSwfDecoder *dec)
{
  guint paint_style_type;
  SwfdecBits *bits;
  SwfdecPattern *pattern;

  g_return_val_if_fail (dec != NULL, NULL);

  bits = &dec->b;
  paint_style_type = swfdec_bits_get_u8 (bits);
  SWFDEC_LOG ("    type 0x%02x", paint_style_type);

  if (paint_style_type == 0x00) {
    pattern = g_object_new (SWFDEC_TYPE_COLOR_PATTERN, NULL);
    SWFDEC_COLOR_PATTERN (pattern)->start_color = swfdec_bits_get_rgba (bits);
    SWFDEC_COLOR_PATTERN (pattern)->end_color = swfdec_bits_get_rgba (bits);
    SWFDEC_LOG ("    color %08x => %08x", SWFDEC_COLOR_PATTERN (pattern)->start_color,
	SWFDEC_COLOR_PATTERN (pattern)->end_color);
  } else if (paint_style_type == 0x10 || paint_style_type == 0x12) {
    pattern = g_object_new (SWFDEC_TYPE_GRADIENT_PATTERN, NULL);
    swfdec_bits_get_matrix (bits, &pattern->start_transform, NULL);
    swfdec_bits_get_matrix (bits, &pattern->end_transform, NULL);
    SWFDEC_GRADIENT_PATTERN (pattern)->gradient = swfdec_bits_get_morph_gradient (bits);
    SWFDEC_GRADIENT_PATTERN (pattern)->radial = (paint_style_type == 0x12);
    SWFDEC_GRADIENT_PATTERN (pattern)->morph = TRUE;
  } else if (paint_style_type >= 0x40 && paint_style_type <= 0x43) {
    guint paint_id = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("   background paint id = %d (type 0x%02x)",
	paint_id, paint_style_type);
    if (paint_id == 65535) {
      /* FIXME: someone explain this magic paint id here */
      pattern = g_object_new (SWFDEC_TYPE_COLOR_PATTERN, NULL);
      SWFDEC_COLOR_PATTERN (pattern)->start_color = SWFDEC_COLOR_COMBINE (0, 255, 255, 255);
      SWFDEC_COLOR_PATTERN (pattern)->end_color = SWFDEC_COLOR_PATTERN (pattern)->start_color;
      swfdec_bits_get_matrix (bits, &pattern->start_transform, NULL);
      swfdec_bits_get_matrix (bits, &pattern->end_transform, NULL);
    } else {
      pattern = g_object_new (SWFDEC_TYPE_IMAGE_PATTERN, NULL);
      swfdec_bits_get_matrix (bits, &pattern->start_transform, NULL);
      swfdec_bits_get_matrix (bits, &pattern->end_transform, NULL);
      SWFDEC_IMAGE_PATTERN (pattern)->image = swfdec_swf_decoder_get_character (dec, paint_id);
      if (!SWFDEC_IS_IMAGE (SWFDEC_IMAGE_PATTERN (pattern)->image)) {
	g_object_unref (pattern);
	SWFDEC_ERROR ("could not find image with id %u for pattern", paint_id);
	return NULL;
      }
      if (paint_style_type == 0x40 || paint_style_type == 0x42) {
	SWFDEC_IMAGE_PATTERN (pattern)->extend = CAIRO_EXTEND_REPEAT;
      } else {
	SWFDEC_IMAGE_PATTERN (pattern)->extend = CAIRO_EXTEND_NONE;
      }
      if (paint_style_type == 0x40 || paint_style_type == 0x41) {
	SWFDEC_IMAGE_PATTERN (pattern)->filter = CAIRO_FILTER_BILINEAR;
      } else {
	SWFDEC_IMAGE_PATTERN (pattern)->filter = CAIRO_FILTER_NEAREST;
      }
    }
  } else {
    SWFDEC_ERROR ("unknown paint style type 0x%02x", paint_style_type);
    return NULL;
  }
  if (cairo_matrix_invert (&pattern->start_transform)) {
    SWFDEC_ERROR ("paint start transform matrix not invertible, resetting");
    cairo_matrix_init_identity (&pattern->start_transform);
  }
  if (cairo_matrix_invert (&pattern->end_transform)) {
    SWFDEC_ERROR ("paint end transform matrix not invertible, resetting");
    cairo_matrix_init_identity (&pattern->end_transform);
  }
  swfdec_bits_syncbits (bits);
  return pattern;
}

/**
 * swfdec_pattern_paint:
 * @pattern: a #SwfdecPattern
 * @cr: the context to paint
 * @path: the path to paint
 * @trans: color transformation to apply before painting
 * @ratio: For morphshapes, the ratio to apply. Other objects should set this to 0.
 *
 * Fills the current path of @cr with the given @pattern.
 **/
void
swfdec_pattern_paint (SwfdecPattern *pattern, cairo_t *cr, const cairo_path_t *path,
    const SwfdecColorTransform *trans, guint ratio)
{
  cairo_pattern_t *cpattern;

  g_return_if_fail (SWFDEC_IS_PATTERN (pattern));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (path != NULL);
  g_return_if_fail (trans != NULL);
  g_return_if_fail (ratio < 65536);

  cpattern = swfdec_pattern_get_pattern (pattern, trans, ratio);
  if (cpattern == NULL)
    return;
  cairo_append_path (cr, path);
  cairo_set_source (cr, cpattern);
  cairo_pattern_destroy (cpattern);
  cairo_fill (cr);
}

/**
 * swfdec_pattern_new_color:
 * @color: color to paint in
 *
 * Creates a new pattern to paint with the given color
 *
 * Returns: a new @SwfdecPattern to paint with
 */
SwfdecPattern *	
swfdec_pattern_new_color (SwfdecColor color)
{
  SwfdecPattern *pattern = g_object_new (SWFDEC_TYPE_COLOR_PATTERN, NULL);

  SWFDEC_COLOR_PATTERN (pattern)->start_color = color;
  SWFDEC_COLOR_PATTERN (pattern)->end_color = color;

  return pattern;
}

char *
swfdec_pattern_to_string (SwfdecPattern *pattern)
{
  g_return_val_if_fail (SWFDEC_IS_PATTERN (pattern), NULL);

  if (SWFDEC_IS_IMAGE_PATTERN (pattern)) {
    SwfdecImagePattern *image = SWFDEC_IMAGE_PATTERN (pattern);
    if (image->image->width == 0)
      cairo_surface_destroy (swfdec_image_create_surface (image->image));
    return g_strdup_printf ("%ux%u image %u (%s, %s)", image->image->width,
	image->image->height, SWFDEC_CHARACTER (image->image)->id,
	image->extend == CAIRO_EXTEND_REPEAT ? "repeat" : "no repeat",
	image->filter == CAIRO_FILTER_BILINEAR ? "bilinear" : "nearest");
  } else if (SWFDEC_IS_COLOR_PATTERN (pattern)) {
    if (SWFDEC_COLOR_PATTERN (pattern)->start_color == SWFDEC_COLOR_PATTERN (pattern)->end_color)
      return g_strdup_printf ("color #%08X", SWFDEC_COLOR_PATTERN (pattern)->start_color);
    else
      return g_strdup_printf ("color #%08X => #%08X", SWFDEC_COLOR_PATTERN (pattern)->start_color,
	  SWFDEC_COLOR_PATTERN (pattern)->end_color);
  } else if (SWFDEC_IS_GRADIENT_PATTERN (pattern)) {
    SwfdecGradientPattern *gradient = SWFDEC_GRADIENT_PATTERN (pattern);
    return g_strdup_printf ("%s gradient (%u colors)", gradient->radial ? "radial" : "linear",
	gradient->gradient->n_gradients);
  } else if (SWFDEC_IS_STROKE (pattern)) {
    SwfdecStroke *line = SWFDEC_STROKE (pattern);
    return g_strdup_printf ("line (width %u, color #%08X)", line->start_width, line->start_color);
  } else {
    return g_strdup_printf ("%s", G_OBJECT_TYPE_NAME (pattern));
  }
}

static void
swfdec_path_get_extents (const cairo_path_t *path, SwfdecRect *extents, double line_width)
{
  cairo_path_data_t *data = path->data;
  int i;
  double x = 0, y = 0;
  gboolean need_current = TRUE;
  gboolean start = TRUE;
#define ADD_POINT(rect, x, y) G_STMT_START{ \
  if (rect->x0 > x + line_width) \
    rect->x0 = x + line_width; \
  if (rect->x1 < x - line_width) \
    rect->x1 = x - line_width; \
  if (rect->y0 > y + line_width) \
    rect->y0 = y + line_width; \
  if (rect->y1 < y - line_width) \
    rect->y1 = y - line_width; \
}G_STMT_END
  g_assert (line_width >= 0.0);
  for (i = 0; i < path->num_data; i++) {
    switch (data[i].header.type) {
      case CAIRO_PATH_CURVE_TO:
	if (need_current) {
	  if (start) {
	    start = FALSE;
	    extents->x0 = x - line_width;
	    extents->x1 = x + line_width;
	    extents->y0 = y - line_width;
	    extents->y1 = y + line_width;
	  } else {
	    ADD_POINT (extents, x, y);
	  }
	  need_current = FALSE;
	}
	ADD_POINT (extents, data[i+1].point.x, data[i+1].point.y);
	ADD_POINT (extents, data[i+2].point.x, data[i+2].point.y);
	ADD_POINT (extents, data[i+3].point.x, data[i+3].point.y);
	i += 3;
	break;
      case CAIRO_PATH_LINE_TO:
	if (need_current) {
	  if (start) {
	    start = FALSE;
	    extents->x0 = x - line_width;
	    extents->x1 = x + line_width;
	    extents->y0 = y - line_width;
	    extents->y1 = y + line_width;
	  } else {
	    ADD_POINT (extents, x, y);
	  }
	  need_current = FALSE;
	}
	ADD_POINT (extents, data[i+1].point.x, data[i+1].point.y);
	i++;
	break;
      case CAIRO_PATH_CLOSE_PATH:
	x = 0;
	y = 0;
	break;
      case CAIRO_PATH_MOVE_TO:
	x = data[i+1].point.x;
	y = data[i+1].point.y;
	i++;
	break;
      default:
	g_assert_not_reached ();
    }
  }
#undef ADD_POINT
}

#define MAX_ALIGN 10

void
swfdec_pattern_get_path_extents (SwfdecPattern *pattern, const cairo_path_t *path, SwfdecRect *extents)
{
  if (SWFDEC_IS_STROKE (pattern)) {
    SwfdecStroke *stroke = SWFDEC_STROKE (pattern);
    double line_width = MAX (stroke->start_width, stroke->end_width);
    line_width = MAX (line_width, SWFDEC_TWIPS_SCALE_FACTOR);
    swfdec_path_get_extents (path, extents, line_width / 2);
    /* add offsets for pixel-alignment here */
    extents->x0 -= MAX_ALIGN;
    extents->y0 -= MAX_ALIGN;
    extents->x1 += MAX_ALIGN;
    extents->y1 += MAX_ALIGN;
  } else {
    swfdec_path_get_extents (path, extents, 0.0);
  }
}

cairo_pattern_t *
swfdec_pattern_get_pattern (SwfdecPattern *pattern, 
    const SwfdecColorTransform *trans, guint ratio)
{
  SwfdecPatternClass *klass;

  g_return_val_if_fail (SWFDEC_IS_PATTERN (pattern), NULL);
  g_return_val_if_fail (trans != NULL, NULL);
  g_return_val_if_fail (ratio < 65536, NULL);

  klass = SWFDEC_PATTERN_GET_CLASS (pattern);
  g_assert (klass->get_pattern);
  return klass->get_pattern (pattern, trans, ratio);
}

