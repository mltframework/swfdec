#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <math.h>

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
  movie->xscale = 1.0;
  movie->yscale = 1.0;
  cairo_matrix_init_identity (&movie->transform);
  cairo_matrix_init_identity (&movie->inverse_transform);

  swfdec_color_transform_init_identity (&movie->color_transform);

  movie->visible = TRUE;

  movie->button_state = SWFDEC_BUTTON_UP;
  movie->current_frame = -1;

  movie->sound_queue = g_queue_new ();
}

static void
swfdec_movie_clip_execute (SwfdecMovieClip *movie, unsigned int condition)
{
  if (movie->events == NULL)
    return;
  swfdec_event_list_execute (movie->events, movie, condition, 0);
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
  SwfdecRect *rect = &movie->original_extents;
  SwfdecRect *extents = &SWFDEC_OBJECT (movie)->extents;

  /* treat root movie special */
  if (movie->parent == NULL)
    return;

  if (movie->child == NULL) {
    swfdec_rect_init_empty (rect);
  } else if (SWFDEC_IS_SPRITE (movie->child)) {
    /* NB: this assumes that the children's extents are up-to-date */
    GList *walk;
    swfdec_rect_init_empty (rect);
    for (walk = movie->list; walk; walk = walk->next) {
      swfdec_rect_union (rect, rect, &SWFDEC_OBJECT (walk->data)->extents);
    }
  } else {
    *rect = movie->child->extents;
  }
  *extents = *rect;
  swfdec_rect_transform (rect, rect, &movie->original_transform);
  swfdec_rect_transform (extents, extents, &movie->transform);
}

/**
 * swfdec_movie_clip_update_matrix:
 * @movie: a #SwfdecMovieClip
 * @invalidate: TRUE to invalidate the area occupied by this movieclip
 *
 * Updates the transformation matrices that are used within this movieclip.
 **/
void
swfdec_movie_clip_update_matrix (SwfdecMovieClip *movie, gboolean invalidate)
{
  SwfdecRect rect;
  cairo_matrix_t mat;

  g_return_if_fail (SWFDEC_IS_MOVIE_CLIP (movie));

  if (invalidate)
    rect = SWFDEC_OBJECT (movie)->extents;

  cairo_matrix_init_translate (&mat, movie->x, movie->y);
  cairo_matrix_scale (&mat, movie->xscale, movie->yscale);
  cairo_matrix_rotate (&mat, movie->rotation * M_PI / 180);
  cairo_matrix_multiply (&mat, &mat, &movie->original_transform);
  movie->transform = mat;
  movie->inverse_transform = mat;
  if (cairo_matrix_invert (&movie->inverse_transform)) {
    g_assert_not_reached ();
  }
  swfdec_movie_clip_update_extents (movie);
  if (invalidate) {
    swfdec_rect_union (&rect, &rect, &SWFDEC_OBJECT (movie)->extents);
    if (movie->parent) {
      swfdec_movie_clip_invalidate (movie->parent, &rect);
    } else {
      swfdec_object_invalidate (SWFDEC_OBJECT (movie), &rect);
    }
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
swfdec_movie_clip_perform_actions (SwfdecMovieClip *movie, guint last_frame, SwfdecRect *after)
{
  SwfdecMovieClip *cur;
  SwfdecRect before;
  SwfdecSpriteFrame *frame = &SWFDEC_SPRITE (movie->child)->frames[movie->current_frame];
  GList *walk, *walk2; /* I suck at variable naming */
  GSList *swalk;

  /* note about the invalidation handling:
   * We have to handle 2 invalidations: stuff that is removed (before) and stuff that 
   * is added. (after) Since the extents have to be updated before we can invalidate the 
   * added regions (or some regions may be missed) we have to do this invalidation very
   * late, especially because children may change their extents, too.
   */
  if (last_frame == movie->current_frame)
    return;
  if (last_frame == (guint) -1 || frame->bg_color != SWFDEC_SPRITE (movie->child)->frames[last_frame].bg_color)
    before = SWFDEC_OBJECT (movie)->extents;
  else
    swfdec_rect_init_empty (&before);
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
      swfdec_rect_union (&before, &before, &SWFDEC_OBJECT (cur)->extents);
      if (cur->name && cur->jsobj)
	swfdec_js_movie_clip_remove_property (cur);
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
      g_assert (!new);
      continue;
    }
    /* invalidate the current element unless it's new */
    if (!new)
      swfdec_rect_union (&before, &before, &SWFDEC_OBJECT (cur)->extents);

    /* copy all the relevant stuff */
    if (cur->child != contents->object)
      swfdec_movie_clip_set_child (cur, contents->object);
    cur->clip_depth = contents->clip_depth;
    cur->ratio = contents->ratio;
    cur->original_transform = contents->transform;
    if (cur->name && cur->jsobj)
      swfdec_js_movie_clip_remove_property (cur);
    cur->name = contents->name;
    cur->events = contents->events;
    if (cur->name && movie->jsobj)
      swfdec_js_movie_clip_add_property (cur);
    if (new)
      swfdec_movie_clip_execute (cur, SWFDEC_EVENT_LOAD);

    /* do all the necessary setup stuff */
    swfdec_movie_clip_update_matrix (cur, FALSE);
    swfdec_rect_union (after, after, &SWFDEC_OBJECT (cur)->extents);
  }
    
  while (walk2) {
    cur = walk2->data;
    swfdec_rect_union (&before, &before, &SWFDEC_OBJECT (cur)->extents);
    if (cur->name && cur->jsobj)
      swfdec_js_movie_clip_remove_property (cur);
    swfdec_object_unref (cur);
    walk2 = walk2->next;
    movie->list = g_list_remove (movie->list, cur);
  }
  for (swalk = frame->sound; swalk; swalk = swalk->next) {
    swfdec_audio_event_init (SWFDEC_OBJECT (movie)->decoder, swalk->data);
  }
  if (!swfdec_rect_is_empty (&before)) {
    swfdec_movie_clip_invalidate (movie, &before);
  }
  /* FIXME: where are these really executed? */
  if (frame->do_actions) {
    GSList *swalk;
    for (swalk = frame->do_actions; swalk; swalk = swalk->next)
      swfdec_decoder_queue_script (SWFDEC_OBJECT (movie)->decoder, movie, swalk->data);
  }
  /* if we did everything right, we should now have as many children as the frame says */
  g_assert (g_list_length (movie->list) == g_list_length (frame->contents));
}

