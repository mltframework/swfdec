/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
 *		 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_bitmap_data.h"

#include <math.h>

#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_color.h"
#include "swfdec_color_transform_as.h"
#include "swfdec_debug.h"
#include "swfdec_image.h"
#include "swfdec_player_internal.h"
#include "swfdec_rectangle.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_resource.h"
#include "swfdec_utils.h"

enum {
  INVALIDATE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL];
G_DEFINE_TYPE (SwfdecBitmapData, swfdec_bitmap_data, SWFDEC_TYPE_AS_OBJECT)

static void
swfdec_bitmap_data_invalidate (SwfdecBitmapData *bitmap, guint x, guint y, guint w, guint h)
{
  SwfdecRectangle rect = { x, y, w, h };

  g_return_if_fail (w > 0);
  g_return_if_fail (h > 0);

  g_signal_emit (bitmap, signals[INVALIDATE], 0, &rect);
}

static void
swfdec_bitmap_data_clear (SwfdecBitmapData *bitmap)
{
  int w, h;

  if (bitmap->surface == NULL)
    return;

  w = cairo_image_surface_get_width (bitmap->surface);
  h = cairo_image_surface_get_height (bitmap->surface);
  swfdec_bitmap_data_invalidate (bitmap, 0, 0, w, h);
  swfdec_as_context_unuse_mem (swfdec_gc_object_get_context (bitmap), 4 * w * h);
  cairo_surface_destroy (bitmap->surface);
  bitmap->surface = NULL;
}

static void
swfdec_bitmap_data_dispose (GObject *object)
{
  SwfdecBitmapData *bitmap = SWFDEC_BITMAP_DATA (object);

  swfdec_bitmap_data_clear (bitmap);

  G_OBJECT_CLASS (swfdec_bitmap_data_parent_class)->dispose (object);
}

static void
swfdec_bitmap_data_class_init (SwfdecBitmapDataClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_bitmap_data_dispose;

  signals[INVALIDATE] = g_signal_new ("invalidate", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__BOXED,
      G_TYPE_NONE, 1, SWFDEC_TYPE_RECTANGLE);
}

static void
swfdec_bitmap_data_init (SwfdecBitmapData *transform)
{
}

SwfdecBitmapData *
swfdec_bitmap_data_new (SwfdecAsContext *context, gboolean transparent, guint width, guint height)
{
  SwfdecBitmapData *bitmap;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (!swfdec_as_context_try_use_mem (context, width * height * 4))
    return NULL;

  bitmap = g_object_new (SWFDEC_TYPE_BITMAP_DATA, "context", context, NULL);
  bitmap->surface = cairo_image_surface_create (
      transparent ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, width, height);

  swfdec_as_object_set_constructor_by_name (SWFDEC_AS_OBJECT (bitmap),
      SWFDEC_AS_STR_flash, SWFDEC_AS_STR_display, SWFDEC_AS_STR_BitmapData, NULL);

  return bitmap;
}

#define swfdec_surface_has_alpha(surface) (cairo_surface_get_content (surface) & CAIRO_CONTENT_ALPHA)

SWFDEC_AS_NATIVE (1100, 40, swfdec_bitmap_data_loadBitmap)
void
swfdec_bitmap_data_loadBitmap (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  SwfdecImage *image;
  const char *name;
  cairo_surface_t *isurface;
  cairo_t *cr;

  SWFDEC_AS_CHECK (0, NULL, "s", &name);

  g_assert (SWFDEC_IS_MOVIE (cx->frame->target));
  image = swfdec_resource_get_export (SWFDEC_MOVIE (cx->frame->target)->resource, name);
  if (!SWFDEC_IS_IMAGE (image)) {
    SWFDEC_ERROR ("loadBitmap cannot find image with name %s", name);
    return;
  }

  /* FIXME: improve this to not create an image if there is one cached */
  isurface = swfdec_image_create_surface (image, NULL);
  if (isurface == NULL)
    return;

  /* FIXME: use image directly instead of doing a copy and then deleting it */
  bitmap = swfdec_bitmap_data_new (cx, 
      swfdec_surface_has_alpha (isurface),
      cairo_image_surface_get_width (isurface),
      cairo_image_surface_get_height (isurface));
  if (bitmap == NULL)
    return;

  cr = cairo_create (bitmap->surface);
  cairo_set_source_surface (cr, isurface, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (isurface);
  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (bitmap));
}

