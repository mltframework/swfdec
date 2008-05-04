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
#include "swfdec_gradient_pattern.h"
#include "swfdec_image.h"
#include "swfdec_path.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_stroke.h"

/*** PATTERN ***/

G_DEFINE_ABSTRACT_TYPE (SwfdecPattern, swfdec_pattern, SWFDEC_TYPE_DRAW);

static void
swfdec_pattern_compute_extents (SwfdecDraw *draw)
{
  swfdec_path_get_extents (&draw->path, &draw->extents);
}

static void
swfdec_pattern_paint (SwfdecDraw *draw, cairo_t *cr, const SwfdecColorTransform *trans)
{
  cairo_pattern_t *pattern;

  pattern = swfdec_pattern_get_pattern (SWFDEC_PATTERN (draw), swfdec_renderer_get (cr),
      trans);
  if (pattern == NULL)
    return;
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_append_path (cr, &draw->path);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
  cairo_fill (cr);
}

static void
swfdec_pattern_morph (SwfdecDraw *dest, SwfdecDraw *source, guint ratio)
{
  SwfdecPattern *dpattern = SWFDEC_PATTERN (dest);
  SwfdecPattern *spattern = SWFDEC_PATTERN (source);

  swfdec_matrix_morph (&dpattern->start_transform,
      &spattern->start_transform, &spattern->end_transform, ratio);
  dpattern->transform = dpattern->start_transform;
  if (cairo_matrix_invert (&dpattern->transform)) {
    SWFDEC_ERROR ("morphed paint transform matrix not invertible, using default");
    dpattern->transform = spattern->transform;
  }

  SWFDEC_DRAW_CLASS (swfdec_pattern_parent_class)->morph (dest, source, ratio);
}

static gboolean
swfdec_pattern_contains (SwfdecDraw *draw, cairo_t *cr, double x, double y)
{
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  cairo_append_path (cr, &draw->path);
  return cairo_in_fill (cr, x, y);
}

static void
swfdec_pattern_class_init (SwfdecPatternClass *klass)
{
  SwfdecDrawClass *draw_class = SWFDEC_DRAW_CLASS (klass);

  draw_class->morph = swfdec_pattern_morph;
  draw_class->paint = swfdec_pattern_paint;
  draw_class->compute_extents = swfdec_pattern_compute_extents;
  draw_class->contains = swfdec_pattern_contains;
}

