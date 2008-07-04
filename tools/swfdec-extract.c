/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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
#include <swfdec/swfdec.h>
#include <swfdec/swfdec_audio_stream.h>
#include <swfdec/swfdec_button.h>
#include <swfdec/swfdec_graphic.h>
#include <swfdec/swfdec_image.h>
#include <swfdec/swfdec_player_internal.h>
#include <swfdec/swfdec_sound.h>
#include <swfdec/swfdec_sprite.h>
#include <swfdec/swfdec_sprite_movie.h>
#include <swfdec/swfdec_swf_decoder.h>
#include <swfdec/swfdec_renderer_internal.h>
#include <swfdec/swfdec_resource.h>

static SwfdecBuffer *
encode_wav (SwfdecBuffer *buffer)
{
  SwfdecBuffer *wav = swfdec_buffer_new (buffer->length + 44);
  unsigned char *data;
  guint i;

  data = wav->data;
  /* FIXME: too much magic in this memmove */
  memmove (data, "RIFF----WAVEfmt \020\0\0\0"
		 "\001\0ccRRRRbbbbAAbbdata", 40);
  *(guint32 *) (void *) &data[4] = GUINT32_TO_LE (buffer->length + 36);
  /* rate */
  *(guint16 *) (void *) &data[22] = GUINT16_TO_LE (44100);
  /* channels */
  *(guint32 *) (void *) &data[24] = GUINT32_TO_LE (2);
  /* bits per sample */
  *(guint16 *) (void *) &data[34] = GUINT16_TO_LE (16);
  /* block align */
  *(guint16 *) (void *) &data[32] = GUINT16_TO_LE (16 * 2);
  /* bytes per second */
  *(guint32 *) (void *) &data[28] = GUINT32_TO_LE (16 * 2 * 44100);
  *(guint32 *) (void *) &data[40] = GUINT32_TO_LE (buffer->length);
  data += 44;
  for (i = 0; i < buffer->length; i += 2) {
    *(gint16 *) (void *) (data + i) = GINT16_TO_LE (*(gint16* ) (void *) (buffer->data + i));
  }
  return wav;
}

