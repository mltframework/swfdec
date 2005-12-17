
#include <math.h>
#include <string.h>

#include "swfdec_internal.h"
#include <swfdec_sound.h>
#include <liboil/liboil.h>

void swfdec_decoder_sound_buffer_append (SwfdecDecoder * s,
    SwfdecBuffer * buffer);

SwfdecRender *
swfdec_render_new (void)
{
  SwfdecRender *render;

  render = g_new0 (SwfdecRender, 1);

  return render;
}

void
swfdec_render_free (SwfdecRender * render)
{
  GList *g;

  for (g = g_list_first (render->object_states); g; g = g_list_next (g)) {
    g_free (g->data);
  }
  g_list_free (render->object_states);

  g_free (render);
}

gboolean
swfdec_render_iterate (SwfdecDecoder * s)
{
  GList *g;

  SWFDEC_DEBUG("iterate, frame_index = %d", s->render->frame_index);

  s->render->frame_index = s->next_frame;
  s->next_frame = -1;

  swfdec_sprite_render_iterate (s, s->main_sprite_seg, s->render);

  SWFDEC_DEBUG ("mouse button %d old_mouse_button %d active_button %p",
      s->mouse_button, s->old_mouse_button, s->render->active_button);
  if (s->mouse_button && !s->old_mouse_button &&
      s->render->active_button) {
              /* FIXME? button down->up/up->down */
    SWFDEC_ERROR("executing button");
    swfdec_button_execute (s, SWFDEC_BUTTON (s->render->active_button));
  }

  for (g=s->execute_list; g; g = g->next) {
    SwfdecBuffer *buffer = g->data;
    swfdec_action_script_execute (s, buffer);
  }
  g_list_free (s->execute_list);
  s->execute_list = NULL;

  s->render->active_button = NULL;
  s->old_mouse_button = s->mouse_button;

  if (s->next_frame == -1) {
    if (!s->main_sprite_seg->stopped) {
      s->next_frame = s->render->frame_index + 1;
      if (s->next_frame >= s->n_frames) {
        if (0 /*s->repeat*/) {
          s->next_frame = 0;
        } else {
          s->next_frame = s->n_frames - 1;
        }
      }
    } else {
      s->next_frame = s->render->frame_index;
    }
  }

  return TRUE;
}

int
swfdec_render_get_frame_index (SwfdecDecoder * s)
{
  return s->render->frame_index;
}

void
swfdec_render_resize (SwfdecDecoder *s)
{
  g_free(s->kept_buffer);
  s->kept_buffer = NULL;
  g_list_free (s->kept_list);
  s->kept_list = NULL;
  s->kept_layers = 0;

}

static int
compare_lists (GList *a, GList *b)
{
  int n = 0;

  while (a != NULL && b != NULL && a->data == b->data) {
    a = g_list_next(a);
    b = g_list_next(b);
    n++;
  }

  return n;
}

