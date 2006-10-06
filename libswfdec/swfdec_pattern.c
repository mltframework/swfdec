#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_pattern.h"
#include "color.h"
#include "swfdec_bits.h"
#include "swfdec_cache.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_image.h"
#include "swfdec_object.h"

/*** PATTERN ***/

G_DEFINE_ABSTRACT_TYPE (SwfdecPattern, swfdec_pattern, G_TYPE_OBJECT);

static void
swfdec_pattern_class_init (SwfdecPatternClass *klass)
{
}

static void
swfdec_pattern_init (SwfdecPattern *pattern)
{
  cairo_matrix_init_identity (&pattern->transform);
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

  swf_color		color;		/* color to paint with */
};

struct _SwfdecColorPatternClass
{
  SwfdecPatternClass	pattern_class;
};

G_DEFINE_TYPE (SwfdecColorPattern, swfdec_color_pattern, SWFDEC_TYPE_PATTERN);

static void
swfdec_color_pattern_fill (SwfdecPattern *pattern, cairo_t *cr,
    const SwfdecColorTransform *trans, unsigned int ratio)
{
  swf_color color;

  color = swfdec_color_apply_transform (SWFDEC_COLOR_PATTERN (pattern)->color, trans);
  swfdec_color_set_source (cr, color);
  cairo_fill (cr);
}

static void
swfdec_color_pattern_class_init (SwfdecColorPatternClass *klass)
{
  SWFDEC_PATTERN_CLASS (klass)->fill = swfdec_color_pattern_fill;
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

static void
swfdec_image_pattern_fill (SwfdecPattern *pat, cairo_t *cr,
    const SwfdecColorTransform *trans, unsigned int ratio)
{
  SwfdecImagePattern *image = SWFDEC_IMAGE_PATTERN (pat);
  cairo_surface_t *src_surface;
  cairo_pattern_t *pattern;
  swf_color color;
  unsigned char *image_data = swfdec_handle_get_data(image->image->handle);
  
  color = swfdec_color_apply_transform (0xFFFFFFFF, trans);
  src_surface = cairo_image_surface_create_for_data (image_data,
      CAIRO_FORMAT_ARGB32, image->image->width, image->image->height,
      image->image->rowstride);
  pattern = cairo_pattern_create_for_surface (src_surface);
  cairo_surface_destroy (src_surface);
  cairo_pattern_set_matrix (pattern, &pat->transform);
  cairo_pattern_set_extend (pattern, image->extend);
  cairo_pattern_set_filter (pattern, image->filter);
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
  if (SWF_COLOR_A (color) < 255) {
    cairo_clip (cr);
    cairo_paint_with_alpha (cr, SWF_COLOR_A (color) / 255.);
  } else {
    cairo_fill (cr);
  }
}

static void
swfdec_image_pattern_class_init (SwfdecImagePatternClass *klass)
{
  SWFDEC_PATTERN_CLASS (klass)->fill = swfdec_image_pattern_fill;
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
};

struct _SwfdecGradientPatternClass
{
  SwfdecPatternClass	pattern_class;
};

G_DEFINE_TYPE (SwfdecGradientPattern, swfdec_gradient_pattern, SWFDEC_TYPE_PATTERN);

static void
swfdec_gradient_pattern_fill (SwfdecPattern *pat, cairo_t *cr,
    const SwfdecColorTransform *trans, unsigned int ratio)
{
  unsigned int i;
  cairo_pattern_t *pattern;
  swf_color color;
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
    cairo_matrix_t mat = pat->transform;
    if (gradient->radial)
      pattern = cairo_pattern_create_radial (0, 0, 0, 0, 0, 16384 / 256.0);
    else
      pattern = cairo_pattern_create_linear (-16384.0 / 256.0, 0, 16384.0 / 256.0, 0);
    cairo_matrix_scale (&mat, 1 / 256.0, 1 / 256.0);
    cairo_pattern_set_matrix (pattern, &mat);
  }
#endif
  for (i = 0; i < gradient->gradient->n_gradients; i++){
    color = swfdec_color_apply_transform (gradient->gradient->array[i].color,
	trans);
    cairo_pattern_add_color_stop_rgba (pattern,
	gradient->gradient->array[i].ratio/255.0,
	SWF_COLOR_R(color) / 255.0, SWF_COLOR_G(color) / 255.0,
	SWF_COLOR_B(color) / 255.0, SWF_COLOR_A(color) / 255.0);
  }
  cairo_set_source (cr, pattern);
  cairo_pattern_destroy (pattern);
  cairo_fill (cr);
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

  SWFDEC_PATTERN_CLASS (klass)->fill = swfdec_gradient_pattern_fill;
}

static void
swfdec_gradient_pattern_init (SwfdecGradientPattern *pattern)
{
}

/*** EXPORTED API ***/

/**
 * swfdec_pattern_parse:
 * @dec: a #SwfdecDecoder
 * @rgba: TRUE if colors are RGBA, FALSE if they're just RGB
 *
 * Parses the bits pointer at by the decoder into a new #SwfdecPattern
 *
 * Returns: a new #SwfdecPattern or NULL on error
 **/
