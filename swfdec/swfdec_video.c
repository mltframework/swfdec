/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_video.h"
#include "swfdec_debug.h"
#include "swfdec_player_internal.h"
#include "swfdec_swf_decoder.h"
#include "swfdec_video_movie.h"
#include "swfdec_video_video_provider.h"

/*** SWFDEC_VIDEO ***/

G_DEFINE_TYPE (SwfdecVideo, swfdec_video, SWFDEC_TYPE_GRAPHIC)

static SwfdecMovie *
swfdec_video_create_movie (SwfdecGraphic *graphic, gsize *size)
{
  SwfdecVideo *video = SWFDEC_VIDEO (graphic);
  SwfdecVideoMovie *movie = g_object_new (SWFDEC_TYPE_VIDEO_MOVIE, NULL);

  movie->video = video;
  g_object_ref (video);
  if (video->n_frames > 0) {
    SwfdecVideoProvider *provider = swfdec_video_video_provider_new (video);
    swfdec_video_movie_set_provider (movie, provider);
    g_object_unref (provider);
  }
  *size = sizeof (SwfdecVideoMovie);
  return SWFDEC_MOVIE (movie);
}

static void
swfdec_video_dispose (GObject *object)
{
  SwfdecVideo * video = SWFDEC_VIDEO (object);
  guint i;

  for (i = 0; i < video->images->len; i++) {
    swfdec_buffer_unref (g_array_index (video->images, SwfdecVideoFrame, i).buffer);
  }
  g_array_free (video->images, TRUE);
  G_OBJECT_CLASS (swfdec_video_parent_class)->dispose (object);
}

static void
swfdec_video_class_init (SwfdecVideoClass * g_class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (g_class);
  SwfdecGraphicClass *graphic_class = SWFDEC_GRAPHIC_CLASS (g_class);

  object_class->dispose = swfdec_video_dispose;

  graphic_class->create_movie = swfdec_video_create_movie;
}

static void
swfdec_video_init (SwfdecVideo * video)
{
  video->images = g_array_new (FALSE, FALSE, sizeof (SwfdecVideoFrame));
}

int
tag_func_define_video (SwfdecSwfDecoder *s, guint tag)
{
  SwfdecVideo *video;
  guint id;
  SwfdecBits *bits = &s->b;
  int smoothing, deblocking;

  id = swfdec_bits_get_u16 (bits);
  video = swfdec_swf_decoder_create_character (s, id, SWFDEC_TYPE_VIDEO);
  if (!video)
    return SWFDEC_STATUS_OK;
  
  video->n_frames = swfdec_bits_get_u16 (bits);
  video->width = swfdec_bits_get_u16 (bits);
  video->height = swfdec_bits_get_u16 (bits);
  swfdec_bits_getbits (bits, 4);
  deblocking = swfdec_bits_getbits (bits, 3);
  smoothing = swfdec_bits_getbit (bits);
  video->format = swfdec_bits_get_u8 (bits);
  SWFDEC_LOG ("  frames: %u", video->n_frames);
  SWFDEC_LOG ("  size: %ux%u", video->width, video->height);
  SWFDEC_LOG ("  deblocking: %d", deblocking);
  SWFDEC_LOG ("  smoothing: %d", smoothing);
  SWFDEC_LOG ("  format: %d", (int) video->format);
  swfdec_decoder_use_video_codec (SWFDEC_DECODER (s), video->format);
  return SWFDEC_STATUS_OK;
}

int
tag_func_video_frame (SwfdecSwfDecoder *s, guint unused_tag)
{
  SwfdecVideo *video;
  guint id;
  SwfdecVideoFrame tag;

  id = swfdec_bits_get_u16 (&s->b);
  video = (SwfdecVideo *) swfdec_swf_decoder_get_character (s, id);
  if (!SWFDEC_IS_VIDEO (video)) {
    SWFDEC_ERROR ("id %u does not reference a video object", id);
    return SWFDEC_STATUS_OK;
  }
  tag.frame = swfdec_bits_get_u16 (&s->b);
  if (tag.frame >= video->n_frames) {
    SWFDEC_ERROR ("frame %u out of range %u", tag.frame, video->n_frames);
    return SWFDEC_STATUS_OK;
  }
  /* it seems flash video saves keyframe + format in every frame - 
   * at least libflv + ming does that and Flash Player plays it.
   */
  if (video->format == SWFDEC_VIDEO_CODEC_SCREEN) {
#if 0
    keyframe = swfdec_bits_get_bits (&s->b, 4);
    format = swfdec_bits_get_bits (&s->b, 4);
#endif
    swfdec_bits_get_u8 (&s->b);
  }
  tag.buffer = swfdec_bits_get_buffer (&s->b, -1);
  if (tag.buffer == NULL) {
    SWFDEC_WARNING ("no buffer, ignoring");
    return SWFDEC_STATUS_OK;
  }
  if (video->images->len == 0) {
    g_array_append_val (video->images, tag);
  } else if (g_array_index (video->images, SwfdecVideoFrame, video->images->len - 1).frame < tag.frame) {
    g_array_append_val (video->images, tag);
  } else {
    guint i;
    SWFDEC_WARNING ("frame not in ascending order (last is %u, this is %u)",
	g_array_index (video->images, SwfdecVideoFrame, video->images->len - 1).frame, tag.frame);
    for (i = 0; i < video->images->len; i++) {
      SwfdecVideoFrame *cur = &g_array_index (video->images, SwfdecVideoFrame, i);
      if (cur->frame < tag.frame)
	continue;
      if (cur->frame == tag.frame) {
	SWFDEC_ERROR ("duplicate frame id %u, discarding", cur->frame);
	swfdec_buffer_unref (tag.buffer);
	break;
      }
      g_array_insert_val (video->images, i, tag);
      break;
    }
    if (i >= video->images->len)
      g_array_append_val (video->images, tag);
  }
  return SWFDEC_STATUS_OK;
}

