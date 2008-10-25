/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_test_image.h"
#include "swfdec_test_function.h"
#include "swfdec_test_utils.h"

#define SWFDEC_TEST_IMAGE_IS_VALID(image) ((image)->surface && \
    cairo_surface_status ((image)->surface) == CAIRO_STATUS_SUCCESS)

SwfdecTestImage *
swfdec_test_image_new (SwfdecAsContext *context, guint width, guint height)
{
  SwfdecTestImage *ret;
  SwfdecAsObject *object;

  ret = g_object_new (SWFDEC_TYPE_TEST_IMAGE, "context", context, NULL);
  ret->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

  object = swfdec_as_object_new (context, NULL);
  swfdec_as_object_set_constructor_by_name (object,
      swfdec_as_context_get_string (context, "Image"), NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (ret));

  return ret;
}

/*** SWFDEC_TEST_IMAGE ***/

G_DEFINE_TYPE (SwfdecTestImage, swfdec_test_image, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_test_image_dispose (GObject *object)
{
  SwfdecTestImage *image = SWFDEC_TEST_IMAGE (object);

  if (image->surface) {
    cairo_surface_destroy (image->surface);
    image->surface = NULL;
  }

  G_OBJECT_CLASS (swfdec_test_image_parent_class)->dispose (object);
}

static void
swfdec_test_image_class_init (SwfdecTestImageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_test_image_dispose;
}

static void
swfdec_test_image_init (SwfdecTestImage *this)
{
}

/*** AS CODE ***/

/* Compare two buffers, returning the number of pixels that are
 * different and the maximum difference of any single color channel in
 * result_ret.
 *
 * This function should be rewritten to compare all formats supported by
 * cairo_format_t instead of taking a mask as a parameter.
 */
static gboolean
buffer_diff_core (unsigned char *buf_a,
		  unsigned char *buf_b,
		  unsigned char *buf_diff,
		  int		width,
		  int		height,
		  int		stride_a,
		  int		stride_b,
		  int		stride_diff)
{
    int x, y;
    gboolean result = TRUE;
    guint32 *row_a, *row_b, *row;

    for (y = 0; y < height; y++) {
	row_a = (guint32 *) (buf_a + y * stride_a);
	row_b = (guint32 *) (buf_b + y * stride_b);
	row = (guint32 *) (buf_diff + y * stride_diff);
	for (x = 0; x < width; x++) {
	    /* check if the pixels are the same */
	    if (row_a[x] != row_b[x]) {
		int channel;
		static const unsigned int threshold = 3;
		guint32 diff_pixel = 0;

		/* calculate a difference value for all 4 channels */
		for (channel = 0; channel < 4; channel++) {
		    int value_a = (row_a[x] >> (channel*8)) & 0xff;
		    int value_b = (row_b[x] >> (channel*8)) & 0xff;
		    unsigned int diff;
		    diff = ABS (value_a - value_b);
		    if (diff <= threshold)
		      continue;
		    diff *= 4;  /* emphasize */
		    diff += 128; /* make sure it's visible */
		    if (diff > 255)
		        diff = 255;
		    diff_pixel |= diff << (channel*8);
		}

		row[x] = diff_pixel;
		if (diff_pixel)
		  result = FALSE;
	    } else {
		row[x] = 0;
	    }
	    row[x] |= 0xff000000; /* Set ALPHA to 100% (opaque) */
	}
    }
    return result;
}

SWFDEC_TEST_FUNCTION ("Image_compare", swfdec_test_image_compare, 0)
void
swfdec_test_image_compare (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestImage *image, *compare, *diff;
  SwfdecAsObject *o;
  int w, h;
  
  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_IMAGE, &image, "o", &o);

  if (!SWFDEC_IS_TEST_IMAGE (o->relay))
    return;
  compare = SWFDEC_TEST_IMAGE (o->relay);

  SWFDEC_AS_VALUE_SET_OBJECT (retval, swfdec_as_relay_get_as_object (SWFDEC_AS_RELAY (image)));
  if (!SWFDEC_TEST_IMAGE_IS_VALID (image) ||
      !SWFDEC_TEST_IMAGE_IS_VALID (compare))
    return;

  g_assert (cairo_surface_get_type (image->surface) == CAIRO_SURFACE_TYPE_IMAGE);
  g_assert (cairo_surface_get_type (compare->surface) == CAIRO_SURFACE_TYPE_IMAGE);

  w = cairo_image_surface_get_width (image->surface);
  if (w != cairo_image_surface_get_width (compare->surface))
    return;
  h = cairo_image_surface_get_height (image->surface);
  if (h != cairo_image_surface_get_height (compare->surface))
    return;
  diff = SWFDEC_TEST_IMAGE (swfdec_test_image_new (cx, w, h));

  if (!buffer_diff_core (cairo_image_surface_get_data (image->surface), 
	cairo_image_surface_get_data (compare->surface), 
	cairo_image_surface_get_data (diff->surface), 
	w, h, 
	cairo_image_surface_get_stride (image->surface),
	cairo_image_surface_get_stride (compare->surface),
	cairo_image_surface_get_stride (diff->surface)) != 0) {
    SWFDEC_AS_VALUE_SET_OBJECT (retval, SWFDEC_AS_OBJECT (diff));
  } else {
    SWFDEC_AS_VALUE_SET_NULL (retval);
  }
}

SWFDEC_TEST_FUNCTION ("Image_save", swfdec_test_image_save, 0)
void
swfdec_test_image_save (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestImage *image;
  const char *filename;
  cairo_status_t status;
  
  SWFDEC_AS_CHECK (SWFDEC_TYPE_TEST_IMAGE, &image, "s", &filename);

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, FALSE);
  if (!SWFDEC_TEST_IMAGE_IS_VALID (image))
    return;

  status = cairo_surface_write_to_png (image->surface, filename);
  if (status != CAIRO_STATUS_SUCCESS) {
    swfdec_test_throw (cx, "Couldn't save to %s: %s", filename, cairo_status_to_string (status));
  }

  SWFDEC_AS_VALUE_SET_BOOLEAN (retval, TRUE);
}

SWFDEC_TEST_FUNCTION ("Image", swfdec_test_image_create, NULL)
void
swfdec_test_image_create (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *retval)
{
  SwfdecTestImage *image;
  const char *filename;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  SWFDEC_AS_CHECK (0, NULL, "s", &filename);

  image = g_object_new (SWFDEC_TYPE_TEST_IMAGE, "context", cx, NULL);
  image->surface = cairo_image_surface_create_from_png (filename);

  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (image));
  SWFDEC_AS_VALUE_SET_OBJECT (retval, object);
}

