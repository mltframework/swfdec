#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include <js/jsapi.h>

#include "swfdec_movieclip.h"
#include "swfdec_internal.h"
#include "swfdec_js.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*** MOVIE CLIP ***/

SWFDEC_OBJECT_BOILERPLATE (SwfdecMovieClip, swfdec_movie_clip)

static void swfdec_movie_clip_base_init (gpointer g_class)
{

}

static void
swfdec_movie_clip_init (SwfdecMovieClip * movie)
{
  movie->xscale = 100;
  movie->yscale = 100;
  cairo_matrix_init_identity (&movie->transform);
  cairo_matrix_init_identity (&movie->inverse_transform);

  swfdec_color_transform_init_identity (&movie->color_transform);

  movie->visible = TRUE;

  movie->button_state = SWFDEC_BUTTON_UP;
  movie->current_frame = -1;

  movie->sound_queue = g_queue_new ();
}

/* NB: modifies rect */
static void
swfdec_movie_clip_invalidate (SwfdecMovieClip *movie, SwfdecRect *rect)
{
  swfdec_rect_transform (rect, rect, &movie->transform);
  while (movie->parent) {
    movie = movie->parent;
    swfdec_rect_transform (rect, rect, &movie->transform);
  }
  swfdec_object_invalidate (SWFDEC_OBJECT (movie), rect);
}

static void
swfdec_movie_clip_invalidate_cb (SwfdecObject *child, const SwfdecRect *rect, SwfdecMovieClip *movie)
{
  SwfdecRect inval;

  inval = *rect;
  swfdec_movie_clip_invalidate (movie, &inval);
}

static void
swfdec_movie_clip_update_extents (SwfdecMovieClip *movie)
{
  /* FIXME: handle real movieclips */
  if (movie->child && !SWFDEC_IS_SPRITE (movie)) {
    swfdec_rect_transform (&SWFDEC_OBJECT (movie)->extents, 
	&movie->child->extents, &movie->transform);
  }
}

static void
swfdec_movie_clip_dispose (SwfdecMovieClip * movie)
{
  g_assert (movie->jsobj == NULL);

  g_list_foreach (movie->list, (GFunc) swfdec_object_unref, NULL);
  g_list_free (movie->list);

  swfdec_movie_clip_set_child (movie, NULL);

  g_queue_free (movie->sound_queue);

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (movie));
}

static void
swfdec_movie_clip_perform_actions (SwfdecMovieClip *movie, guint last_frame)
{
  SwfdecMovieClip *cur;
  SwfdecRect inval;
  SwfdecSpriteFrame *frame = &SWFDEC_SPRITE (movie->child)->frames[movie->current_frame];
  GList *walk, *walk2; /* I suck at variable naming */
  GSList *swalk;

  if (last_frame == (guint) -1 || frame->bg_color != SWFDEC_SPRITE (movie->child)->frames[last_frame].bg_color)
    inval = SWFDEC_OBJECT (movie)->extents;
  else
    swfdec_rect_init_empty (&inval);
  walk2 = movie->list;
  for (walk = frame->contents; walk; walk = g_list_next (walk)) {
    SwfdecSpriteContent *contents = walk->data;
    gboolean new = FALSE;

    /* first, find or create the correct clip */
    while (walk2) {
      cur = walk2->data;
      if (cur->depth > contents->depth)
	break;
      walk2 = walk2->next;
      if (cur->depth == contents->depth)
	goto found;
      /* if cur->depth < contents->depth */
      movie->list = g_list_remove (movie->list, cur);
      swfdec_rect_union (&inval, &inval, &SWFDEC_OBJECT (cur)->extents);
      swfdec_object_unref (cur);
    }
    new = TRUE;
    cur = swfdec_movie_clip_new (movie);
    cur->depth = contents->depth;
    movie->list = g_list_insert_before (movie->list, walk2, cur);
found:
    /* check if we need to change anything */
    if (contents->first_frame <= last_frame &&
	last_frame < contents->last_frame) {
      continue;
    }
    /* invalidate the current element unless it's new */
    if (!new)
      swfdec_rect_union (&inval, &inval, &SWFDEC_OBJECT (cur)->extents);

    /* copy all the relevant stuff */
    if (cur->child != contents->object)
      swfdec_movie_clip_set_child (cur, contents->object);
    cur->clip_depth = contents->clip_depth;
    cur->ratio = contents->ratio;
    cur->inverse_transform = cur->transform = contents->transform;
    if (cairo_matrix_invert (&cur->inverse_transform)) {
      g_assert_not_reached ();
    }
    cur->color_transform = contents->color_transform;
    cur->name = contents->name;
    cur->events = contents->events;

    /* do all the necessary setup stuff */
    swfdec_movie_clip_update_extents (cur);
    swfdec_rect_union (&inval, &inval, &SWFDEC_OBJECT (cur)->extents);
  }
    
  while (walk2) {
    cur = walk2->data;
    swfdec_rect_union (&inval, &inval, &SWFDEC_OBJECT (cur)->extents);
    swfdec_object_unref (cur);
    walk2 = walk2->next;
    movie->list = g_list_remove (movie->list, cur);
  }
  for (swalk = frame->sound; swalk; swalk = swalk->next) {
    swfdec_audio_event_init (SWFDEC_OBJECT (movie)->decoder, walk->data);
  }
  if (frame->do_actions) {
    for (swalk = frame->do_actions; swalk; swalk = swalk->next)
      swfdec_decoder_queue_script (SWFDEC_OBJECT (movie)->decoder, movie, swalk->data);
  }
  if (!swfdec_rect_is_empty (&inval)) {
    swfdec_movie_clip_invalidate (movie, &inval);
  }
  /* if we did everything right, we should now have as many children as the frame says */
  g_assert (g_list_length (movie->list) == g_list_length (frame->contents));
}

