#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <math.h>

#include <js/jsapi.h>

#include "swfdec_movieclip.h"
#include "swfdec_internal.h"
#include "swfdec_js.h"
#include "swfdec_edittext.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*** GOTO HANDLING ***/

typedef struct {
  SwfdecMovieClip *	movie;		/* movie that is supposed to jump */
  guint			frame;		/* frame to jump to */
  gboolean		do_enter_frame;	/* TRUE to execute enterFrame event */
} GotoEntry;

static GotoEntry *
goto_entry_new (SwfdecMovieClip *movie, guint frame, gboolean do_enter_frame)
{
  GotoEntry *entry = g_new (GotoEntry, 1);

  entry->movie = movie;
  entry->frame = frame;
  entry->do_enter_frame = do_enter_frame;
  return entry;
}

static void
goto_entry_free (GotoEntry *entry)
{
  g_free (entry);
}

static int
compare_goto (gconstpointer a, gconstpointer b)
{
  g_assert (SWFDEC_IS_MOVIE_CLIP (b));

  return ((const GotoEntry *) a)->movie == (SwfdecMovieClip *) b ? 0 : 1;
}

static void
swfdec_movie_clip_remove_gotos (SwfdecMovieClip *movie)
{
  SwfdecDecoder *dec;
  GList *item;

  dec = SWFDEC_OBJECT (movie)->decoder;
  while ((item = g_queue_find_custom (dec->gotos, movie, compare_goto))) {
    goto_entry_free (item->data);
    g_queue_delete_link (dec->gotos, item);
  }
}

void
swfdec_movie_clip_goto (SwfdecMovieClip *movie, guint frame, gboolean do_enter_frame)
{
  SwfdecDecoder *dec;
  GotoEntry *entry;

  g_assert (SWFDEC_IS_MOVIE_CLIP (movie));
  g_assert (SWFDEC_IS_SPRITE (movie->child));
  g_assert (frame < SWFDEC_SPRITE (movie->child)->n_frames);

  dec = SWFDEC_OBJECT (movie)->decoder;
  entry = goto_entry_new (movie, frame, do_enter_frame);
  SWFDEC_LOG ("queueing goto %u for %p %d", frame, movie, movie->child->id);
  g_queue_push_tail (dec->gotos, entry);
  movie->next_frame = frame;
}

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
  movie->content = &default_content;

  movie->xscale = 1.0;
  movie->yscale = 1.0;
  cairo_matrix_init_identity (&movie->transform);
  cairo_matrix_init_identity (&movie->inverse_transform);

  movie->visible = TRUE;
  movie->stopped = TRUE;

  movie->button_state = SWFDEC_BUTTON_UP;

  movie->sound_queue = g_queue_new ();
  swfdec_rect_init_empty (&SWFDEC_OBJECT (movie)->extents);
}

static void
swfdec_movie_clip_execute (SwfdecMovieClip *movie, unsigned int condition)
{
  if (movie->content->events == NULL)
    return;
  swfdec_event_list_execute (movie->content->events, movie, condition, 0);
}

static void
swfdec_movie_clip_invalidate (SwfdecMovieClip *movie)
{
  gboolean was_empty;
  SwfdecDecoder *s;
  SwfdecRect rect = SWFDEC_OBJECT (movie)->extents;

  SWFDEC_LOG ("invalidating %g %g  %g %g", rect.x0, rect.y0, rect.x1, rect.y1);
  while (movie->parent) {
    movie = movie->parent;
    if (movie->cache_state > SWFDEC_MOVIE_CLIP_INVALID_EXTENTS)
      return;
    swfdec_rect_transform (&rect, &rect, &movie->transform);
  }
  s = SWFDEC_OBJECT (movie)->decoder;
  was_empty = swfdec_rect_is_empty (&s->invalid);
  SWFDEC_DEBUG ("toplevel invalidation: %g %g  %g %g", rect.x0, rect.y0, rect.x1, rect.y1);
  swfdec_rect_union (&s->invalid, &s->invalid, &rect);
  if (was_empty)
    g_object_notify (G_OBJECT (s), "invalid");
}

/**
 * swfdec_movie_clip_queue_update:
 * @movie: a #SwfdecMovieClip
 * @state: how much needs to be updated
 *
 * Queues an update of all cached values inside @movie.
 **/