// properties
SWFDEC_AS_NATIVE (1100, 100, swfdec_bitmap_data_get_width)
void
swfdec_bitmap_data_get_width (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  SWFDEC_AS_VALUE_SET_INT (ret, bitmap->surface ? 
      cairo_image_surface_get_width (bitmap->surface) : -1);
}

SWFDEC_AS_NATIVE (1100, 101, swfdec_bitmap_data_set_width)
void
swfdec_bitmap_data_set_width (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.width (set)");
}

SWFDEC_AS_NATIVE (1100, 102, swfdec_bitmap_data_get_height)
void
swfdec_bitmap_data_get_height (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  SWFDEC_AS_VALUE_SET_INT (ret, bitmap->surface ? 
      cairo_image_surface_get_height (bitmap->surface) : -1);
}

SWFDEC_AS_NATIVE (1100, 103, swfdec_bitmap_data_set_height)
void
swfdec_bitmap_data_set_height (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.height (set)");
}

SWFDEC_AS_NATIVE (1100, 104, swfdec_bitmap_data_get_rectangle)
void
swfdec_bitmap_data_get_rectangle (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  SwfdecAsObject *o;
  SwfdecAsValue args[4];

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  SWFDEC_AS_VALUE_SET_INT (ret, -1);
  if (bitmap->surface == NULL)
    return;
  
  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_flash, args);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (args))
    return;
  o = SWFDEC_AS_VALUE_GET_OBJECT (args);
  swfdec_as_object_get_variable (o, SWFDEC_AS_STR_geom, args);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (args))
    return;
  o = SWFDEC_AS_VALUE_GET_OBJECT (args);
  swfdec_as_object_get_variable (o, SWFDEC_AS_STR_Rectangle, args);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (args))
    return;
  o = SWFDEC_AS_VALUE_GET_OBJECT (args);
  if (!SWFDEC_IS_AS_FUNCTION (o))
    return;

  SWFDEC_AS_VALUE_SET_INT (&args[0], 0);
  SWFDEC_AS_VALUE_SET_INT (&args[1], 0);
  SWFDEC_AS_VALUE_SET_INT (&args[2], cairo_image_surface_get_width (bitmap->surface));
  SWFDEC_AS_VALUE_SET_INT (&args[3], cairo_image_surface_get_height (bitmap->surface));
  swfdec_as_object_create (SWFDEC_AS_FUNCTION (o), 4, args, ret);
}

SWFDEC_AS_NATIVE (1100, 105, swfdec_bitmap_data_set_rectangle)
void
swfdec_bitmap_data_set_rectangle (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.rectangle (set)");
}

SWFDEC_AS_NATIVE (1100, 106, swfdec_bitmap_data_get_transparent)
void
swfdec_bitmap_data_get_transparent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  if (bitmap->surface) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (ret, 
	swfdec_surface_has_alpha (bitmap->surface) ? TRUE : FALSE);
  } else {
    SWFDEC_AS_VALUE_SET_INT (ret, -1);
  }
}

SWFDEC_AS_NATIVE (1100, 107, swfdec_bitmap_data_set_transparent)
void
swfdec_bitmap_data_set_transparent (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.transparent (set)");
}

#define SWFDEC_COLOR_MULTIPLY(color) SWFDEC_COLOR_COMBINE ( \
    (SWFDEC_COLOR_ALPHA (color) * SWFDEC_COLOR_RED (color) + 128) / 255, \
    (SWFDEC_COLOR_ALPHA (color) * SWFDEC_COLOR_GREEN (color) + 128) / 255, \
    (SWFDEC_COLOR_ALPHA (color) * SWFDEC_COLOR_BLUE (color) + 128) / 255, \
    SWFDEC_COLOR_ALPHA (color))