SwfdecBuffer *
swfdec_render_get_image (SwfdecDecoder * s)
{
  SwfdecSpriteSegment *seg;
  SwfdecBuffer *buffer;
  SwfdecSpriteFrame *frame;
  GList *g;
  GList *render_list = NULL;
  int clip_depth = 0;
  int n_kept_layers;
  int n_frames;
  int n;
  int i;

  g_return_val_if_fail (s->render->frame_index < s->n_frames, NULL);

  SWFDEC_DEBUG ("swf_render_frame index %d", s->render->frame_index);

  s->render->drawrect.x0 = 0;
  s->render->drawrect.x1 = 0;
  s->render->drawrect.y0 = 0;
  s->render->drawrect.y1 = 0;
  swf_invalidate_irect (s, &s->irect);

  SWFDEC_DEBUG ("inval rect %d %d %d %d", s->render->drawrect.x0,
      s->render->drawrect.x1, s->render->drawrect.y0, s->render->drawrect.y1);

  frame = &s->main_sprite->frames[s->render->frame_index];
  for (g = g_list_last (frame->segments); g; g = g_list_previous (g)) {
    seg = (SwfdecSpriteSegment *) g->data;

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

    render_list = g_list_append (render_list, seg);
  }

  if (s->kept_layers) {
    n = compare_lists (s->kept_list, render_list);
    if (n < s->kept_layers) {
      g_list_free (s->kept_list);
      s->kept_list = NULL;
      s->kept_layers = 0;
    }
  }

#define MAX_KEPT_FRAMES 10
  n_frames = MAX_KEPT_FRAMES;
  i = 0;
  n_kept_layers = 0;
  for (g = render_list; g; g = g_list_next (g)) {
    int n;

    seg = (SwfdecSpriteSegment *) g->data;
    //n = seg->last_frame - s->render->frame_index - 1;
    n = 1;
    if (n==0) break;
    if (i < s->kept_layers) {
      if (n < n_frames) n_frames = n;
      n_kept_layers++;
    } else {
      gboolean is_button;
      SwfdecObject *obj;

      obj = swfdec_object_get (s, seg->id);
      if (obj) {
        is_button = SWFDEC_IS_BUTTON (obj);
      } else {
        is_button = FALSE;
      }
      if ((n <= n_frames || n >= MAX_KEPT_FRAMES) &&
          (n_kept_layers + 1) * n > n_kept_layers * n_frames &&
          !is_button) {
        if (n < MAX_KEPT_FRAMES) n_frames = n;
        n_kept_layers++;
        SWFDEC_DEBUG ("keeping layer (%d frames)", n);
      } else {
        SWFDEC_DEBUG ("not keeping layer for %d frames", n);
        break;
      }
    }
    i++;
  }
  if (n_kept_layers < s->kept_layers) {
    n_kept_layers = 0;
  }
  SWFDEC_DEBUG ("keeping %d layers for %d frames", n_kept_layers, n_frames);

  swfdec_render_be_start (s);

  g = render_list;
  i = 0;
  if (s->kept_layers) {
    oil_copy_u8 (s->buffer, s->kept_buffer, s->stride * s->height);

#if 0
    /* this highlights areas that have been cached */
    {
      int i,j;
      for(i=0;i<s->height;i++){
        for(j=0;j<s->width;j++){
          if ((i+j)&1){
            *(guint32 *)(s->buffer + i*s->stride + j*4) = 0;
          }
        }
      }
    }
#endif

    for(i=0;i<s->kept_layers;i++){
      g = g_list_next (g);
    }
  } else {
    swfdec_render_be_clear (s);
  }
  for (; g; g = g_list_next (g)) {
    SwfdecObject *object;

    seg = (SwfdecSpriteSegment *) g->data;
    object = swfdec_object_get (s, seg->id);
    if (object) {
      if (SWFDEC_OBJECT_GET_CLASS (object)->render) {
        SWFDEC_OBJECT_GET_CLASS (object)->render (s, seg, object);
      } else {
        SWFDEC_ERROR ("class render function is NULL for class %s",
            g_type_name (G_TYPE_FROM_INSTANCE (object)));
      }
    } else {
      SWFDEC_DEBUG ("could not find object (id = %d)", seg->id);
    }
    if (i < n_kept_layers) {
      s->kept_list = g_list_append(s->kept_list, seg);
      if (i == n_kept_layers - 1) {
        if (s->kept_buffer == NULL) {
          s->kept_buffer = g_malloc (s->stride * s->height);
        }
        oil_copy_u8 (s->kept_buffer, s->buffer, s->stride * s->height);
        s->kept_layers = n_kept_layers;
      }
    }
    i++;
  }
  g_list_free (render_list);

  swfdec_render_be_stop (s);

  buffer = swfdec_buffer_new_with_data (s->buffer, s->stride * s->height);

  s->buffer = NULL;
  return buffer;
}

SwfdecBuffer *
swfdec_render_get_audio (SwfdecDecoder * s)
{
  g_return_val_if_fail (s->render->frame_index < s->n_frames, NULL);

  if (s->stream_sound_obj) {
    SwfdecBuffer *chunk;

    chunk = s->main_sprite->frames[s->render->frame_index].sound_chunk;
    if (chunk) {
      SwfdecSound *sound;
      int n;

      sound = SWFDEC_SOUND (s->stream_sound_obj);

      n = chunk->length;
      if (sound->tmpbuflen + n > 2048) {
        n = 2048 - sound->tmpbuflen;
        SWFDEC_WARNING ("clipping audio");
      }
      oil_copy_u8 (sound->tmpbuf + sound->tmpbuflen, chunk->data, n);
      sound->tmpbuflen += n;
      swfdec_sound_mp3_decode_stream (s, s->stream_sound_obj);
    }
  }

  if (s->main_sprite->frames[s->render->frame_index].sound_play) {
    SwfdecSound *sound;
    SwfdecSoundChunk *chunk =
      s->main_sprite->frames[s->render->frame_index].sound_play;

    SWFDEC_DEBUG("chunk %p frame_index %d", chunk, s->render->frame_index);
    SWFDEC_DEBUG("play sound object=%d start=%d stop=%d stopflag=%d no_restart=%d loop_count=%d",
        chunk->object, chunk->start_sample, chunk->stop_sample,
        chunk->stop, chunk->no_restart, chunk->loop_count);

    sound = SWFDEC_SOUND(swfdec_object_get (s, chunk->object));
    if (sound) {
      swfdec_audio_add_sound (s, sound, chunk->loop_count);
    }
  }

  return swfdec_audio_render (s, 44100/s->rate);
}