unsigned int
swfdec_movie_clip_get_next_frame (SwfdecMovieClip *movie, unsigned int current_frame)
{
  unsigned int next_frame, n_frames;

  g_assert (SWFDEC_IS_SPRITE (movie->child));

  n_frames = MIN (SWFDEC_SPRITE (movie->child)->n_frames, 
		  SWFDEC_SPRITE (movie->child)->parse_frame);
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
  SwfdecSpriteFrame *last;
  SwfdecSpriteFrame *current;
  unsigned int n_bytes;

  g_assert (SWFDEC_IS_SPRITE (movie->child));

  last = last_frame == (guint) -1 ? NULL : &SWFDEC_SPRITE (movie->child)->frames[last_frame];
  current = &SWFDEC_SPRITE (movie->child)->frames[movie->current_frame];
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

static void
swfdec_movie_clip_iterate_queue_enter_frame (SwfdecMovieClip *movie)
{
  GList *walk;

  if (!movie->stopped && SWFDEC_IS_SPRITE (movie->child)) {
    movie->next_frame = swfdec_movie_clip_get_next_frame (movie, movie->next_frame);
  }
  swfdec_movie_clip_execute (movie, SWFDEC_EVENT_ENTER);
  for (walk = movie->list; walk; walk = walk->next) {
    swfdec_movie_clip_iterate_queue_enter_frame (walk->data);
  }
}

static void
swfdec_movie_clip_iterate_update (SwfdecMovieClip *movie)
{
  GList *walk;
  unsigned int last_frame;
  SwfdecRect after;

  swfdec_rect_init_empty (&after);
  SWFDEC_LOG ("iterate, frame_index = %d", movie->next_frame);
  if (SWFDEC_IS_SPRITE (movie->child)) {
    last_frame = movie->current_frame;
    movie->current_frame = movie->next_frame;
    swfdec_movie_clip_perform_actions (movie, last_frame, &after);
    swfdec_movie_clip_iterate_audio (movie, last_frame);
  }
  for (walk = movie->list; walk; walk = walk->next) {
    swfdec_movie_clip_iterate_update (walk->data);
  }
  swfdec_movie_clip_update_extents (movie);
  if (!swfdec_rect_is_empty (&after))
    swfdec_movie_clip_invalidate (movie, &after);
}

void 
swfdec_movie_clips_iterate (SwfdecDecoder *dec)
{
  /* first, queue all enterFrame events and execute them. */
  g_assert (swfdec_js_script_queue_is_empty (dec));
  swfdec_movie_clip_iterate_queue_enter_frame (dec->root);
  swfdec_decoder_execute_scripts (dec);

  /* second, update the visual state of all movies and execute load/unload events */
  g_assert (swfdec_js_script_queue_is_empty (dec));
  swfdec_movie_clip_iterate_update (dec->root);
  swfdec_decoder_execute_scripts (dec);
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
#if 0
  {
    double x = 1.0, y = 0.0;
    cairo_transform (cr, &movie->inverse_transform);
    cairo_user_to_device_distance (cr, &x, &y);
    cairo_set_source_rgb (cr, 1.0, 0.0, 0.0);
    cairo_set_line_width (cr, 1 / sqrt (x * x + y * y));
    cairo_rectangle (cr, object->extents.x0 + 10, object->extents.y0 + 10,
	object->extents.x1 - object->extents.x0 - 20,
	object->extents.y1 - object->extents.y0 - 20);
    cairo_stroke (cr);
  }
#endif
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
    swfdec_object_unref (movie->child);
  }
  movie->child = child;
  if (child) {
    g_object_ref (child);
    g_signal_connect (child, "invalidate", 
	G_CALLBACK (swfdec_movie_clip_invalidate_cb), movie);
    swfdec_movie_clip_update_extents (movie);
  }
  movie->current_frame = -1;
  movie->next_frame = 0;
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