/* FIXME: This algorithm rounds wrong, no idea how though */
#define SWFDEC_COLOR_UNMULTIPLY(color) (SWFDEC_COLOR_ALPHA (color) ? (\
    SWFDEC_COLOR_ALPHA (color) == 0xFF ? color : SWFDEC_COLOR_COMBINE ( \
    (SWFDEC_COLOR_RED (color) * 255 + SWFDEC_COLOR_ALPHA (color) / 2) / SWFDEC_COLOR_ALPHA (color), \
    (SWFDEC_COLOR_GREEN (color) * 255 + SWFDEC_COLOR_ALPHA (color) / 2) / SWFDEC_COLOR_ALPHA (color), \
    (SWFDEC_COLOR_BLUE (color) * 255 + SWFDEC_COLOR_ALPHA (color) / 2) / SWFDEC_COLOR_ALPHA (color), \
    SWFDEC_COLOR_ALPHA (color))) : 0)

SWFDEC_AS_NATIVE (1100, 1, swfdec_bitmap_data_getPixel)
void
swfdec_bitmap_data_getPixel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  guint x, y, color;
  guint8 *addr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "ii", &x, &y);

  if (bitmap->surface == NULL ||
      x >= (guint) cairo_image_surface_get_width (bitmap->surface) ||
      y >= (guint) cairo_image_surface_get_height (bitmap->surface))
    return;

  addr = cairo_image_surface_get_data (bitmap->surface);
  addr += cairo_image_surface_get_stride (bitmap->surface) * y;
  addr += 4 * x;
  color = *(SwfdecColor *) (gpointer) addr;
  color = SWFDEC_COLOR_UNMULTIPLY (color);
  color &= SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0);
  SWFDEC_AS_VALUE_SET_INT (ret, color);
}

SWFDEC_AS_NATIVE (1100, 2, swfdec_bitmap_data_setPixel)
void
swfdec_bitmap_data_setPixel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  guint x, y, color;
  SwfdecColor old;
  guint8 *addr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "iii", &x, &y, &color);

  if (bitmap->surface == NULL ||
      x >= (guint) cairo_image_surface_get_width (bitmap->surface) ||
      y >= (guint) cairo_image_surface_get_height (bitmap->surface))
    return;

  addr = cairo_image_surface_get_data (bitmap->surface);
  addr += cairo_image_surface_get_stride (bitmap->surface) * y;
  addr += 4 * x;
  old = *(SwfdecColor *) (gpointer) addr;
  old |= SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0);
  color = old & SWFDEC_COLOR_OPAQUE (color);
  *(SwfdecColor *) (gpointer) addr = SWFDEC_COLOR_MULTIPLY (color);
  cairo_surface_mark_dirty_rectangle (bitmap->surface, x, y, 1, 1);
  swfdec_bitmap_data_invalidate (bitmap, x, y, 1, 1);
}

SWFDEC_AS_NATIVE (1100, 3, swfdec_bitmap_data_fillRect)
void
swfdec_bitmap_data_fillRect (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.fillRect");
}

static gboolean
swfdec_rectangle_from_as_object (SwfdecRectangle *rect, SwfdecAsObject *object)
{
  SwfdecAsValue *val;
  SwfdecAsContext *cx = swfdec_gc_object_get_context (object);

  /* FIXME: This function is untested */
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_x);
  if (val)
    rect->x = swfdec_as_value_to_integer (cx, val);
  else
    rect->x = 0;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_y);
  if (val)
    rect->y = swfdec_as_value_to_integer (cx, val);
  else
    rect->y = 0;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_width);
  if (val)
    rect->width = swfdec_as_value_to_integer (cx, val);
  else
    rect->width = 0;
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_height);
  if (val)
    rect->height = swfdec_as_value_to_integer (cx, val);
  else
    rect->height = 0;
  return rect->width > 0 && rect->height > 0;
}

static void
swfdec_point_from_as_object (int *x, int *y, SwfdecAsObject *object)
{
  SwfdecAsValue *val;
  SwfdecAsContext *cx = swfdec_gc_object_get_context (object);

  /* FIXME: This function is untested */
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_x);
  *x = swfdec_as_value_to_integer (cx, val);
  val = swfdec_as_object_peek_variable (object, SWFDEC_AS_STR_y);
  *y = swfdec_as_value_to_integer (cx, val);
}

