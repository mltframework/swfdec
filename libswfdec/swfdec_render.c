
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
  GList *g;

  for (g=g_list_first (render->object_states); g; g = g_list_next(g)) {
    g_free(g->data);
  }
  g_list_free(render->object_states);

  g_free (render);
}

gboolean
swfdec_render_iterate (SwfdecDecoder *s)
{
  GList *g;
  
  if (s->render->seek_frame != -1) {
    SwfdecSound *sound;

    s->render->frame_index = s->render->seek_frame;
    s->render->seek_frame = -1;

    sound = SWFDEC_SOUND(s->stream_sound_obj);
    if (sound) sound->tmpbuflen = 0;
  } else {
    if (!s->stopped) {
      if (s->main_sprite->actions[s->render->frame_index]) {
        swfdec_action_script_execute(s,
            s->main_sprite->actions[s->render->frame_index]);
      }
    }

    if (!s->stopped) {
      s->render->frame_index++;
      if (s->render->frame_index >= s->n_frames) {
        SWFDEC_WARNING ("iterating past end");
        return FALSE;
#if 0
        s->stopped = TRUE;
        s->render->frame_index = s->n_frames - 1;
#endif
      }
    }
  }

  for (g=g_list_first (s->render->object_states); g; g = g_list_next(g)) {
    SwfdecRenderState *state = g->data;

    state->frame_index++;
    SWFDEC_INFO ("iterate layer=%d frame_index=%d", state->layer,
        state->frame_index);
  }

  return TRUE;
}

SwfdecRenderState *
swfdec_render_get_object_state (SwfdecRender *render, int layer)
{
  GList *g;

  for (g=g_list_first (render->object_states); g; g = g_list_next(g)) {
    SwfdecRenderState *state = g->data;

    if (state->layer == layer) return state;
  }

  return NULL;
}

void
swfdec_render_seek (SwfdecDecoder *s, int frame)
{
  if (frame < 0 || frame >= s->n_frames) return;

  s->render->seek_frame = frame;
}

int
swfdec_render_get_frame_index (SwfdecDecoder *s)
{
  return s->render->frame_index;
}

SwfdecBuffer *
swfdec_render_get_image (SwfdecDecoder *s)
{
  SwfdecSpriteSegment *seg;
  SwfdecBuffer *buffer;
  GList *g;
  int clip_depth = 0;

  g_return_val_if_fail (s->render->frame_index < s->n_frames, NULL);

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

    /* FIXME need to clip layers instead */
    if (seg->clip_depth) {
      SWFDEC_INFO ("clip_depth=%d", seg->clip_depth);
      clip_depth = seg->clip_depth;
    }
#if 0
    /* don't render clipped layers */
    if (clip_depth && seg->depth <= clip_depth) {
      SWFDEC_INFO ("clipping depth=%d", seg->depth);
      continue;
    }
#endif
#if 0
    /* render only the clipping layer */
    if (seg->clip_depth == 0 && clip_depth && seg->depth <= clip_depth) {
      SWFDEC_INFO ("clipping depth=%d", seg->depth);
      continue;
    }
#endif
#if 1
    /* don't render clipping layer */
    if (seg->clip_depth) {
      continue;
    }
#endif

    object = swfdec_object_get (s, seg->id);
    if (object) {
      SWFDEC_OBJECT_GET_CLASS(object)->render (s, seg, object);
    } else {
      SWFDEC_DEBUG ("could not find object (id = %d)", seg->id);
    }
  }

  buffer = swfdec_buffer_new_with_data (s->buffer, s->stride * s->height);

  s->buffer = NULL;
  return buffer;
}

SwfdecBuffer *
swfdec_render_get_audio (SwfdecDecoder *s)
{
  SwfdecBuffer *buffer;
  GList *g;

  g_return_val_if_fail (s->render->frame_index < s->n_frames, NULL);

  if (s->stream_sound_obj) {
    SwfdecBuffer *chunk;

    chunk = s->main_sprite->sound_chunks[s->render->frame_index];
    if (chunk) {
      SwfdecSound *sound;
      int n;

      sound = SWFDEC_SOUND(s->stream_sound_obj);

      n = chunk->length;
      if (sound->tmpbuflen + n > 2048) {
        n = 2048 - sound->tmpbuflen;
        SWFDEC_WARNING ("clipping audio");
      }
      memcpy (sound->tmpbuf + sound->tmpbuflen, chunk->data, n);
      sound->tmpbuflen += n;
      swfdec_sound_mp3_decode_stream (s, s->stream_sound_obj);
    }
  }

  swfdec_sound_render (s);

  g = g_list_first (s->sound_buffers);
  if (!g) return NULL;

  buffer = (SwfdecBuffer *) g->data;
  s->sound_buffers = g_list_delete_link (s->sound_buffers, g);

  return buffer;
}




