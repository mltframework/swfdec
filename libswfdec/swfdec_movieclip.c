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

static const SwfdecSpriteContent default_content = {
  NULL, -1, 0, 0, { 1.0, 0.0, 0.0, 1.0, 0.0, 0.0 },
  { { 1.0, 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0, 0.0 } },
  NULL, NULL, NULL
};

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

  movie->visible = TRUE;
  movie->stopped = TRUE;

  movie->button_state = SWFDEC_BUTTON_UP;

  movie->sound_queue = g_queue_new ();
}

static void
swfdec_movie_clip_execute (SwfdecMovieClip *movie, unsigned int condition)
{
  if (movie->content->events == NULL)
    return;
  swfdec_event_list_execute (movie->content->events, movie, condition, 0);
}

static void
swfdec_movie_clip_invalidate (SwfdecMovieClip *movie, const SwfdecRect *rect)
{
  SwfdecRect tmp;
  SWFDEC_LOG ("invalidating %g %g  %g %g", rect->x0, rect->y0, rect->x1, rect->y1);
  swfdec_rect_transform (&tmp, rect, &movie->transform);
  while (movie->parent) {
    movie = movie->parent;
    swfdec_rect_transform (&tmp, &tmp, &movie->transform);
  }
  swfdec_object_invalidate (SWFDEC_OBJECT (movie), &tmp);
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
  swfdec_rect_transform (rect, rect, &movie->content->transform);
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
  cairo_matrix_multiply (&mat, &mat, &movie->content->transform);
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
get_rid_of_children (gpointer moviep, gpointer unused)
{
  SwfdecMovieClip *movie = moviep;

  movie->parent = NULL;
  g_object_unref (movie);
}

static void
swfdec_movie_clip_dispose (SwfdecMovieClip * movie)
{
  g_assert (movie->jsobj == NULL);

  g_list_foreach (movie->list, get_rid_of_children, NULL);
  g_list_free (movie->list);
  movie->list = NULL;

  if (movie->content != &default_content)
    swfdec_movie_clip_set_content (movie, NULL);
  g_assert (movie->child == NULL);

  g_queue_free (movie->sound_queue);

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (movie));
}

static SwfdecMovieClip *
swfdec_movie_clip_find (GList *movie_list, guint depth)
{
  GList *walk;

  for (walk = movie_list; walk; walk = walk->next) {
    SwfdecMovieClip *movie = walk->data;

    if (movie->content->depth < depth)
      continue;
    if (movie->content->depth == depth)
      return movie;
    break;
  }
  return NULL;
}

void
swfdec_movie_clip_set_content (SwfdecMovieClip *movie, const SwfdecSpriteContent *content)
{
  if (content == NULL)
    content = &default_content;

  if (movie->content) {
    if (movie->parent)
      swfdec_movie_clip_invalidate (movie->parent, &SWFDEC_OBJECT (movie)->extents);
    if (movie->content->name && movie->jsobj)
      swfdec_js_movie_clip_remove_property (movie);
  }
  movie->content = content;
  if (movie->child != content->object)
    swfdec_movie_clip_set_child (movie, content->object);
  if (content->name && movie->parent && movie->parent->jsobj)
    swfdec_js_movie_clip_add_property (movie);

  /* do all the necessary setup stuff */
  swfdec_movie_clip_update_matrix (movie, FALSE);
  if (movie->parent)
    swfdec_movie_clip_invalidate (movie->parent, &SWFDEC_OBJECT (movie)->extents);
}

static void
swfdec_movie_clip_remove (SwfdecMovieClip *movie)
{
  g_assert (movie->parent);
  while (movie->list) {
    swfdec_movie_clip_remove (movie->list->data);
  }
  /* FIXME: order? */
  swfdec_movie_clip_execute (movie, SWFDEC_EVENT_UNLOAD);
  swfdec_movie_clip_set_content (movie, NULL);
  movie->parent->list = g_list_remove (movie->parent->list, movie);
  movie->parent = NULL;
  g_object_unref (movie);
}