SWFDEC_AS_NATIVE (1100, 4, swfdec_bitmap_data_copyPixels)
void
swfdec_bitmap_data_copyPixels (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap, *source, *alpha = NULL;
  SwfdecAsObject *recto = NULL, *pt, *apt = NULL, *so, *ao = NULL;
  SwfdecRectangle rect;
  gboolean copy_alpha = FALSE;
  cairo_t *cr;
  int x, y;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "ooo|oob", &so, &recto, &pt,
      &ao, &apt, &copy_alpha);

  if (bitmap->surface == NULL ||
      !SWFDEC_IS_BITMAP_DATA (so) ||
      (source = SWFDEC_BITMAP_DATA (so))->surface == NULL ||
      (ao != NULL && (!SWFDEC_IS_BITMAP_DATA (ao) || 
		    (alpha = SWFDEC_BITMAP_DATA (ao))->surface == NULL)) ||
      !swfdec_rectangle_from_as_object (&rect, recto))
    return;

  x = rect.x;
  y = rect.y;
  swfdec_point_from_as_object (&rect.x, &rect.y, pt);
  cr = cairo_create (bitmap->surface);
  if (bitmap == source) {
    cairo_surface_t *copy = cairo_surface_create_similar (source->surface,
	cairo_surface_get_content (source->surface),
	rect.width, rect.height);
    cairo_t *cr2 = cairo_create (copy);
    cairo_set_source_surface (cr2, source->surface, x, y);
    cairo_paint (cr2);
    cairo_destroy (cr2);
    cairo_set_source_surface (cr, copy, rect.x, rect.y);
    cairo_surface_destroy (copy);
  } else {
    cairo_set_source_surface (cr, source->surface, rect.x - x, rect.y - y);
  }

  if (swfdec_surface_has_alpha (bitmap->surface) && !copy_alpha) {
    cairo_set_operator (cr, CAIRO_OPERATOR_SOURCE);
  }

  cairo_rectangle (cr, rect.x, rect.y, rect.width, rect.height);
  if (alpha) {
    cairo_surface_t *mask = cairo_surface_create_similar (alpha->surface,
	CAIRO_CONTENT_COLOR_ALPHA, rect.width, rect.height);
    cairo_t *cr2 = cairo_create (mask);

    cairo_surface_set_device_offset (mask, -rect.x, -rect.y);
    cairo_set_source (cr2, cairo_get_source (cr));
    if (apt) {
      swfdec_point_from_as_object (&x, &y, apt);
    } else {
      x = y = 0;
    }
    cairo_mask_surface (cr2, alpha->surface, rect.x - x, rect.y - y);
    cairo_destroy (cr2);
    cairo_set_source_surface (cr, mask, 0, 0);
    cairo_surface_destroy (mask);
  }
  cairo_fill (cr);
  cairo_destroy (cr);
  cairo_surface_mark_dirty_rectangle (bitmap->surface, rect.x, rect.y, 
      rect.width, rect.height);
}

SWFDEC_AS_NATIVE (1100, 5, swfdec_bitmap_data_applyFilter)
void
swfdec_bitmap_data_applyFilter (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.applyFilter");
}

SWFDEC_AS_NATIVE (1100, 6, swfdec_bitmap_data_scroll)
void
swfdec_bitmap_data_scroll (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.scroll");
}

SWFDEC_AS_NATIVE (1100, 7, swfdec_bitmap_data_threshold)
void
swfdec_bitmap_data_threshold (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.threshold");
}