static unsigned int
swfdec_movie_clip_get_next_frame (SwfdecMovieClip *movie, unsigned int current_frame)
{
  unsigned int next_frame, n_frames;

  g_assert (SWFDEC_IS_SPRITE (movie->child));

  n_frames = SWFDEC_SPRITE (movie->child)->n_frames;
  next_frame = current_frame + 1;
  if (next_frame >= n_frames) {
    if (0 /*s->repeat*/) {
      next_frame = 0;
    } else {
      next_frame = n_frames - 1;
    }
  }
  return next_frame;
}

static void
swfdec_movie_clip_iterate_audio (SwfdecMovieClip *movie, unsigned int last_frame)
{
  SwfdecBuffer *cur;
  SwfdecSpriteFrame *last = last_frame == (guint) -1 ? NULL : &SWFDEC_SPRITE (movie->child)->frames[last_frame];
  SwfdecSpriteFrame *current = &SWFDEC_SPRITE (movie->child)->frames[movie->current_frame];
  unsigned int n_bytes;

  if (last == NULL || last->sound_head != current->sound_head) {
    /* empty queue */
    while ((cur = g_queue_pop_head (movie->sound_queue)))
      swfdec_buffer_unref (cur);
    if (movie->sound_decoder)
      swfdec_sound_finish_decoder (last->sound_head, movie->sound_decoder);
    if (current->sound_head)
      movie->sound_decoder = swfdec_sound_init_decoder (current->sound_head);
    else
      movie->sound_decoder = NULL;
  } else if (swfdec_movie_clip_get_next_frame (movie, last_frame) != movie->current_frame) {
    /* empty queue */
    while ((cur = g_queue_pop_head (movie->sound_queue)))
      swfdec_buffer_unref (cur);
  } 
  /* flush unneeded bytes */
  n_bytes = SWFDEC_OBJECT (movie)->decoder->samples_this_frame * 4;
  while (n_bytes > 0 && (cur = g_queue_pop_head (movie->sound_queue))) {
    if (n_bytes < cur->length) {
      SwfdecBuffer *sub = swfdec_buffer_new_subbuffer (cur, n_bytes, cur->length - n_bytes);
      g_assert (sub->length % 4 == 0);
      g_queue_push_head (movie->sound_queue, sub);
      swfdec_buffer_unref (cur);
      break;
    }
    n_bytes -= cur->length;
    swfdec_buffer_unref (cur);
  }
}

void 
swfdec_movie_clip_iterate (SwfdecMovieClip *movie)
{
  GList *walk;
  unsigned int last_frame;

  if (movie->stopped || !movie->visible)
    return;

  /* don't do anything if the requested frame isn't loaded yet */
  if (SWFDEC_IS_SPRITE (movie->child) && 
      movie->next_frame >= SWFDEC_SPRITE (movie->child)->parse_frame) {
    SWFDEC_DEBUG ("not iterating, frame %u isn't loaded (loading %u)",
      movie->next_frame, SWFDEC_SPRITE (movie->child)->parse_frame);
    return;
  }

  SWFDEC_LOG ("iterate, frame_index = %d", movie->next_frame);
  last_frame = movie->current_frame;
  movie->current_frame = movie->next_frame;
  movie->next_frame = -1;
  if (SWFDEC_IS_SPRITE (movie->child)) {
    swfdec_movie_clip_perform_actions (movie, last_frame);
    for (walk = movie->list; walk; walk = walk->next) {
      swfdec_movie_clip_iterate (walk->data);
    }
    if (movie->next_frame == (unsigned int) -1)
      movie->next_frame = swfdec_movie_clip_get_next_frame (movie, movie->current_frame);
    swfdec_movie_clip_iterate_audio (movie, last_frame);
  }
}