static gboolean
swfdec_movie_clip_remove_child (SwfdecMovieClip *movie, guint depth)
{
  SwfdecMovieClip *child = swfdec_movie_clip_find (movie->list, depth);

  if (child == NULL)
    return FALSE;

  swfdec_movie_clip_remove (child);
  return TRUE;
}

static int
swfdec_movie_clip_compare_depths (gconstpointer a, gconstpointer b)
{
  return (int) (SWFDEC_MOVIE_CLIP (a)->content->depth - SWFDEC_MOVIE_CLIP (b)->content->depth);
}

static void
swfdec_movie_clip_perform_one_action (SwfdecMovieClip *movie, SwfdecSpriteAction *action,
    gboolean skip_scripts, GList **movie_list)
{
  SwfdecMovieClip *child;
  SwfdecSpriteContent *content;

  switch (action->type) {
    case SWFDEC_SPRITE_ACTION_SCRIPT:
      SWFDEC_LOG ("SCRIPT action");
      if (!skip_scripts)
	swfdec_js_execute_script (SWFDEC_OBJECT (movie)->decoder, movie, action->data, NULL);
      break;
    case SWFDEC_SPRITE_ACTION_ADD:
      content = action->data;
      SWFDEC_LOG ("ADD action: depth %u", content->depth);
      if (swfdec_movie_clip_remove_child (movie, content->depth))
	SWFDEC_DEBUG ("removed a child before adding new one");
      child = swfdec_movie_clip_find (*movie_list, content->depth);
      if (child == NULL || child->content->sequence != content->sequence) {
	child = swfdec_movie_clip_new (movie, content);
      } else {
	swfdec_movie_clip_set_content (child, content);
	*movie_list = g_list_remove (*movie_list, child);
	movie->list = g_list_insert_sorted (movie->list, child, swfdec_movie_clip_compare_depths);
      }
      break;
    case SWFDEC_SPRITE_ACTION_UPDATE:
      content = action->data;
      SWFDEC_LOG ("UPDATE action: depth %u", content->depth);
      child = swfdec_movie_clip_find (movie->list, content->depth);
      if (child != NULL) {
	swfdec_movie_clip_set_content (child, content);
      } else {
	SWFDEC_WARNING ("supposed to update depth %u, but no child", content->depth);
      }
      break;
    case SWFDEC_SPRITE_ACTION_REMOVE:
      SWFDEC_LOG ("REMOVE action: depth %u", GPOINTER_TO_UINT (action->data));
      if (!swfdec_movie_clip_remove_child (movie, GPOINTER_TO_UINT (action->data)))
	SWFDEC_WARNING ("could not remove, no child at depth %u", GPOINTER_TO_UINT (action->data));
      break;
    default:
      g_assert_not_reached ();
  }
}