SWFDEC_AS_NATIVE (1100, 8, swfdec_bitmap_data_draw)
void
swfdec_bitmap_data_draw (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsObject *o, *matrix = NULL, *trans = NULL;
  cairo_t *cr;
  SwfdecColorTransform ctrans;
  SwfdecBitmapData *bitmap;
  SwfdecRenderer *renderer;
  cairo_matrix_t mat;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "o|OO", &o, &matrix, &trans);

  if (bitmap->surface == NULL)
    return;

  if (argc >= 2) {
    if (matrix == NULL || !swfdec_matrix_from_as_object (&mat, matrix))
      return;
  } else {
    cairo_matrix_init_identity (&mat);
  }
  if (SWFDEC_IS_COLOR_TRANSFORM_AS (trans)) {
    swfdec_color_transform_get_transform (SWFDEC_COLOR_TRANSFORM_AS (trans), &ctrans);
  } else {
    swfdec_color_transform_init_identity (&ctrans);
  }

  if (argc > 3) {
    SWFDEC_FIXME ("only the first 3 arguments to Bitmap.draw() are implemented");
  }

  cr = cairo_create (bitmap->surface);
  /* FIXME: Do we have a better renderer? */
  renderer = SWFDEC_PLAYER (cx)->priv->renderer;
  swfdec_renderer_attach (renderer, cr);
  cairo_transform (cr, &mat);

  if (SWFDEC_IS_BITMAP_DATA (o)) {
    SwfdecBitmapData *src = SWFDEC_BITMAP_DATA (o);
    if (src->surface) {
      if (swfdec_color_transform_is_identity (&ctrans)) {
	cairo_set_source_surface (cr, SWFDEC_BITMAP_DATA (o)->surface, 0, 0);
      } else {
	cairo_surface_t *transformed = swfdec_renderer_transform (renderer,
	    SWFDEC_BITMAP_DATA (o)->surface, &ctrans);
	SWFDEC_FIXME ("unmodified pixels will be treated as -1, not as 0 as in our "
	    "transform code, but we don't know if a pixel is unmodified.");
	cairo_set_source_surface (cr, transformed, 0, 0);
	cairo_surface_destroy (transformed);
      }
      cairo_paint (cr);
    }
  } else if (SWFDEC_IS_MOVIE (o)) {
    SwfdecMovie *movie = SWFDEC_MOVIE (o);
    swfdec_movie_update (movie);
    cairo_scale (cr, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR);
    cairo_transform (cr, &movie->inverse_matrix);
    swfdec_movie_render (movie, cr, &ctrans);
  } else {
    SWFDEC_FIXME ("BitmapData.draw() with a %s?", G_OBJECT_TYPE_NAME (o));
  }

  cairo_destroy (cr);
}

SWFDEC_AS_NATIVE (1100, 9, swfdec_bitmap_data_pixelDissolve)
void
swfdec_bitmap_data_pixelDissolve (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.pixelDissolve");
}

SWFDEC_AS_NATIVE (1100, 10, swfdec_bitmap_data_getPixel32)
void
swfdec_bitmap_data_getPixel32 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  guint x, y, color;
  guint8 *addr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "ii", &x, &y);

  if (bitmap->surface == NULL ||
      x >= (guint) cairo_image_surface_get_width (bitmap->surface) ||
      y >= (guint) cairo_image_surface_get_height (bitmap->surface))
    return;

  addr = cairo_image_surface_get_data (bitmap->surface);
  addr += cairo_image_surface_get_stride (bitmap->surface) * y;
  addr += 4 * x;
  color = *(SwfdecColor *) (gpointer) addr;
  color = SWFDEC_COLOR_UNMULTIPLY (color);
  SWFDEC_AS_VALUE_SET_INT (ret, color);
}

SWFDEC_AS_NATIVE (1100, 11, swfdec_bitmap_data_setPixel32)
void
swfdec_bitmap_data_setPixel32 (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  guint x, y, color;
  guint8 *addr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "iii", &x, &y, &color);

  if (bitmap->surface == NULL ||
      x >= (guint) cairo_image_surface_get_width (bitmap->surface) ||
      y >= (guint) cairo_image_surface_get_height (bitmap->surface))
    return;

  addr = cairo_image_surface_get_data (bitmap->surface);
  addr += cairo_image_surface_get_stride (bitmap->surface) * y;
  addr += 4 * x;
  if (swfdec_surface_has_alpha (bitmap->surface)) {
    *(SwfdecColor *) (gpointer) addr = SWFDEC_COLOR_MULTIPLY ((SwfdecColor) color);
  } else {
    *(SwfdecColor *) (gpointer) addr = SWFDEC_COLOR_OPAQUE ((SwfdecColor) color);
  }
  cairo_surface_mark_dirty_rectangle (bitmap->surface, x, y, 1, 1);
  swfdec_bitmap_data_invalidate (bitmap, x, y, 1, 1);
}

