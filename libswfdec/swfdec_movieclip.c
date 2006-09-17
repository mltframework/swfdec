#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>

#include <js/jsapi.h>

#include "swfdec_movieclip.h"
#include "swfdec_internal.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*** DISPLAY LIST ***/

static void
swfdec_display_list_init (SwfdecDisplayList *list)
{
  list->list = NULL;
}

static void
swfdec_display_list_reset (SwfdecDisplayList *list)
{
  g_list_foreach (list->list, (GFunc) g_object_unref, NULL);
  g_list_free (list->list);
  list->list = NULL;
}

static SwfdecMovieClip *
swfdec_display_list_add (SwfdecDisplayList *list, int depth, SwfdecMovieClip *movie)
{
  GList *walk;

  movie->depth = depth;
  for (walk = list->list; walk; walk = walk->next) {
    SwfdecMovieClip *cur = walk->data;
    if (cur->depth < depth)
      continue;
    if (cur->depth > depth) {
      list->list = g_list_insert_before (list->list, walk, movie);
      return NULL;
    }
    /* if (cur->depth == depth) */
    walk->data = movie;
    return cur;
  }
  list->list = g_list_append (list->list, movie);
  return NULL;
}

static SwfdecMovieClip *
swfdec_display_list_get (SwfdecDisplayList *list, int depth)
{
  GList *walk;

  for (walk = list->list; walk; walk = walk->next) {
    SwfdecMovieClip *cur = walk->data;
    if (cur->depth > depth)
      return NULL;
    if (cur->depth == depth)
      return cur;
  }
  return NULL;
}

static SwfdecMovieClip *
swfdec_display_list_remove (SwfdecDisplayList *list, int depth)
{
  GList *walk;

  for (walk = list->list; walk; walk = walk->next) {
    SwfdecMovieClip *cur = walk->data;
    if (cur->depth > depth)
      return NULL;
    if (cur->depth == depth) {
      list->list = g_list_delete_link (list->list, walk);
      return cur;
    }
  }
  return NULL;

}

/*** MOVIE CLIP ***/

SWFDEC_OBJECT_BOILERPLATE (SwfdecMovieClip, swfdec_movie_clip)

static void swfdec_movie_clip_base_init (gpointer g_class)
{

}

static void
swfdec_movie_clip_init (SwfdecMovieClip * movie)
{
  swfdec_display_list_init (&movie->list);

  movie->xscale = 100;
  movie->yscale = 100;
  cairo_matrix_init_identity (&movie->transform);
  cairo_matrix_init_identity (&movie->inverse_transform);

  swfdec_color_transform_init_identity (&movie->color_transform);

  movie->visible = TRUE;

  movie->button_state = SWFDEC_BUTTON_UP;
}

static void
swfdec_movie_clip_dispose (SwfdecMovieClip * movie)
{
  swfdec_display_list_reset (&movie->list);

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (movie));
}