static gboolean
swfdec_movie_clip_grab_mouse (SwfdecMovieClip *movie, SwfdecMovieClip *grab)
{
  if (movie->mouse_grab)
    return FALSE;

  if (movie->parent && !swfdec_movie_clip_grab_mouse (movie->parent, movie))
    return FALSE;

  movie->mouse_grab = grab;
  return TRUE;
}

static void
swfdec_movie_clip_ungrab_mouse (SwfdecMovieClip *movie)
{
  g_assert (movie->mouse_grab != NULL);

  if (movie->parent)
    swfdec_movie_clip_ungrab_mouse (movie->parent);
  movie->mouse_grab = NULL;
}

gboolean
swfdec_movie_clip_handle_mouse (SwfdecMovieClip *movie,
      double x, double y, int button)
{
  GList *g;
  unsigned int clip_depth = 0;
  SwfdecMovieClip *child;
  gboolean was_in = movie->mouse_in;
  guint old_button = movie->mouse_button;

  g_assert (button == 0 || button == 1);

  cairo_matrix_transform_point (&movie->inverse_transform, &x, &y);
  movie->mouse_in = FALSE;
  movie->mouse_x = x;
  movie->mouse_y = y;
  SWFDEC_LOG ("moviclip %p mouse: %g %g\n", movie, x, y);
  movie->mouse_button = button;
  g = movie->list;
  if (movie->mouse_grab) {
    child = movie->mouse_grab;
    movie->mouse_grab = NULL;
    goto grab_exists;
  }
  while (g) {
    child = g->data;
    g = g->next;
grab_exists:
    if (child->clip_depth) {
      SWFDEC_LOG ("clip_depth=%d", child->clip_depth);
      clip_depth = child->clip_depth;
    }

    if (clip_depth && child->depth <= clip_depth) {
      SWFDEC_DEBUG ("clipping depth=%d", child->clip_depth);
      continue;
    }

    if (swfdec_movie_clip_handle_mouse (child, x, y, button))
      movie->mouse_in = TRUE;
  }
  if (movie->child) {
    if (swfdec_object_mouse_in (movie->child, x, y, button))
      movie->mouse_in = TRUE;
    if (SWFDEC_IS_BUTTON (movie->child)) {
      SwfdecButtonState state = swfdec_button_change_state (movie, was_in, old_button);
      if (state != movie->button_state) {
	SWFDEC_INFO ("%p changed button state from %u to %u", movie, movie->button_state, state);
	movie->button_state = state;
      }
    }
  }
  return movie->mouse_in;
}

static void
swfdec_movie_clip_render (SwfdecObject *object, cairo_t *cr,
    const SwfdecColorTransform *color_transform, const SwfdecRect *inval)
{
  SwfdecMovieClip *movie = SWFDEC_MOVIE_CLIP (object);
  GList *g;
  unsigned int clip_depth = 0;
  SwfdecColorTransform trans;
  SwfdecRect rect;

  /* FIXME: do proper clipping */
  if (movie->parent == NULL) {
    /* only the root has a background */
    swfdec_color_set_source (cr, movie->bg_color);
    cairo_paint (cr);
  }

  SWFDEC_LOG ("%stransforming clip: %g %g  %g %g   %g %g", movie->parent ? "  " : "",
      movie->transform.xx, movie->transform.yy,
      movie->transform.xy, movie->transform.yx,
      movie->transform.x0, movie->transform.y0);
  cairo_transform (cr, &movie->transform);
  swfdec_rect_transform (&rect, inval, &movie->inverse_transform);
  SWFDEC_LOG ("%sinvalid area is now: %g %g  %g %g",  movie->parent ? "  " : "",
      rect.x0, rect.y0, rect.x1, rect.y1);
  swfdec_color_transform_chain (&trans, &movie->color_transform, color_transform);

  for (g = movie->list; g; g = g_list_next (g)) {
    SwfdecMovieClip *child = g->data;

    if (child->clip_depth) {
      SWFDEC_INFO ("clip_depth=%d", child->clip_depth);
      clip_depth = child->clip_depth;
    }

    if (clip_depth && child->depth <= clip_depth) {
      SWFDEC_INFO ("clipping depth=%d", child->clip_depth);
      continue;
    }

    SWFDEC_LOG ("rendering %p with depth %d", child, child->depth);
    swfdec_object_render (SWFDEC_OBJECT (child), cr, &trans, &rect);
  }
  if (movie->child) {
    if (SWFDEC_IS_BUTTON (movie->child)) {
      swfdec_button_render (SWFDEC_BUTTON (movie->child), movie->button_state,
	  cr, &trans, &rect);
    } else {
      swfdec_object_render (movie->child, cr, &trans, &rect);
    }
  }
}