void
swfdec_movie_clip_queue_update (SwfdecMovieClip *movie, SwfdecMovieClipState state)
{
  g_return_if_fail (SWFDEC_IS_MOVIE_CLIP (movie));

  while (movie && movie->cache_state < state) {
    movie->cache_state = state;
    movie = movie->parent;
    state = SWFDEC_MOVIE_CLIP_INVALID_CHILDREN;
  }
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
  swfdec_rect_transform (extents, rect, &movie->transform);
  swfdec_rect_transform (rect, rect, &movie->content->transform);
  if (movie->parent && movie->parent->cache_state < SWFDEC_MOVIE_CLIP_INVALID_EXTENTS) {
    SwfdecRect tmp;
    swfdec_rect_transform (&tmp, extents, &movie->parent->transform);
    if (!swfdec_rect_inside (&SWFDEC_OBJECT (movie->parent)->extents, &tmp))
      swfdec_movie_clip_queue_update (movie->parent, SWFDEC_MOVIE_CLIP_INVALID_EXTENTS);
  }
}

static void
swfdec_movie_clip_update_matrix (SwfdecMovieClip *movie)
{
  cairo_matrix_t mat;

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
}

static void
swfdec_movie_clip_do_update (SwfdecMovieClip *movie)
{
  GList *walk;

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovieClip *child = walk->data;

    if (child->cache_state != SWFDEC_MOVIE_CLIP_UP_TO_DATE)
      swfdec_movie_clip_do_update (child);
  }

  switch (movie->cache_state) {
    case SWFDEC_MOVIE_CLIP_INVALID_CHILDREN:
      break;
    case SWFDEC_MOVIE_CLIP_INVALID_EXTENTS:
      swfdec_movie_clip_update_extents (movie);
      break;
    case SWFDEC_MOVIE_CLIP_INVALID_MATRIX:
      swfdec_movie_clip_update_matrix (movie);
      break;
    case SWFDEC_MOVIE_CLIP_UP_TO_DATE:
    default:
      g_assert_not_reached ();
  }
  if (movie->cache_state > SWFDEC_MOVIE_CLIP_INVALID_EXTENTS)
    swfdec_movie_clip_invalidate (movie);
  movie->cache_state = SWFDEC_MOVIE_CLIP_UP_TO_DATE;
}

/**
 * swfdec_movie_clip_update:
 * @movie: a #SwfdecMovieClip
 *
 * Brings the cached values of @movie up-to-date if they are not. This includes
 * transformation matrices, extents among others.
 **/
void
swfdec_movie_clip_update (SwfdecMovieClip *movie)
{
  g_return_if_fail (SWFDEC_IS_MOVIE_CLIP (movie));

  if (movie->cache_state == SWFDEC_MOVIE_CLIP_UP_TO_DATE)
    return;

  if (movie->parent && movie->parent->cache_state != SWFDEC_MOVIE_CLIP_UP_TO_DATE) {
    swfdec_movie_clip_update (movie->parent);
  } else {
    swfdec_movie_clip_do_update (movie);
  }
}

static void
swfdec_movie_clip_dispose (SwfdecMovieClip * movie)
{
  g_assert (movie->jsobj == NULL);

  swfdec_movie_clip_remove_gotos (movie);
  g_assert (movie->list == NULL);
  g_assert (movie->content == &default_content);
  g_assert (movie->child == NULL);

  g_queue_free (movie->sound_queue);
  if (movie->text_variables) {
    g_assert (g_hash_table_size (movie->text_variables) == 0);
    g_hash_table_destroy (movie->text_variables);
    movie->text_variables = NULL;
  }

  g_assert (movie->text == NULL);
  g_assert (movie->text_paragraph == NULL);

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
    if (movie->content->name && movie->jsobj)
      swfdec_js_movie_clip_remove_property (movie);
  }
  movie->content = content;
  if (movie->child != content->object)
    swfdec_movie_clip_set_child (movie, content->object);
  if (content->name && movie->parent && movie->parent->jsobj)
    swfdec_js_movie_clip_add_property (movie);

  swfdec_movie_clip_invalidate (movie);
  swfdec_movie_clip_queue_update (movie, SWFDEC_MOVIE_CLIP_INVALID_MATRIX);
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
  swfdec_movie_clip_invalidate (movie);
  movie->parent = NULL;
  g_object_unref (movie);
}

