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
#include "swfdec_as_context.h"
#include "swfdec_as_frame_internal.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_strings.h"
#include "swfdec_color.h"
#include "swfdec_debug.h"
#include "swfdec_image.h"
#include "swfdec_player_internal.h"
#include "swfdec_rectangle.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_resource.h"

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
  SwfdecAsValue val;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);

  if (!swfdec_as_context_try_use_mem (context, width * height * 4))
    return NULL;

  bitmap = g_object_new (SWFDEC_TYPE_BITMAP_DATA, "context", context, NULL);
  bitmap->surface = cairo_image_surface_create (
      transparent ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24, width, height);

  swfdec_as_object_get_variable (context->global, SWFDEC_AS_STR_flash, &val);
  if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
    swfdec_as_object_get_variable (SWFDEC_AS_VALUE_GET_OBJECT (&val), 
	SWFDEC_AS_STR_display, &val);
    if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
      swfdec_as_object_get_variable (SWFDEC_AS_VALUE_GET_OBJECT (&val), 
	  SWFDEC_AS_STR_BitmapData, &val);
      if (SWFDEC_AS_VALUE_IS_OBJECT (&val)) {
	swfdec_as_object_set_constructor (SWFDEC_AS_OBJECT (bitmap),
	    SWFDEC_AS_VALUE_GET_OBJECT (&val));
      }
    }
  }

  return bitmap;
}

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
      cairo_image_surface_get_format (isurface) != CAIRO_FORMAT_RGB24,
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
  SWFDEC_AS_VALUE_SET_INT (&args[3], cairo_image_surface_get_width (bitmap->surface));
  swfdec_as_object_create (SWFDEC_AS_FUNCTION (o), 4, args, ret);
  swfdec_as_context_run (cx);
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
	cairo_image_surface_get_format (bitmap->surface) != CAIRO_FORMAT_RGB24);
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
  color = *(SwfdecColor *) addr;
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
  old = *(SwfdecColor *) addr;
  old |= SWFDEC_COLOR_COMBINE (0xFF, 0xFF, 0xFF, 0);
  color = old & SWFDEC_COLOR_OPAQUE (color);
  *(SwfdecColor *) addr = SWFDEC_COLOR_MULTIPLY (color);
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

SWFDEC_AS_NATIVE (1100, 4, swfdec_bitmap_data_copyPixels)
void
swfdec_bitmap_data_copyPixels (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("BitmapData.copyPixels");
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
  SwfdecAsObject *o;
  cairo_t *cr;
  SwfdecColorTransform ctrans;
  SwfdecBitmapData *bitmap;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "o", &o);

  if (argc > 1) {
    SWFDEC_FIXME ("only the first argument to Bitmap.draw() is implemented");
  }
  swfdec_color_transform_init_identity (&ctrans);

  cr = cairo_create (bitmap->surface);
  /* FIXME: Do we have a better renderer? */
  swfdec_renderer_attach (SWFDEC_PLAYER (cx)->priv->renderer, cr);

  if (SWFDEC_IS_BITMAP_DATA (o)) {
    SwfdecBitmapData *src = SWFDEC_BITMAP_DATA (o);
    if (src->surface) {
      cairo_set_source_surface (cr, SWFDEC_BITMAP_DATA (o)->surface, 0, 0);
      cairo_paint (cr);
    }
  } else if (SWFDEC_IS_MOVIE (o)) {
    cairo_scale (cr, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR);
    swfdec_movie_render (SWFDEC_MOVIE (o), cr, &ctrans);
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
  color = *(SwfdecColor *) addr;
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
  *(SwfdecColor *) addr = SWFDEC_COLOR_MULTIPLY ((SwfdecColor) color);
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
      cairo_image_surface_get_format (bitmap->surface) != CAIRO_FORMAT_RGB24,
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
  gboolean transparent;
  guint color;

  if (!swfdec_as_context_is_constructing (cx))
    return;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_BITMAP_DATA, &bitmap, "ii|bi", 
      &w, &h, &transparent, &color);
  
  if (w > 2880 || w <= 0 || h > 2880 || h <= 0) {
    SWFDEC_FIXME ("the constructor should return undefined here");
    return;
  }
  if (argc < 3)
    transparent = TRUE;

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