static gboolean
must_not_happen (SwfdecObject *object, double x, double y, int button)
{
  g_assert_not_reached ();
  return FALSE;
}

static void
swfdec_movie_clip_class_init (SwfdecMovieClipClass * g_class)
{
  SwfdecObjectClass *object_class = SWFDEC_OBJECT_CLASS (g_class);

  object_class->render = swfdec_movie_clip_render;
  object_class->mouse_in = must_not_happen;
}

/**
 * swfdec_movie_clip_update_visuals:
 * @movie_clip: a #SwfdecMovieClip
 *
 * Updates all the internal data associated with the visual representation
 * of the @movie_clip. This includes the cache and the area queued for updates.
 **/
void
swfdec_movie_clip_update_visuals (SwfdecMovieClip *movie_clip)
{
  cairo_matrix_t *matrix;
  SwfdecObject *object;
  SwfdecRect rect = { 0, 0, 0, 0 };

  g_return_if_fail (SWFDEC_IS_SPRITE (movie_clip));

  object = SWFDEC_OBJECT (movie_clip);
  matrix = &movie_clip->transform;
  cairo_matrix_init_identity (matrix);
  cairo_matrix_scale (matrix, movie_clip->xscale / 100., movie_clip->yscale / 100.);
  cairo_matrix_translate (matrix, movie_clip->x, movie_clip->y);
  cairo_matrix_rotate (matrix, M_PI * movie_clip->rotation / 180);
  movie_clip->inverse_transform = movie_clip->transform;
  if (cairo_matrix_invert (&movie_clip->inverse_transform)) {
    g_assert_not_reached ();
  }
  rect = object->extents;
  swfdec_movie_clip_invalidate (movie_clip, &rect);
  rect.x1 = movie_clip->width;
  rect.y1 = movie_clip->height;
  swfdec_rect_transform (&rect, &rect, matrix);
  object->extents.x0 = MIN (rect.x0, rect.x1);
  object->extents.y0 = MIN (rect.y0, rect.y1);
  object->extents.x1 = MAX (rect.x0, rect.x1);
  object->extents.y1 = MAX (rect.y0, rect.y1);
  rect = object->extents;
  swfdec_movie_clip_invalidate (movie_clip, &rect);
}

unsigned int
swfdec_movie_clip_get_n_frames (SwfdecMovieClip *movie)
{
  g_return_val_if_fail (SWFDEC_IS_MOVIE_CLIP (movie), 1);
  
  if (!SWFDEC_IS_SPRITE (movie->child))
    return 1;
  return SWFDEC_SPRITE (movie->child)->n_frames;
}

/**
 * swfdec_movie_clip_new:
 * @parent: the parent movie that contains this movie
 *
 * Creates a new #SwfdecMovieClip as a child of @parent.
 *
 * Returns: a new #SwfdecMovieClip
 **/
SwfdecMovieClip *
swfdec_movie_clip_new (SwfdecMovieClip *parent)
{
  SwfdecMovieClip *ret;

  g_return_val_if_fail (SWFDEC_IS_MOVIE_CLIP (parent), NULL);

  SWFDEC_DEBUG ("new movie for parent %p", parent);
  ret = swfdec_object_new (SWFDEC_OBJECT (parent)->decoder, SWFDEC_TYPE_MOVIE_CLIP);
  ret->parent = parent;
  ret->width = parent->width;
  ret->height = parent->height;

  return ret;
}