unsigned int
swfdec_movie_clip_get_next_frame (SwfdecMovieClip *movie, unsigned int current_frame)
{
  unsigned int next_frame, n_frames;

  g_assert (SWFDEC_IS_SPRITE (movie->child));

  n_frames = MIN (SWFDEC_SPRITE (movie->child)->n_frames, 
		  SWFDEC_SPRITE (movie->child)->parse_frame);
  next_frame = current_frame + 1;
  if (next_frame >= n_frames)
    next_frame = 0;
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

  SWFDEC_LOG ("iterating audio (frame %u, last %u)", movie->current_frame, last_frame);
  last = last_frame == (guint) -1 ? NULL : &SWFDEC_SPRITE (movie->child)->frames[last_frame];
  current = &SWFDEC_SPRITE (movie->child)->frames[movie->current_frame];
  if (last == NULL || last->sound_head != current->sound_head) {
    /* empty queue */
    while ((cur = g_queue_pop_head (movie->sound_queue)))
      swfdec_buffer_unref (cur);
    if (movie->sound_decoder) {
      g_assert (last);
      SWFDEC_LOG ("finishing sound decoder in frame %u", movie->current_frame);
      swfdec_sound_finish_decoder (last->sound_head, movie->sound_decoder);
    }
    if (current->sound_head) {
      movie->sound_decoder = swfdec_sound_init_decoder (current->sound_head);
      SWFDEC_LOG ("initialized sound decoder in frame %u", movie->current_frame);
    } else {
      movie->sound_decoder = NULL;
    }
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
swfdec_movie_clip_goto_frame (SwfdecMovieClip *movie, unsigned int goto_frame)
{
  GList *old, *walk;
  guint i, j, start;

  if (goto_frame == movie->current_frame)
    return;

  if (goto_frame < movie->current_frame) {
    start = 0;
    old = movie->list;
    movie->list = NULL;
  } else {
    start = movie->current_frame + 1;
    old = NULL;
  }
  movie->current_frame = goto_frame;
  SWFDEC_DEBUG ("iterating from %u to %u", start, movie->current_frame);
  for (i = start; i <= movie->current_frame; i++) {
    SwfdecSpriteFrame *frame = &SWFDEC_SPRITE (movie->child)->frames[i];
    if (frame->actions == NULL)
      continue;
    for (j = 0; j < frame->actions->len; j++) {
      swfdec_movie_clip_perform_one_action (movie,
	  &g_array_index (frame->actions, SwfdecSpriteAction, j),
	  i != movie->current_frame, &old);
    }
  }
  /* FIXME: not sure about the order here, might be relevant for unload events */
  for (walk = old; walk; walk = walk->next) {
    swfdec_movie_clip_remove (walk->data);
  }
  g_list_free (old);
}

static void
swfdec_movie_clip_update_current_frame (SwfdecMovieClip *movie)
{
  unsigned int last_frame, next_frame;

  if (!SWFDEC_IS_SPRITE (movie->child))
    return;
  if (!movie->stopped) {
    movie->next_frame = swfdec_movie_clip_get_next_frame (movie, movie->next_frame);
  }
  next_frame = movie->next_frame;
  last_frame = movie->current_frame;
  swfdec_movie_clip_execute (movie, SWFDEC_EVENT_ENTER);
  swfdec_movie_clip_goto_frame (movie, next_frame);
  while (movie->current_frame != movie->next_frame) {
    swfdec_movie_clip_goto_frame (movie, movie->next_frame);
  }
  if (last_frame == (guint) -1 || 
      SWFDEC_SPRITE (movie->child)->frames[last_frame].bg_color != 
      SWFDEC_SPRITE (movie->child)->frames[movie->current_frame].bg_color) {
    swfdec_object_invalidate (SWFDEC_OBJECT (movie), NULL);
  }
  swfdec_movie_clip_iterate_audio (movie, last_frame);
}

static gboolean
swfdec_movie_clip_should_iterate (SwfdecMovieClip *movie)
{
  guint parent_frame;
  SwfdecMovieClip *parent;
  /* movies that are scheduled for removal this frame don't do anything */
  /* FIXME: This function is complicated. It doesn't seem to be as stupid as
   * Flash normally is. I suppose there's a simpler solution */

  /* root movie always gets executed */
  if (movie->parent == NULL)
    return TRUE;

  parent = movie->parent;
  g_assert (SWFDEC_IS_SPRITE (parent->child));
  if (parent->stopped) {
    parent_frame = parent->next_frame;
  } else {
    parent_frame = swfdec_movie_clip_get_next_frame (parent, parent->next_frame);
  }
  /* check if we'll get removed until parent's next frame */
  if (parent_frame < movie->content->sequence->start ||
      parent_frame >= movie->content->sequence->end)
    return FALSE;
  /* check if parent will get removed */
  return swfdec_movie_clip_should_iterate (parent);
}

/* Iterating is a sensitive thing because a lot of scripts are excuted during 
 * iteration, in particular the DoInitAction, DoAction, enterFrame, load and 
 * unload scripts.
 * If you change anything here, please make sure that these orders are still 
 * correct. And run the testsuite. :)
 */
void 
swfdec_movie_clip_iterate (SwfdecMovieClip *movie)
{
  g_assert (SWFDEC_IS_MOVIE_CLIP (movie));

  if (!swfdec_movie_clip_should_iterate (movie))
    return;

  swfdec_movie_clip_update_current_frame (movie);
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
    if (child->content->clip_depth) {
      SWFDEC_LOG ("clip_depth=%d", child->content->clip_depth);
      clip_depth = child->content->clip_depth;
    }

    if (clip_depth && child->content->depth <= clip_depth) {
      SWFDEC_DEBUG ("clipping depth=%d", child->content->clip_depth);
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
  if (movie->content)
    swfdec_color_transform_chain (&trans, &movie->content->color_transform, color_transform);

  for (g = movie->list; g; g = g_list_next (g)) {
    SwfdecMovieClip *child = g->data;

    if (child->content->clip_depth) {
      SWFDEC_INFO ("clip_depth=%d", child->content->clip_depth);
      clip_depth = child->content->clip_depth;
    }

    if (clip_depth && child->content->depth <= clip_depth) {
      SWFDEC_INFO ("clipping depth=%d", child->content->clip_depth);
      continue;
    }

    SWFDEC_LOG ("rendering %p with depth %d", child, child->content->depth);
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
 * @content: the contents to display
 *
 * Creates a new #SwfdecMovieClip as a child of @parent with the given content.
 * @parent must not contain a movie clip at the depth specified by @content.
 *
 * Returns: a new #SwfdecMovieClip
 **/
SwfdecMovieClip *
swfdec_movie_clip_new (SwfdecMovieClip *parent, SwfdecSpriteContent *content)
{
  SwfdecMovieClip *ret;

  g_return_val_if_fail (SWFDEC_IS_MOVIE_CLIP (parent), NULL);

  SWFDEC_DEBUG ("new movie for parent %p", parent);
  ret = swfdec_object_new (SWFDEC_OBJECT (parent)->decoder, SWFDEC_TYPE_MOVIE_CLIP);
  ret->parent = parent;
  swfdec_movie_clip_set_content (ret, content);
  if (parent)
    ret->parent->list = g_list_insert_sorted (ret->parent->list, ret,
	swfdec_movie_clip_compare_depths);
  swfdec_movie_clip_execute (ret, SWFDEC_EVENT_LOAD);

  return ret;
}

void
swfdec_movie_clip_set_child (SwfdecMovieClip *movie, SwfdecObject *child)
{
  SwfdecDecoder *s;

  g_return_if_fail (SWFDEC_IS_MOVIE_CLIP (movie));
  g_return_if_fail (child == NULL || SWFDEC_IS_OBJECT (child));
  /* we oonly set or unset here, changing isn't supported */
  g_assert (movie->child == NULL || child == NULL);

  s = SWFDEC_OBJECT (movie)->decoder;
  if (movie->child) {
    if (SWFDEC_IS_SPRITE (movie->child)) {
      SwfdecBuffer *cur;
      SwfdecSpriteFrame *frame;
      g_assert (movie->list == NULL);
      frame = &SWFDEC_SPRITE (movie->child)->frames[movie->current_frame];
      if (movie->sound_decoder)
	swfdec_sound_finish_decoder (frame->sound_head, movie->sound_decoder);
      movie->sound_decoder = NULL;
      /* empty sound queue */
      while ((cur = g_queue_pop_head (movie->sound_queue)))
	swfdec_buffer_unref (cur);
      s->movies = g_list_remove (s->movies, movie);
    }
    if (g_signal_handlers_disconnect_by_func (movie->child, 
	swfdec_movie_clip_invalidate_cb, movie) != 1) {
      g_assert_not_reached ();
    }
    g_object_unref (movie->child);
  }
  movie->child = child;
  movie->current_frame = -1;
  movie->next_frame = 0;
  movie->stopped = TRUE;
  if (child) {
    g_object_ref (child);
    g_signal_connect (child, "invalidate", 
	G_CALLBACK (swfdec_movie_clip_invalidate_cb), movie);
    if (SWFDEC_IS_SPRITE (child)) {
      movie->stopped = FALSE;
      s->movies = g_list_prepend (s->movies, movie);
      swfdec_movie_clip_goto_frame (movie, 0);
    }
    swfdec_movie_clip_update_extents (movie);
  }
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