static gboolean
export_sound (SwfdecSound *sound, const char *filename)
{
  GError *error = NULL;
  SwfdecBuffer *wav, *buffer;

  /* try to render the sound, that should decode it. */
  buffer = swfdec_sound_get_decoded (sound);
  if (buffer == NULL) {
    g_printerr ("Couldn't decode sound. For extraction of streams extract the sprite.\n");
    return FALSE;
  }
  wav = encode_wav (buffer);
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

static gboolean
export_sprite_sound (SwfdecSprite *sprite, const char *filename)
{
  g_printerr ("FIXME: Someone implement sound export from sprites plz\n");
  return FALSE;
#if 0
  GError *error = NULL;
  guint i, depth;
  SwfdecAudio *audio;
  SwfdecBufferQueue *queue;
  SwfdecBuffer *buffer, *wav;

  audio = swfdec_audio_stream_new (NULL, sprite, i);
  i = 4096;
  queue = swfdec_buffer_queue_new ();
  while (i > 0) {
    buffer = swfdec_buffer_new0 (i * 4);
    {
      swfdec_audio_render (audio, (gint16 *) (void *) buffer->data, 0, i);
    }
    i = swfdec_audio_iterate (audio, i);
    i = MIN (i, 4096);
    swfdec_buffer_queue_push (queue, buffer);
  }
  depth = swfdec_buffer_queue_get_depth (queue);
  if (depth == 0) {
    swfdec_buffer_queue_unref (queue);
    g_printerr ("Sprite contains no sound\n");
    return FALSE;
  }
  buffer = swfdec_buffer_queue_pull (queue, depth);
  swfdec_buffer_queue_unref (queue);
  wav = encode_wav (buffer);
  swfdec_buffer_unref (buffer);
  if (!g_file_set_contents (filename, (char *) wav->data, 
	wav->length, &error)) {
    g_printerr ("Couldn't save sound to file \"%s\": %s\n", filename, error->message);
    swfdec_buffer_unref (wav);
    g_error_free (error);
    return FALSE;
  }
  swfdec_buffer_unref (wav);
  return TRUE;
#endif
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
  SwfdecRenderer *renderer;
  cairo_surface_t *surface;
  cairo_t *cr;
  guint width, height;
  const SwfdecColorTransform trans = { FALSE, 256, 0, 256, 0, 256, 0, 256, 0 };

  if (SWFDEC_IS_SPRITE (graphic)) {
    g_printerr ("Sprites can not be exported\n");
    return FALSE;
  }
  if (SWFDEC_IS_BUTTON (graphic)) {
    g_printerr ("Buttons can not be exported\n");
    return FALSE;
  }
  width = ceil (graphic->extents.x1 / SWFDEC_TWIPS_SCALE_FACTOR) 
    - floor (graphic->extents.x0 / SWFDEC_TWIPS_SCALE_FACTOR);
  height = ceil (graphic->extents.y1 / SWFDEC_TWIPS_SCALE_FACTOR) 
    - floor (graphic->extents.y0 / SWFDEC_TWIPS_SCALE_FACTOR);
  surface = surface_create_for_filename (filename, width, height);
  cr = cairo_create (surface);
  cairo_translate (cr, - floor (graphic->extents.x0 / SWFDEC_TWIPS_SCALE_FACTOR),
    - floor (graphic->extents.y0 / SWFDEC_TWIPS_SCALE_FACTOR));
  cairo_scale (cr, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR, 1.0 / SWFDEC_TWIPS_SCALE_FACTOR);
  renderer = swfdec_renderer_new (surface);
  swfdec_renderer_attach (renderer, cr);
  swfdec_graphic_render (graphic, cr, &trans);
  cairo_show_page (cr);
  cairo_destroy (cr);
  g_object_unref (renderer);
  return surface_destroy_for_type (surface, filename);
}

static gboolean
export_image (SwfdecImage *image, const char *filename)
{
  cairo_surface_t *surface = swfdec_image_create_surface (image, NULL);

  if (surface == NULL)
    return FALSE;
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
  glong id;
  SwfdecURL *url;

  swfdec_init ();

  if (argc != 4) {
    usage (argv[0]);
    return 0;
  }

  player = swfdec_player_new (NULL);
  url = swfdec_url_new_from_input (argv[1]);
  swfdec_player_set_url (player, url);
  swfdec_url_free (url);
  /* FIXME: HACK! */
  swfdec_player_advance (player, 0);
  if (!SWFDEC_IS_SPRITE_MOVIE (player->priv->roots->data)) {
    g_printerr ("Error parsing file \"%s\"\n", argv[1]);
    g_object_unref (player);
    player = NULL;
    return 1;
  }
  id = strtol (argv[2], NULL, 0);
  if (id >= 0) {
    character = swfdec_swf_decoder_get_character (
	SWFDEC_SWF_DECODER (SWFDEC_MOVIE (player->priv->roots->data)->resource->decoder),
	id);
  } else {
    character = SWFDEC_CHARACTER (SWFDEC_SWF_DECODER (
	  SWFDEC_MOVIE (player->priv->roots->data)->resource->decoder)->main_sprite);
  }
  if (SWFDEC_IS_SPRITE (character)) {
    if (!export_sprite_sound (SWFDEC_SPRITE (character), argv[3]))
      ret = 1;
  } else if (SWFDEC_IS_SOUND (character)) {
    if (!export_sound (SWFDEC_SOUND (character), argv[3]))
      ret = 1;
  } else if (SWFDEC_IS_GRAPHIC (character)) {
    if (!export_graphic (SWFDEC_GRAPHIC (character), argv[3]))
      ret = 1;
  } else if (SWFDEC_IS_IMAGE (character)) {
    if (!export_image (SWFDEC_IMAGE (character), argv[3]))
      ret = 1;
  } else {
    g_printerr ("id %ld does not specify an exportable object\n", id);
    ret = 1;
  }

  g_object_unref (player);
  player = NULL;

  return ret;
}