static void
swfdec_pattern_init (SwfdecPattern *pattern)
{
  cairo_matrix_init_identity (&pattern->transform);
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

GType swfdec_color_pattern_get_type (void);
G_DEFINE_TYPE (SwfdecColorPattern, swfdec_color_pattern, SWFDEC_TYPE_PATTERN);

static void
swfdec_color_pattern_morph (SwfdecDraw *dest, SwfdecDraw *source, guint ratio)
{
  SwfdecColorPattern *dpattern = SWFDEC_COLOR_PATTERN (dest);
  SwfdecColorPattern *spattern = SWFDEC_COLOR_PATTERN (source);

  dpattern->start_color = swfdec_color_apply_morph (spattern->start_color, spattern->end_color, ratio);

  SWFDEC_DRAW_CLASS (swfdec_color_pattern_parent_class)->morph (dest, source, ratio);
}

static cairo_pattern_t *
swfdec_color_pattern_get_pattern (SwfdecPattern *pat, SwfdecRenderer *renderer,
    const SwfdecColorTransform *trans)
{
  SwfdecColor color = SWFDEC_COLOR_PATTERN (pat)->start_color;

  color = swfdec_color_apply_transform (color, trans);
  return cairo_pattern_create_rgba ( 
      SWFDEC_COLOR_R (color) / 255.0, SWFDEC_COLOR_G (color) / 255.0,
      SWFDEC_COLOR_B (color) / 255.0, SWFDEC_COLOR_A (color) / 255.0);
}

static void
swfdec_color_pattern_class_init (SwfdecColorPatternClass *klass)
{
  SWFDEC_DRAW_CLASS (klass)->morph = swfdec_color_pattern_morph;

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

GType swfdec_image_pattern_get_type (void);
G_DEFINE_TYPE (SwfdecImagePattern, swfdec_image_pattern, SWFDEC_TYPE_PATTERN);

static void
swfdec_image_pattern_morph (SwfdecDraw *dest, SwfdecDraw *source, guint ratio)
{
  SwfdecImagePattern *dpattern = SWFDEC_IMAGE_PATTERN (dest);
  SwfdecImagePattern *spattern = SWFDEC_IMAGE_PATTERN (source);

  dpattern->image = g_object_ref (spattern->image);
  dpattern->extend = spattern->extend;
  dpattern->filter = spattern->filter;

  SWFDEC_DRAW_CLASS (swfdec_image_pattern_parent_class)->morph (dest, source, ratio);
}

static cairo_pattern_t *
swfdec_image_pattern_get_pattern (SwfdecPattern *pat, SwfdecRenderer *renderer,
    const SwfdecColorTransform *trans)
{
  SwfdecImagePattern *image = SWFDEC_IMAGE_PATTERN (pat);
  cairo_pattern_t *pattern;
  cairo_surface_t *surface;
  
  if (swfdec_color_transform_is_mask (trans))
    return cairo_pattern_create_rgb (0, 0, 0);
  surface = swfdec_image_create_surface_transformed (image->image, renderer, trans);
  if (surface == NULL)
    return NULL;
  pattern = cairo_pattern_create_for_surface (surface);
  cairo_surface_destroy (surface);
  cairo_pattern_set_matrix (pattern, &pat->transform);
  cairo_pattern_set_extend (pattern, image->extend);
  cairo_pattern_set_filter (pattern, image->filter);
  return pattern;
}

static void
swfdec_image_pattern_class_init (SwfdecImagePatternClass *klass)
{
  SWFDEC_DRAW_CLASS (klass)->morph = swfdec_image_pattern_morph;

  SWFDEC_PATTERN_CLASS (klass)->get_pattern = swfdec_image_pattern_get_pattern;
}

static void
swfdec_image_pattern_init (SwfdecImagePattern *pattern)
{
}

/*** EXPORTED API ***/

static SwfdecDraw *
swfdec_pattern_do_parse (SwfdecBits *bits, SwfdecSwfDecoder *dec, gboolean rgba)
{
  guint paint_style_type;
  SwfdecPattern *pattern;

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
  } else if (paint_style_type == 0x10 || paint_style_type == 0x12 || paint_style_type == 0x13) {
    SwfdecGradientPattern *gradient;
    guint i, interpolation;
    pattern = SWFDEC_PATTERN (swfdec_gradient_pattern_new ());
    gradient = SWFDEC_GRADIENT_PATTERN (pattern);
    swfdec_bits_get_matrix (bits, &pattern->start_transform, NULL);
    pattern->end_transform = pattern->start_transform;
    switch (swfdec_bits_getbits (bits, 2)) {
      case 0:
	gradient->extend = CAIRO_EXTEND_PAD;
	break;
      case 1:
	gradient->extend = CAIRO_EXTEND_REFLECT;
	break;
      case 2:
	gradient->extend = CAIRO_EXTEND_REPEAT;
	break;
      case 3:
	SWFDEC_ERROR ("spread mode 3 is undefined for gradients");
	gradient->extend = CAIRO_EXTEND_PAD;
	break;
      default:
	g_assert_not_reached ();
    }
    interpolation = swfdec_bits_getbits (bits, 2);
    if (interpolation) {
      SWFDEC_FIXME ("only normal interpolation is implemented, mode %u is not", interpolation);
    }
    gradient->n_gradients = swfdec_bits_getbits (bits, 4);
    for (i = 0; i < gradient->n_gradients; i++) {
      gradient->gradient[i].ratio = swfdec_bits_get_u8 (bits);
      if (rgba)
	gradient->gradient[i].color = swfdec_bits_get_rgba (bits);
      else
	gradient->gradient[i].color = swfdec_bits_get_color (bits);
    }
    gradient->radial = (paint_style_type != 0x10);
    /* FIXME: need a way to ensure 0x13 only happens in Flash 8 */
    if (paint_style_type == 0x13) {
      gradient->focus = swfdec_bits_get_s16 (bits) / 256.0;
    }
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
	SWFDEC_IMAGE_PATTERN (pattern)->extend = CAIRO_EXTEND_PAD;
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
  pattern->transform = pattern->start_transform;
  if (cairo_matrix_invert (&pattern->transform)) {
    SWFDEC_ERROR ("paint transform matrix not invertible, resetting");
    cairo_matrix_init_identity (&pattern->transform);
  }
  swfdec_bits_syncbits (bits);
  return SWFDEC_DRAW (pattern);
}

/**
 * swfdec_pattern_parse:
 * @bits: the bits to parse from
 * @dec: a #SwfdecDecoder to take context from
 * @rgba: TRUE if colors are RGBA, FALSE if they're just RGB
 *
 * Continues parsing @dec into a new #SwfdecPattern
 *
 * Returns: a new #SwfdecPattern or NULL on error
 **/
SwfdecDraw *
swfdec_pattern_parse (SwfdecBits *bits, SwfdecSwfDecoder *dec)
{
  g_return_val_if_fail (bits != NULL, NULL);
  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (dec), NULL);

  return swfdec_pattern_do_parse (bits, dec, FALSE);
}

SwfdecDraw *
swfdec_pattern_parse_rgba (SwfdecBits *bits, SwfdecSwfDecoder *dec)
{
  g_return_val_if_fail (bits != NULL, NULL);
  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (dec), NULL);

  return swfdec_pattern_do_parse (bits, dec, TRUE);
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
SwfdecDraw *
swfdec_pattern_parse_morph (SwfdecBits *bits, SwfdecSwfDecoder *dec)
{
  guint paint_style_type;
  SwfdecPattern *pattern;

  g_return_val_if_fail (bits != NULL, NULL);
  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (dec), NULL);

  paint_style_type = swfdec_bits_get_u8 (bits);
  SWFDEC_LOG ("    type 0x%02x", paint_style_type);

  if (paint_style_type == 0x00) {
    pattern = g_object_new (SWFDEC_TYPE_COLOR_PATTERN, NULL);
    SWFDEC_COLOR_PATTERN (pattern)->start_color = swfdec_bits_get_rgba (bits);
    SWFDEC_COLOR_PATTERN (pattern)->end_color = swfdec_bits_get_rgba (bits);
    SWFDEC_LOG ("    color %08x => %08x", SWFDEC_COLOR_PATTERN (pattern)->start_color,
	SWFDEC_COLOR_PATTERN (pattern)->end_color);
  } else if (paint_style_type == 0x10 || paint_style_type == 0x12 || paint_style_type == 0x13) {
    SwfdecGradientPattern *gradient;
    guint i, interpolation;
    pattern = SWFDEC_PATTERN (swfdec_gradient_pattern_new ());
    gradient = SWFDEC_GRADIENT_PATTERN (pattern);
    swfdec_bits_get_matrix (bits, &pattern->start_transform, NULL);
    swfdec_bits_get_matrix (bits, &pattern->end_transform, NULL);
    switch (swfdec_bits_getbits (bits, 2)) {
      case 0:
	gradient->extend = CAIRO_EXTEND_PAD;
	break;
      case 1:
	gradient->extend = CAIRO_EXTEND_REFLECT;
	break;
      case 2:
	gradient->extend = CAIRO_EXTEND_REPEAT;
	break;
      case 3:
	SWFDEC_ERROR ("spread mode 3 is undefined for gradients");
	gradient->extend = CAIRO_EXTEND_PAD;
	break;
      default:
	g_assert_not_reached ();
    }
    interpolation = swfdec_bits_getbits (bits, 2);
    if (interpolation) {
      SWFDEC_FIXME ("only normal interpolation is implemented, mode %u is not", interpolation);
    }
    gradient->n_gradients = swfdec_bits_getbits (bits, 4);
    for (i = 0; i < gradient->n_gradients; i++) {
      gradient->gradient[i].ratio = swfdec_bits_get_u8 (bits);
      gradient->gradient[i].color = swfdec_bits_get_rgba (bits);
      gradient->end_gradient[i].ratio = swfdec_bits_get_u8 (bits);
      gradient->end_gradient[i].color = swfdec_bits_get_rgba (bits);
    }
    gradient->radial = (paint_style_type != 0x10);
    /* FIXME: need a way to ensure 0x13 only happens in Flash 8 */
    if (paint_style_type == 0x13) {
      gradient->focus = swfdec_bits_get_s16 (bits) / 256.0;
    }
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
	SWFDEC_IMAGE_PATTERN (pattern)->extend = CAIRO_EXTEND_PAD;
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
  pattern->transform = pattern->start_transform;
  if (cairo_matrix_invert (&pattern->transform)) {
    SWFDEC_ERROR ("paint transform matrix not invertible, resetting");
    cairo_matrix_init_identity (&pattern->transform);
  }
  swfdec_bits_syncbits (bits);
  return SWFDEC_DRAW (pattern);
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
      cairo_surface_destroy (swfdec_image_create_surface (image->image, NULL));
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
	gradient->n_gradients);
  } else {
    return g_strdup_printf ("%s", G_OBJECT_TYPE_NAME (pattern));
  }
}

cairo_pattern_t *
swfdec_pattern_get_pattern (SwfdecPattern *pattern, SwfdecRenderer *renderer,
    const SwfdecColorTransform *trans)
{
  SwfdecPatternClass *klass;

  g_return_val_if_fail (SWFDEC_IS_PATTERN (pattern), NULL);
  g_return_val_if_fail (SWFDEC_IS_RENDERER (renderer), NULL);
  g_return_val_if_fail (trans != NULL, NULL);

  klass = SWFDEC_PATTERN_GET_CLASS (pattern);
  g_assert (klass->get_pattern);
  return klass->get_pattern (pattern, renderer, trans);
}

