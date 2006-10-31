/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#ifdef CAIRO_HAS_SVG_SURFACE
#  include <cairo-svg.h>
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
#  include <cairo-pdf.h>
#endif
#include <libswfdec/swfdec.h>
#include <libswfdec/color.h>
#include <libswfdec/swfdec_button.h>
#include <libswfdec/swfdec_graphic.h>
#include <libswfdec/swfdec_player_internal.h>
#include <libswfdec/swfdec_root_movie.h>
#include <libswfdec/swfdec_sound.h>
#include <libswfdec/swfdec_sprite.h>
#include <libswfdec/swfdec_swf_decoder.h>

static SwfdecBuffer *
encode_wav (SwfdecBuffer *buffer)
{
  SwfdecBuffer *wav = swfdec_buffer_new_and_alloc (buffer->length + 44);
  unsigned char *data;
  guint i;

  data = wav->data;
  memmove (data, "RIFF", 4);
  data += 4;
  *(gint32 *) data = GUINT32_TO_LE (buffer->length + 36);
  data += 4;
  memmove (data, "WAVEfmt ", 8);
  data += 8;
  *(gint32 *) data = GUINT32_TO_LE (16);
  data += 4;
  *data++ = 1;
  *data++ = 0;
  *data++ = 2;
  *data++ = 0;
  *(gint32 *) data = GUINT32_TO_LE (44100);
  data += 4;
  *(gint32 *) data = GUINT32_TO_LE (44100 * 4);
  data += 4;
  *data++ = 4;
  *data++ = 0;
  *data++ = 16;
  *data++ = 0;
  memmove (data, "data", 4);
  data += 4;
  *(gint32 *) data = GUINT32_TO_LE (buffer->length);
  data += 4;
  for (i = 0; i < buffer->length; i += 2) {
    *(gint16 *) (data + i) = GINT16_TO_LE (*(gint16* )(buffer->data + i));
  }
  return wav;
}

static gboolean
export_sound (SwfdecSound *sound, const char *filename)
{
  GError *error = NULL;
  SwfdecBuffer *wav;

  if (sound->decoded == NULL) {
    g_printerr ("not a sound event. Streams are not supported yet.");
    return FALSE;
  }
  wav = encode_wav (sound->decoded);
  if (!g_file_set_contents (filename, (char *) wav->data, 
	wav->length, &error)) {
    g_printerr ("Couldn't save sound to file \"%s\": %s\n", filename, error->message);
    swfdec_buffer_unref (wav);
    g_error_free (error);
    return FALSE;
  }
  swfdec_buffer_unref (wav);
  return TRUE;
}

static cairo_surface_t *
surface_create_for_filename (const char *filename, int width, int height)
{
  guint len = strlen (filename);
  cairo_surface_t *surface;
  if (FALSE) {
#ifdef CAIRO_HAS_PDF_SURFACE
  } else if (len >= 3 && g_ascii_strcasecmp (filename + len - 3, "pdf") == 0) {
    surface = cairo_pdf_surface_create (filename, width, height);
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  } else if (len >= 3 && g_ascii_strcasecmp (filename + len - 3, "svg") == 0) {
    surface = cairo_svg_surface_create (filename, width, height);
#endif
  } else {
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  }
  return surface;
}

static gboolean
surface_destroy_for_type (cairo_surface_t *surface, const char *filename)
{
  if (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE) {
    cairo_status_t status = cairo_surface_write_to_png (surface, filename);
    if (status != CAIRO_STATUS_SUCCESS) {
      g_printerr ("Error saving file: %s\n", cairo_status_to_string (status));
      cairo_surface_destroy (surface);
      return FALSE;
    }
  }
  cairo_surface_destroy (surface);
  return TRUE;
}

static gboolean
export_graphic (SwfdecGraphic *graphic, const char *filename)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  guint width, height;
  const SwfdecColorTransform trans = { { 1, 1, 1, 1 }, { 0, 0, 0, 0 } };

  if (SWFDEC_IS_SPRITE (graphic)) {
    g_printerr ("Sprites can not be exported");
    return FALSE;
  }
  if (SWFDEC_IS_BUTTON (graphic)) {
    g_printerr ("Buttons can not be exported");
    return FALSE;
  }
  width = ceil (graphic->extents.x1 / SWFDEC_SCALE_FACTOR) 
    - floor (graphic->extents.x0 / SWFDEC_SCALE_FACTOR);
  height = ceil (graphic->extents.y1 / SWFDEC_SCALE_FACTOR) 
    - floor (graphic->extents.y0 / SWFDEC_SCALE_FACTOR);
  surface = surface_create_for_filename (filename, width, height);
  cr = cairo_create (surface);
  cairo_translate (cr, - floor (graphic->extents.x0 / SWFDEC_SCALE_FACTOR),
    - floor (graphic->extents.y0 / SWFDEC_SCALE_FACTOR));
  cairo_scale (cr, 1 / SWFDEC_SCALE_FACTOR, 1 / SWFDEC_SCALE_FACTOR);
  swfdec_graphic_render (graphic, cr, &trans, &graphic->extents, TRUE);
  cairo_show_page (cr);
  cairo_destroy (cr);
  return surface_destroy_for_type (surface, filename);
}

static void
usage (const char *app)
{
  g_print ("usage: %s SWFFILE ID OUTFILE\n\n", app);
}

int
main (int argc, char *argv[])
{
  SwfdecCharacter *character;
  int ret = 0;
  SwfdecPlayer *player;
  GError *error = NULL;
  guint id;

  swfdec_init ();

  if (argc != 4) {
    usage (argv[0]);
    return 0;
  }

  player = swfdec_player_new_from_file (argv[1], &error);
  if (player == NULL) {
    g_printerr ("Couldn't open file \"%s\": %s\n", argv[1], error->message);
    g_error_free (error);
    return 1;
  }
  if (swfdec_player_get_rate (player) == 0) {
    g_printerr ("Error parsing file \"%s\"\n", argv[1]);
    g_object_unref (player);
    player = NULL;
    return 1;
  }
  id = strtoul (argv[2], NULL, 0);
  character = swfdec_swf_decoder_get_character (
      SWFDEC_SWF_DECODER (SWFDEC_ROOT_MOVIE (player->roots->data)->decoder),
      id);
  if (SWFDEC_IS_SOUND (character)) {
    if (!export_sound (SWFDEC_SOUND (character), argv[3]))
      ret = 1;
  } else if (SWFDEC_IS_GRAPHIC (character)) {
    if (!export_graphic (SWFDEC_GRAPHIC (character), argv[3]))
      ret = 1;
  } else {
    g_printerr ("id %u does not specify an exportable object", id);
    ret = 1;
  }

  g_object_unref (player);
  player = NULL;

  return ret;
}