SwfdecPattern *
swfdec_pattern_parse (SwfdecDecoder *dec, gboolean rgba)
{
  unsigned int fill_style_type;
  SwfdecBits *bits;
  SwfdecPattern *pattern;

  g_return_val_if_fail (SWFDEC_IS_DECODER (dec), NULL);

  bits = &dec->b;
  fill_style_type = swfdec_bits_get_u8 (bits);
  SWFDEC_LOG ("    type 0x%02x", fill_style_type);

  if (fill_style_type == 0x00) {
    pattern = g_object_new (SWFDEC_TYPE_COLOR_PATTERN, NULL);
    if (rgba) {
      SWFDEC_COLOR_PATTERN (pattern)->color = swfdec_bits_get_rgba (bits);
    } else {
      SWFDEC_COLOR_PATTERN (pattern)->color = swfdec_bits_get_color (bits);
    }
    SWFDEC_LOG ("    color %08x", SWFDEC_COLOR_PATTERN (pattern)->color);
  } else if (fill_style_type == 0x10 || fill_style_type == 0x12) {
    pattern = g_object_new (SWFDEC_TYPE_GRADIENT_PATTERN, NULL);
    swfdec_bits_get_matrix (bits, &pattern->transform);
    if (rgba) {
      SWFDEC_GRADIENT_PATTERN (pattern)->gradient = swfdec_bits_get_gradient_rgba (bits);
    } else {
      SWFDEC_GRADIENT_PATTERN (pattern)->gradient = swfdec_bits_get_gradient (bits);
    }
    SWFDEC_GRADIENT_PATTERN (pattern)->radial = (fill_style_type == 0x12);
  } else if (fill_style_type >= 0x40 && fill_style_type <= 0x43) {
    guint fill_id = swfdec_bits_get_u16 (bits);
    SWFDEC_LOG ("   background fill id = %d (type 0x%02x)",
	fill_id, fill_style_type);
    if (fill_id == 65535) {
      /* FIXME: someone explain this magic fill id here */
      pattern = g_object_new (SWFDEC_TYPE_COLOR_PATTERN, NULL);
      SWFDEC_COLOR_PATTERN (pattern)->color = SWF_COLOR_COMBINE (0, 255, 255, 255);
      swfdec_bits_get_matrix (bits, &pattern->transform);
    } else {
      pattern = g_object_new (SWFDEC_TYPE_IMAGE_PATTERN, NULL);
      swfdec_bits_get_matrix (bits, &pattern->transform);
      SWFDEC_IMAGE_PATTERN (pattern)->image = swfdec_object_get (dec, fill_id);
      if (!SWFDEC_IS_IMAGE (SWFDEC_IMAGE_PATTERN (pattern)->image)) {
	g_object_unref (pattern);
	SWFDEC_ERROR ("could not find image with id %u for pattern", fill_id);
	return NULL;
      }
      if (fill_style_type == 0x40 || fill_style_type == 0x42) {
	SWFDEC_IMAGE_PATTERN (pattern)->extend = CAIRO_EXTEND_REPEAT;
      } else {
	SWFDEC_IMAGE_PATTERN (pattern)->extend = CAIRO_EXTEND_NONE;
      }
      if (fill_style_type == 0x40 || fill_style_type == 0x41) {
	SWFDEC_IMAGE_PATTERN (pattern)->filter = CAIRO_FILTER_BILINEAR;
      } else {
	SWFDEC_IMAGE_PATTERN (pattern)->filter = CAIRO_FILTER_NEAREST;
      }
    }
  } else {
    SWFDEC_ERROR ("unknown fill style type 0x%02x", fill_style_type);
    return NULL;
  }
  if (cairo_matrix_invert (&pattern->transform)) {
    SWFDEC_ERROR ("fill transform matrix not invertible, resetting");
    cairo_matrix_init_identity (&pattern->transform);
  }
  swfdec_bits_syncbits (bits);
  return pattern;
}

/**
 * swfdec_pattern_fill:
 * @pattern: a #SwfdecPattern
 * @cr: the context to fill
 * @trans: color transformation to apply before filling
 * @ratio: For morphshapes, the ratio to apply. Other objects should set this to 0.
 *
 * Fills the current path of @cr with the given @pattern.
 **/
void
swfdec_pattern_fill (SwfdecPattern *pattern, cairo_t *cr,
    const SwfdecColorTransform *trans, unsigned int ratio)
{
  SwfdecPatternClass *klass;

  g_return_if_fail (SWFDEC_IS_PATTERN (pattern));
  g_return_if_fail (cr != NULL);
  g_return_if_fail (trans != NULL);

  klass = SWFDEC_PATTERN_GET_CLASS (pattern);
  g_return_if_fail (klass->fill);

  klass->fill (pattern, cr, trans, ratio);
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
swfdec_pattern_new_color (swf_color color)
{
  SwfdecPattern *pattern = g_object_new (SWFDEC_TYPE_COLOR_PATTERN, NULL);

  SWFDEC_COLOR_PATTERN (pattern)->color = color;

  return pattern;
}

char *
swfdec_pattern_to_string (SwfdecPattern *pattern)
{
  g_return_val_if_fail (SWFDEC_IS_PATTERN (pattern), NULL);

  if (SWFDEC_IS_IMAGE_PATTERN (pattern)) {
    SwfdecImagePattern *image = SWFDEC_IMAGE_PATTERN (pattern);
    return g_strdup_printf ("image %u (%s, %s)", SWFDEC_OBJECT (image->image)->id,
	image->extend == CAIRO_EXTEND_REPEAT ? "repeat" : "no repeat",
	image->filter == CAIRO_FILTER_BILINEAR ? "bilinear" : "nearest");
  } else if (SWFDEC_IS_COLOR_PATTERN (pattern)) {
    return g_strdup_printf ("color #%08X", SWFDEC_COLOR_PATTERN (pattern)->color);
  } else if (SWFDEC_IS_GRADIENT_PATTERN (pattern)) {
    SwfdecGradientPattern *gradient = SWFDEC_GRADIENT_PATTERN (pattern);
    return g_strdup_printf ("%s gradient (%u colors)", gradient->radial ? "radial" : "linear",
	gradient->gradient->n_gradients);
  } else {
    return g_strdup_printf ("%s", G_OBJECT_TYPE_NAME (pattern));
  }
}