static void
swfdec_movie_clip_perform_actions (SwfdecMovieClip *movie)
{
  SwfdecSpriteAction *action;
  guint i;
  SwfdecMovieClip *cur = NULL, *tmp;
  SwfdecRect inval;
  SwfdecSpriteFrame *frame = &SWFDEC_SPRITE (movie->child)->frames[movie->current_frame];

  if (frame->actions != NULL) {
    swfdec_rect_init_empty (&inval);
    for (i = 0; i < frame->actions->len; i++) {
      action = &g_array_index (frame->actions, SwfdecSpriteAction, i);
      SWFDEC_LOG ("performing action %d\n", action->type);
      switch (action->type) {
	case SWFDEC_SPRITE_ACTION_REMOVE_OBJECT:
	  tmp = swfdec_display_list_remove (&movie->list, action->uint.value[0]);
	  if (tmp) {
	    swfdec_rect_union (&inval, &inval, &SWFDEC_OBJECT (tmp)->extents);
	    g_object_unref (tmp);
	    if (tmp == movie->mouse_grab)
	      movie->mouse_grab = NULL;
	  }
	  break;
	case SWFDEC_SPRITE_ACTION_PLACE_OBJECT:
	  if (cur) {
	    swfdec_rect_union (&inval, &inval, &SWFDEC_OBJECT (cur)->extents);
	  }
	  cur = swfdec_movie_clip_new (movie, action->uint.value[0]);
	  tmp = swfdec_display_list_add (&movie->list, action->uint.value[1], cur);
	  if (tmp) {
	    swfdec_rect_union (&inval, &inval, &SWFDEC_OBJECT (tmp)->extents);
	    g_object_unref (tmp);
	  }
	  break;
	case SWFDEC_SPRITE_ACTION_GET_OBJECT:
	  if (cur) {
	    swfdec_rect_union (&inval, &inval, &SWFDEC_OBJECT (cur)->extents);
	  }
	  cur = swfdec_display_list_get (&movie->list, action->uint.value[1]);
	  break;
	case SWFDEC_SPRITE_ACTION_TRANSFORM:
	  if (cur) {
	    cur->inverse_transform = cur->transform = action->matrix.matrix;
	    /* FIXME: calculate x, y, rotation, xscale, yscale */
	    if (cairo_matrix_invert (&cur->inverse_transform)) {
	      g_assert_not_reached ();
	    }
	  }
	  break;
	case SWFDEC_SPRITE_ACTION_BG_COLOR:
	  movie->bg_color = swfdec_color_apply_transform (0, &action->color.transform);
	  swfdec_object_invalidate (SWFDEC_OBJECT (movie), &SWFDEC_OBJECT (movie)->extents);
	  break;
	case SWFDEC_SPRITE_ACTION_COLOR_TRANSFORM:
	  if (cur) {
	    cur->color_transform = action->color.transform;
	  }
	  break;
	case SWFDEC_SPRITE_ACTION_RATIO:
	  if (cur) {
	    cur->ratio = action->uint.value[0];
	  }
	  break;
	case SWFDEC_SPRITE_ACTION_NAME:
	  g_free (cur->name);
	  cur->name = g_strdup (action->string.string);
	  break;
	case SWFDEC_SPRITE_ACTION_CLIP_DEPTH:
	  cur->clip_depth = action->uint.value[0];
	  break;
	default:
	  g_assert_not_reached ();
	  break;
      }
    }
    if (cur) {
      swfdec_rect_union (&inval, &inval, &SWFDEC_OBJECT (cur)->extents);
    }
    if (!swfdec_rect_is_empty (&inval))
      swfdec_object_invalidate (SWFDEC_OBJECT (movie), &inval);
  }
  if (frame->do_actions) {
    GSList *walk;
    for (walk = frame->do_actions; walk; walk = walk->next)
      swfdec_decoder_queue_script (SWFDEC_OBJECT (movie)->decoder, walk->data);
  }
}

void 
swfdec_movie_clip_iterate (SwfdecMovieClip *movie)
{
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
  movie->current_frame = movie->next_frame;
  movie->next_frame = -1;
  if (SWFDEC_IS_SPRITE (movie->child)) {
    swfdec_movie_clip_perform_actions (movie);
    if (movie->next_frame == -1) {
      int n_frames = SWFDEC_SPRITE (movie->child)->n_frames;
      movie->next_frame = movie->current_frame + 1;
      if (movie->next_frame >= n_frames) {
        if (0 /*s->repeat*/) {
          movie->next_frame = 0;
        } else {
          movie->next_frame = n_frames - 1;
        }
      }
    }
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
  int clip_depth = 0;
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
  g = movie->list.list;
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
  int clip_depth = 0;
  SwfdecColorTransform trans;
  SwfdecRect rect;

  /* FIXME: do proper clipping */
  if (movie->parent == NULL) {
    /* only the root has a background */
    swfdec_color_set_source (cr, movie->bg_color);
    cairo_paint (cr);
  }

  cairo_transform (cr, &movie->transform);
  swfdec_rect_transform (&rect, inval, &movie->inverse_transform);
  swfdec_color_transform_chain (&trans, &movie->color_transform, color_transform);

  for (g = movie->list.list; g; g = g_list_next (g)) {
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
  swfdec_object_invalidate (object, &object->extents);
  rect.x1 = movie_clip->width;
  rect.y1 = movie_clip->height;
  swfdec_rect_transform (&rect, &rect, matrix);
  object->extents.x0 = MIN (rect.x0, rect.x1);
  object->extents.y0 = MIN (rect.y0, rect.y1);
  object->extents.x1 = MAX (rect.x0, rect.x1);
  object->extents.y1 = MAX (rect.y0, rect.y1);
  swfdec_object_invalidate (object, &object->extents);
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
 * @id: the id of the object contained in this movie
 *
 * Creates a new #SwfdecMovieClip as a child of @parent.
 *
 * Returns: a new #SwfdecMovieClip
 **/
SwfdecMovieClip *
swfdec_movie_clip_new (SwfdecMovieClip *parent, unsigned int id)
{
  SwfdecMovieClip *ret;

  g_return_val_if_fail (SWFDEC_IS_MOVIE_CLIP (parent), NULL);

  SWFDEC_DEBUG ("new movie for item %d\n", id);
  ret = swfdec_object_new (SWFDEC_OBJECT (parent)->decoder, SWFDEC_TYPE_MOVIE_CLIP);
  ret->parent = parent;
  ret->child = swfdec_object_get (SWFDEC_OBJECT (parent)->decoder, id);
  ret->width = parent->width;
  ret->height = parent->height;

  return ret;
}