SWFDEC_AS_NATIVE (1100, 12, swfdec_bitmap_data_floodFill)
void
swfdec_bitmap_data_floodFill (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.floodFill");
}

SWFDEC_AS_NATIVE (1100, 13, swfdec_bitmap_data_getColorBoundsRect)
void
swfdec_bitmap_data_getColorBoundsRect (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.getColorBoundsRect");
}

SWFDEC_AS_NATIVE (1100, 14, swfdec_bitmap_data_perlinNoise)
void
swfdec_bitmap_data_perlinNoise (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.perlinNoise");
}

SWFDEC_AS_NATIVE (1100, 15, swfdec_bitmap_data_colorTransform)
void
swfdec_bitmap_data_colorTransform (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.colorTransform");
}

SWFDEC_AS_NATIVE (1100, 16, swfdec_bitmap_data_hitTest)
void
swfdec_bitmap_data_hitTest (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.hitTest");
}

SWFDEC_AS_NATIVE (1100, 17, swfdec_bitmap_data_paletteMap)
void
swfdec_bitmap_data_paletteMap (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.paletteMap");
}

SWFDEC_AS_NATIVE (1100, 18, swfdec_bitmap_data_merge)
void
swfdec_bitmap_data_merge (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.merge");
}

SWFDEC_AS_NATIVE (1100, 19, swfdec_bitmap_data_noise)
void
swfdec_bitmap_data_noise (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.noise");
}

SWFDEC_AS_NATIVE (1100, 20, swfdec_bitmap_data_copyChannel)
void
swfdec_bitmap_data_copyChannel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.copyChannel");
}

SWFDEC_AS_NATIVE (1100, 21, swfdec_bitmap_data_clone)
void
swfdec_bitmap_data_clone (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap, *clone;
  cairo_t *cr;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  if (bitmap->surface == NULL)
    return;

  clone = swfdec_bitmap_data_new (cx,
      swfdec_surface_has_alpha (bitmap->surface),
      cairo_image_surface_get_width (bitmap->surface),
      cairo_image_surface_get_height (bitmap->surface));
  if (clone == NULL)
    return;

  cr = cairo_create (clone->surface);
  cairo_set_source_surface (cr, bitmap->surface, 0, 0);
  cairo_paint (cr);
  cairo_destroy (cr);
  SWFDEC_AS_VALUE_SET_OBJECT (ret, SWFDEC_AS_OBJECT (clone));
}

SWFDEC_AS_NATIVE (1100, 22, swfdec_bitmap_data_do_dispose)
void
swfdec_bitmap_data_do_dispose (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "");

  swfdec_bitmap_data_clear (bitmap);
}

SWFDEC_AS_NATIVE (1100, 23, swfdec_bitmap_data_generateFilterRect)
void
swfdec_bitmap_data_generateFilterRect (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.generateFilterRect");
}

SWFDEC_AS_NATIVE (1100, 24, swfdec_bitmap_data_compare)
void
swfdec_bitmap_data_compare (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.compare");
}

SWFDEC_AS_CONSTRUCTOR (1100, 0, swfdec_bitmap_data_construct, swfdec_bitmap_data_get_type)
void
swfdec_bitmap_data_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecBitmapData *bitmap;
  int w, h;
  gboolean transparent = TRUE;
  guint color = 0;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "ii|bi", 
      &w, &h, &transparent, &color);
  
  if (w > 2880 || w <= 0 || h > 2880 || h <= 0) {
    SWFDEC_FIXME ("the constructor should return undefined here");
    return;
  }

  if (!swfdec_as_context_try_use_mem (cx, w * h * 4))
    return;
  bitmap->surface = cairo_image_surface_create (
      transparent ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, w, h);

  if (color) {
    cairo_t *cr = cairo_create (bitmap->surface);
    swfdec_color_set_source (cr, transparent ? color : SWFDEC_COLOR_OPAQUE (color));
    cairo_paint (cr);
    cairo_destroy (cr);
  }
}