void
swfdec_movie_clip_remove_root (SwfdecMovieClip *root)
{
  g_return_if_fail (SWFDEC_IS_MOVIE_CLIP (root));
  g_return_if_fail (root->parent == NULL);

  while (root->list) {
    swfdec_movie_clip_remove (root->list->data);
  }
  swfdec_movie_clip_set_content (root, NULL);
  g_object_unref (root);
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

void
swfdec_movie_clip_iterate_audio (SwfdecMovieClip *movie)
{
  SwfdecBuffer *cur;
  SwfdecSpriteFrame *last;
  SwfdecSpriteFrame *current;
  unsigned int n_bytes;

  g_assert (SWFDEC_IS_SPRITE (movie->child));

  SWFDEC_LOG ("iterating audio (frame %u, last %u)", movie->current_frame, 
      movie->sound_current_frame);
  last = movie->sound_current_frame == (guint) -1 ? NULL : 
    &SWFDEC_SPRITE (movie->child)->frames[movie->sound_current_frame];
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
  } else if (swfdec_movie_clip_get_next_frame (movie, movie->sound_current_frame) != movie->current_frame) {
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
  movie->sound_current_frame = movie->current_frame;
}

static void
swfdec_movie_clip_goto_frame (SwfdecMovieClip *movie, unsigned int goto_frame, 
    gboolean do_enter_frame)
{
  GList *old, *walk;
  guint i, j, start;

  if (do_enter_frame)
    swfdec_movie_clip_execute (movie, SWFDEC_EVENT_ENTER);

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
  if (movie->current_frame == (guint) -1 || 
      SWFDEC_SPRITE (movie->child)->frames[goto_frame].bg_color != 
      SWFDEC_SPRITE (movie->child)->frames[movie->current_frame].bg_color) {
    swfdec_movie_clip_invalidate (movie);
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

gboolean
swfdec_decoder_do_goto (SwfdecDecoder *dec)
{
  GotoEntry *entry;

  entry = g_queue_pop_head (dec->gotos);
  if (entry == NULL)
    return FALSE;

  swfdec_movie_clip_goto_frame (entry->movie, entry->frame, entry->do_enter_frame);
  goto_entry_free (entry);
  return TRUE;
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

void
swfdec_movie_clip_queue_iterate (SwfdecMovieClip *movie)
{
  unsigned int goto_frame;

  if (!SWFDEC_IS_SPRITE (movie->child))
    return;
  if (!swfdec_movie_clip_should_iterate (movie))
    return;
  if (movie->stopped) {
    goto_frame = movie->next_frame;
  } else {
    goto_frame = swfdec_movie_clip_get_next_frame (movie, movie->next_frame);
  }
  swfdec_movie_clip_goto (movie, goto_frame, TRUE);
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
  SWFDEC_LOG ("moviclip %p mouse: %g %g", movie, x, y);
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
swfdec_movie_clip_render_child (SwfdecMovieClip *movie, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill)
{
  if (SWFDEC_IS_BUTTON (movie->child)) {
    swfdec_button_render (SWFDEC_BUTTON (movie->child), movie->button_state,
	cr, trans, inval, fill);
  } else if (SWFDEC_IS_EDIT_TEXT (movie->child)) {
    if (movie->text_paragraph)
      swfdec_edit_text_render (SWFDEC_EDIT_TEXT (movie->child), cr,
	  movie->text_paragraph, trans, inval, fill);
  } else {
    swfdec_object_render (movie->child, cr, trans, inval, fill);
  }
}

static void
swfdec_movie_clip_render (SwfdecObject *object, cairo_t *cr,
    const SwfdecColorTransform *color_transform, const SwfdecRect *inval, gboolean fill)
{
  SwfdecMovieClip *movie = SWFDEC_MOVIE_CLIP (object);
  GList *g;
  unsigned int clip_depth = 0;
  SwfdecColorTransform trans;
  SwfdecRect rect;

  /* FIXME: do proper clipping */
  if (movie->parent == NULL && fill) {
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
  swfdec_color_transform_chain (&trans, &movie->content->color_transform, color_transform);

  for (g = movie->list; g; g = g_list_next (g)) {
    SwfdecMovieClip *child = g->data;

    if (child->content->clip_depth) {
      if (clip_depth) {
	/* FIXME: is clipping additive? */
	SWFDEC_INFO ("unsetting clip depth %d for new clip depth", clip_depth);
	cairo_restore (cr);
	clip_depth = 0;
      }
      if (fill == FALSE) {
	SWFDEC_WARNING ("clipping inside clipping not implemented");
      } else {
	SWFDEC_INFO ("clipping up to depth %d by using %p with depth %d", child->content->clip_depth,
	    child, child->content->depth);
	clip_depth = child->content->clip_depth;
	cairo_save (cr);
	swfdec_object_render (SWFDEC_OBJECT (child), cr, &trans, &rect, FALSE);
	cairo_clip (cr);
	continue;
      }
    }

    if (clip_depth && child->content->depth > clip_depth) {
      SWFDEC_INFO ("unsetting clip depth %d for depth %d", clip_depth, child->content->depth);
      clip_depth = 0;
      cairo_restore (cr);
    }

    SWFDEC_LOG ("rendering %p with depth %d", child, child->content->depth);
    swfdec_object_render (SWFDEC_OBJECT (child), cr, &trans, &rect, fill);
  }
  if (clip_depth) {
    SWFDEC_INFO ("unsetting clip depth %d after rendering", clip_depth);
    clip_depth = 0;
    cairo_restore (cr);
  }
  if (movie->child) 
    swfdec_movie_clip_render_child (movie, cr, &trans, &rect, fill);
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

static SwfdecMovieClip *
swfdec_movie_clip_get_by_slash_path (SwfdecMovieClip *movie, const char *path)
{
  char *s;
  guint len;
  GList *walk;

  if (*path == '/') {
    path++;
    movie = SWFDEC_OBJECT (movie)->decoder->root;
  }
  do {
    s = strchr (path, '/');
    if (s)
      len = s - path;
    else
      len = strlen (path);
    if (len == 2 && path[0] == '.' && path[1] == '.') {
      movie = movie->parent;
      if (movie == NULL)
	return NULL;
    } else { 
      for (walk = movie->list; walk; walk = walk->next) {
	SwfdecMovieClip *child = walk->data;
	if (child->content->name == NULL)
	  continue;
	if (strncmp (child->content->name, path, len) != 0)
	  continue;
	movie = child;
	goto out;
      }
      return NULL;
    }
out:
    if (s)
      path = s + 1;
  } while (s);
  return movie;
}

static SwfdecMovieClip *
swfdec_movie_clip_get_by_path (SwfdecMovieClip *movie, const char *path)
{
  GList *walk;
  char *s;
  guint len;

  if (strchr (path, '/'))
    return swfdec_movie_clip_get_by_slash_path (movie, path);

  if (strcmp (path, "_root") == 0) {
    movie = SWFDEC_OBJECT (movie)->decoder->root;
    if (path[5] == '\0')
      return movie;
    path += 6;
  }
  do {
    s = strchr (path, '.');
    if (s)
      len = s - path;
    else
      len = strlen (path);
    if (len == 6 && 
	strcmp (path, "parent") == 0) {
      movie = movie->parent;
      if (movie == NULL)
	return NULL;
    } else { 
      for (walk = movie->list; walk; walk = walk->next) {
	SwfdecMovieClip *child = walk->data;
	if (child->content->name == NULL)
	  continue;
	if (strncmp (child->content->name, path, len) != 0)
	  continue;
	movie = child;
	goto out;
      }
      return NULL;
    }
out:
    if (s)
      path = s + 1;
  } while (s);
  return movie;
}

static void
swfdec_movie_clip_attach_variable (SwfdecMovieClip *parent, const char *variable,
    SwfdecMovieClip *movie)
{
  GList *list, *org;
  SWFDEC_LOG ("attaching %p's variable \"%s\" from movie %p", movie, variable,
      parent);
  if (parent->text_variables == NULL) 
    parent->text_variables = g_hash_table_new_full (g_str_hash, g_str_equal, 
	NULL, (GDestroyNotify) g_list_free);
  org = list = g_hash_table_lookup (parent->text_variables, variable);
  list = g_list_prepend (list, movie);
  if (list != org) {
    g_hash_table_steal (parent->text_variables, variable);
    g_hash_table_insert (parent->text_variables, (gpointer) variable, list);
  }
  if (parent->jsobj) {
    jsval val;
    SwfdecDecoder *dec = SWFDEC_OBJECT (parent)->decoder;
    if (JS_LookupProperty (dec->jscx, parent->jsobj,
	  variable, &val)) {
      if (val == JSVAL_VOID) {
	JSString *string = JS_NewStringCopyZ (dec->jscx, movie->text ? movie->text : "");
	if (string) {
	  val = STRING_TO_JSVAL (string);
	  JS_SetProperty (dec->jscx, parent->jsobj, variable, &val);
	}
      } else {
	const char *bytes = swfdec_js_to_string (dec->jscx, val);
	if (bytes)
	  swfdec_movie_clip_set_text (movie, bytes);
      }
    }
  }
}

static void
swfdec_movie_clip_detach_variable (SwfdecMovieClip *parent, const char *variable,
    SwfdecMovieClip *movie)
{
  GList *list, *org;

  SWFDEC_LOG ("detaching %p's variable \"%s\" from movie %p", movie, variable, parent);
  g_assert (parent->text_variables);
  org = list = g_hash_table_lookup (parent->text_variables, variable);
  g_assert (list);
  list = g_list_remove (list, movie);
  if (list != org) {
    g_hash_table_steal (parent->text_variables, variable);
    g_hash_table_insert (parent->text_variables, (gpointer) variable, list);
  }
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
    if (SWFDEC_IS_EDIT_TEXT (movie->child)) { 
      SwfdecEditText *text = SWFDEC_EDIT_TEXT (movie->child);

      /* FIXME: movie->parent == NULL only happens when every movie gets destroyed, or??? */
      if (text->variable) {
	SwfdecMovieClip *parent;
	if (text->path)
	  parent = swfdec_movie_clip_get_by_path (movie->parent, text->path);
	else
	  parent = movie->parent;
	if (parent != NULL)
	  swfdec_movie_clip_detach_variable (parent, text->variable, movie);
      }
      g_free (movie->text);
      movie->text = NULL;
      if (movie->text_paragraph) {
	swfdec_paragraph_free (movie->text_paragraph);
	movie->text_paragraph = NULL;
      }
    }
    g_object_unref (movie->child);
  }
  movie->child = child;
  movie->current_frame = -1;
  movie->next_frame = 0;
  movie->sound_current_frame = -1;
  movie->stopped = FALSE;
  if (child) {
    g_object_ref (child);
    if (SWFDEC_IS_SPRITE (child)) {
      s->movies = g_list_prepend (s->movies, movie);
      swfdec_movie_clip_goto_frame (movie, 0, FALSE);
      swfdec_movie_clip_iterate_audio (movie);
    }
    if (SWFDEC_IS_EDIT_TEXT (child)) {
      SwfdecEditText *text = SWFDEC_EDIT_TEXT (child);
      
      if (text->text)
	swfdec_movie_clip_set_text (movie, text->text);
      if (text->variable != NULL) {
	SwfdecMovieClip *parent;
	if (text->path)
	  parent = swfdec_movie_clip_get_by_path (movie->parent, text->path);
	else
	  parent = movie->parent;
	if (parent != NULL)
	  swfdec_movie_clip_attach_variable (parent, text->variable, movie);
      }
    }
  }
  swfdec_movie_clip_invalidate (movie);
  swfdec_movie_clip_queue_update (movie, SWFDEC_MOVIE_CLIP_INVALID_EXTENTS);
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

void
swfdec_movie_clip_set_text (SwfdecMovieClip *movie, const char *text)
{
  SwfdecEditText *edittext;

  g_assert (SWFDEC_IS_EDIT_TEXT (movie->child));
  g_assert (text != NULL);

  edittext = SWFDEC_EDIT_TEXT (movie->child);
  SWFDEC_LOG ("setting %s's text to \"%s\"", 
      edittext->variable ? edittext->variable : "???", text);

  if (movie->text_paragraph)
    swfdec_paragraph_free (movie->text_paragraph);
  g_free (movie->text);
  movie->text = g_strdup (text);
  if (edittext->html)
    movie->text_paragraph = swfdec_paragraph_html_parse (edittext, text);
  else
    movie->text_paragraph = swfdec_paragraph_text_parse (edittext, text);
  swfdec_movie_clip_invalidate (movie);
}
