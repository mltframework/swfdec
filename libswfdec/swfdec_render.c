
#include <math.h>
#include <string.h>

#include "swfdec_internal.h"
#include <swfdec_sound.h>


SwfdecRender *
swfdec_render_new (void)
{
  return g_new0 (SwfdecRender, 1);
}

void
swfdec_render_free (SwfdecRender *render)
{
  g_free (render);
}

void
swfdec_render_iterate (SwfdecDecoder *s)
{
  if (s->render->seek_frame != -1) {
    s->render->frame_index = s->render->seek_frame;
    s->render->seek_frame = -1;
  } else {
    s->render->frame_index++;
  }

}

void
swfdec_render_seek (SwfdecDecoder *s, int frame)
{
  if (frame < 0 || frame >= s->n_frames) return;

  s->render->seek_frame = frame;
}

unsigned char *
swfdec_render_get_image (SwfdecDecoder *s)
{
  SwfdecSpriteSegment *seg;
  SwfdecLayer *layer;
  unsigned char *buf;
  GList *g;

  SWFDEC_DEBUG ("swf_render_frame");

  s->render->drawrect.x0 = 0;
  s->render->drawrect.x1 = 0;
  s->render->drawrect.y0 = 0;
  s->render->drawrect.y1 = 0;
  if (!s->buffer) {
    s->buffer = g_malloc (s->stride * s->height);
    swf_invalidate_irect (s, &s->irect);
  }
swf_invalidate_irect (s, &s->irect);
  if (!s->tmp_scanline) {
    s->tmp_scanline = g_malloc (s->width);
  }

  SWFDEC_DEBUG ("rendering frame %d", s->render->frame_index);

  SWFDEC_DEBUG ("inval rect %d %d %d %d", s->render->drawrect.x0, s->render->drawrect.x1,
      s->render->drawrect.y0, s->render->drawrect.y1);

  s->fillrect (s->buffer, s->stride, s->bg_color, &s->render->drawrect);

  for (g = g_list_last (s->main_sprite->layers); g; g = g_list_previous (g)) {
    SwfdecObject *object;

    seg = (SwfdecSpriteSegment *) g->data;

    SWFDEC_LOG("testing seg %d <= %d < %d",
	seg->first_frame, s->render->frame_index, seg->last_frame);
    if (seg->first_frame > s->render->frame_index)
      continue;
    if (seg->last_frame <= s->render->frame_index)
      continue;

    object = swfdec_object_get (s, seg->id);
    if (object) {
      layer = SWFDEC_OBJECT_GET_CLASS(object)->prerender (s, seg, object, NULL);

      if (layer) {
        swfdec_layer_render (s, layer);
        swfdec_layer_free (layer);
      } else {
        SWFDEC_WARNING ("prerender returned NULL");
      }
    } else {
      SWFDEC_DEBUG ("could not find object (id = %d)", seg->id);
    }
  }

  buf = s->buffer;
  s->buffer = NULL;
  return buf;
}

int
swfdec_render_get_audio (SwfdecDecoder *s, unsigned char **data)
{
  int length;
  SwfdecBuffer *buffer;
  GList *g;

  if (s->stream_sound_obj) {
    SwfdecSoundChunk *chunk;

    chunk = s->main_sprite->sound_chunks[s->render->frame_index];
    if (chunk) {
      SwfdecSound *sound;

      sound = SWFDEC_SOUND(s->stream_sound_obj);

      memcpy (sound->tmpbuf + sound->tmpbuflen, chunk->data, chunk->length);
      sound->tmpbuflen += chunk->length;
      swfdec_sound_mp3_decode_stream (s, s->stream_sound_obj);

    }
  }

  swfdec_sound_render (s);

  g = g_list_first (s->sound_buffers);
  if (!g) return 0;

  buffer = (SwfdecBuffer *) g->data;
  s->sound_buffers = g_list_delete_link (s->sound_buffers, g);

  if (data) {
    *data = buffer->data;
  } else {
    g_free (buffer->data);
  }
  length = buffer->length;
  g_free(buffer);

  return length;
}





#if 0

SwfdecLayer *
swfdec_spriteseg_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecLayer * oldlayer)
{
  SwfdecObject *object;
  SwfdecObjectClass *klass;

  object = swfdec_object_get (s, seg->id);
  if (!object)
    return NULL;

  klass = SWFDEC_OBJECT_GET_CLASS (object);

  if (klass->prerender) {
    return klass->prerender (s, seg, object, oldlayer);
  }

  SWFDEC_ERROR ("why is prerender NULL?");

  return NULL;
}

void
swfdec_layer_render (SwfdecDecoder * s, SwfdecLayer * layer)
{
  int i;
  SwfdecLayerVec *layervec;
  SwfdecLayer *child_layer;
  GList *g;

#if 0
  /* This rendering order seems to mostly fit the observed behavior
   * of Macromedia's player.  */
  /* or not */
  for (i = 0; i < MAX (layer->fills->len, layer->lines->len); i++) {
    if (i < layer->lines->len) {
      layervec = &g_array_index (layer->lines, SwfdecLayerVec, i);
      swfdec_layervec_render (s, layervec);
    }
    if (i < layer->fills->len) {
      layervec = &g_array_index (layer->fills, SwfdecLayerVec, i);
      swfdec_layervec_render (s, layervec);
    }
  }
#else
  for (i = 0; i < layer->fills->len; i++) {
    layervec = &g_array_index (layer->fills, SwfdecLayerVec, i);
    swfdec_layervec_render (s, layervec);
  }
  for (i = 0; i < layer->lines->len; i++) {
    layervec = &g_array_index (layer->lines, SwfdecLayerVec, i);
    swfdec_layervec_render (s, layervec);
  }
#endif

  for (g = g_list_first (layer->sublayers); g; g = g_list_next (g)) {
    child_layer = (SwfdecLayer *) g->data;
    swfdec_layer_render (s, child_layer);
  }
}

#endif