void
swfdec_movie_clip_set_child (SwfdecMovieClip *movie, SwfdecObject *child)
{
  g_return_if_fail (SWFDEC_IS_MOVIE_CLIP (movie));
  g_return_if_fail (child == NULL || SWFDEC_IS_OBJECT (child));

  if (movie->child) {
    if (SWFDEC_IS_SPRITE (movie->child) && movie->sound_decoder) {
      SwfdecBuffer *cur;
      SwfdecSpriteFrame *frame;
      frame = &SWFDEC_SPRITE (movie->child)->frames[movie->current_frame];
      swfdec_sound_finish_decoder (frame->sound_head, movie->sound_decoder);
      movie->sound_decoder = NULL;
      /* empty sound queue */
      while ((cur = g_queue_pop_head (movie->sound_queue)))
	swfdec_buffer_unref (cur);
    }
    if (g_signal_handlers_disconnect_by_func (movie->child, 
	swfdec_movie_clip_invalidate_cb, movie) != 1) {
      g_assert_not_reached ();
    }
    g_object_unref (movie->child);
  }
  movie->child = child;
  if (child) {
    g_object_ref (child);
    g_signal_connect (child, "invalidate", 
	G_CALLBACK (swfdec_movie_clip_invalidate_cb), movie);
  }
  movie->current_frame = -1;
  movie->next_frame = 0;
}

/* NB: width and height are in twips */
void
swfdec_movie_clip_set_size (SwfdecMovieClip *clip, guint width, guint height)
{
  SwfdecRect rect = { 0, 0, width, height };

  g_return_if_fail (SWFDEC_IS_MOVIE_CLIP (clip));

  swfdec_object_invalidate (SWFDEC_OBJECT (clip), NULL);
  clip->width = width;
  clip->height = height;
  swfdec_rect_transform (&SWFDEC_OBJECT (clip)->extents, &rect, &clip->inverse_transform);
  swfdec_object_invalidate (SWFDEC_OBJECT (clip), NULL);
}

static gboolean
swfdec_movie_clip_push_audio (SwfdecMovieClip *movie)
{
  SwfdecSpriteFrame *frame;
  SwfdecBuffer *buf;

  if (g_queue_is_empty (movie->sound_queue)) {
    movie->sound_frame = movie->current_frame;
  }

  do {
    frame = &SWFDEC_SPRITE (movie->child)->frames[movie->sound_frame];
    if (frame->sound_block == NULL)
      return FALSE;
    SWFDEC_LOG ("decoding frame %u in frame %u now\n", movie->sound_frame, movie->current_frame);
    buf = swfdec_sound_decode_buffer (frame->sound_head, movie->sound_decoder, frame->sound_block);
    movie->sound_frame = swfdec_movie_clip_get_next_frame (movie, movie->sound_frame);
  } while (buf == NULL);
  g_assert (buf->length % 4 == 0);
  if (g_queue_is_empty (movie->sound_queue)) {
    if (frame->sound_skip < 0) {
      SWFDEC_ERROR ("help, i can't skip negative frames!");
    } else if ((guint) frame->sound_skip * 4 >= buf->length) {
      SWFDEC_INFO ("skipping whole frame?!");
      swfdec_buffer_unref (buf);
      return FALSE;
    } if (frame->sound_skip > 0) {
      SwfdecBuffer *tmp = swfdec_buffer_new_subbuffer (buf, frame->sound_skip * 4, 
	  buf->length - frame->sound_skip * 4);
      swfdec_buffer_unref (buf);
      buf = tmp;
    }
  }
  g_queue_push_tail (movie->sound_queue, buf);
  return TRUE;
}

void
swfdec_movie_clip_render_audio (SwfdecMovieClip *movie, gint16 *dest,
    guint n_samples)
{
  static const guint16 volume[2] = { G_MAXUINT16, G_MAXUINT16 };
  GList *g;
  unsigned int rendered;

  for (g = movie->list; g; g = g_list_next (g)) {
    swfdec_movie_clip_render_audio (g->data, dest, n_samples);
  }

  if (!SWFDEC_IS_SPRITE (movie->child))
    return;

  for (g = g_queue_peek_head_link (movie->sound_queue); g && n_samples > 0; g = g->next) {
    SwfdecBuffer *buffer = g->data;
    rendered = MIN (n_samples, buffer->length / 4);
    swfdec_sound_add (dest, (gint16 *) buffer->data, rendered, volume);
    n_samples -= rendered;
    dest += rendered * 2;
  }
  while (n_samples > 0 && swfdec_movie_clip_push_audio (movie)) {
    SwfdecBuffer *buffer = g_queue_peek_tail (movie->sound_queue);
    g_assert (buffer);
    rendered = MIN (n_samples, buffer->length / 4);
    swfdec_sound_add (dest, (gint16 *) buffer->data, rendered, volume);
    n_samples -= rendered;
    dest += rendered * 2;
  }
}
