#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <js/jsapi.h>
#include "swfdec_internal.h"
#include "swfdec_button.h"
#include "swfdec_movieclip.h"


SWFDEC_OBJECT_BOILERPLATE (SwfdecButton, swfdec_button)


     static void swfdec_button_base_init (gpointer g_class)
{

}

static void
swfdec_button_init (SwfdecButton * button)
{
  button->records = g_array_new (FALSE, TRUE, sizeof (SwfdecButtonRecord));

  /* we init this empty here so adding records fills this up */
  swfdec_rect_init_empty (&SWFDEC_OBJECT (button)->extents);
}

static void
swfdec_button_dispose (SwfdecButton * button)
{
  g_array_free (button->records, TRUE);
  if (button->events != NULL) {
    swfdec_event_list_free (button->events);
    button->events = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (button));
}

static gboolean
swfdec_button_mouse_in (SwfdecObject *object, double x, double y, int mouse_button)
{
  guint i;
  SwfdecButtonRecord *record;
  SwfdecButton *button = SWFDEC_BUTTON (object);

  for (i = 0; i < button->records->len; i++) {
    SwfdecObject *obj;
    double tmpx, tmpy;

    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->states & SWFDEC_BUTTON_HIT) {
      obj = record->object;
      tmpx = x;
      tmpy = y;
      swfdec_matrix_transform_point_inverse (&record->transform, &tmpx, &tmpy);

      if (swfdec_object_mouse_in (obj, tmpx, tmpy, mouse_button))
	return TRUE;
    }
  }
  return FALSE;
}

static void
swfdec_button_execute (SwfdecButton *button, SwfdecMovieClip *movie,
    SwfdecButtonCondition condition)
{
  if (button->menubutton) {
    g_assert ((condition & (SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN \
                         | SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN \
                         | SWFDEC_BUTTON_OUT_DOWN_TO_IDLE)) == 0);
  } else {
    g_assert ((condition & (SWFDEC_BUTTON_IDLE_TO_OVER_DOWN \
                         | SWFDEC_BUTTON_OVER_DOWN_TO_IDLE)) == 0);
  }
  if (button->events)
    swfdec_event_list_execute (button->events, movie->parent, condition, 0);
}

SwfdecButtonState
swfdec_button_change_state (SwfdecMovieClip *movie, gboolean was_in, int old_button)
{
  SwfdecButton *button = SWFDEC_BUTTON (movie->child);
  SwfdecButtonState state = movie->button_state;

  switch (movie->button_state) {
    case SWFDEC_BUTTON_UP:
      if (!movie->mouse_in)
	break;
      if (movie->mouse_button) {
	state = SWFDEC_BUTTON_DOWN;
	if (button->menubutton) {
	  swfdec_button_execute (button, movie, SWFDEC_BUTTON_IDLE_TO_OVER_DOWN);
	} else {
	  /* simulate entering then clicking */
	  swfdec_button_execute (button, movie, SWFDEC_BUTTON_IDLE_TO_OVER_UP);
	  swfdec_button_execute (button, movie, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN);
	}
      } else {
	state = SWFDEC_BUTTON_OVER;
	swfdec_button_execute (button, movie, SWFDEC_BUTTON_IDLE_TO_OVER_UP);
      }
      break;
    case SWFDEC_BUTTON_OVER:
      if (!movie->mouse_in) {
	state = SWFDEC_BUTTON_UP;
	swfdec_button_execute (button, movie, SWFDEC_BUTTON_OVER_UP_TO_IDLE);
      } else if (movie->mouse_button) {
	state = SWFDEC_BUTTON_DOWN;
	swfdec_button_execute (button, movie, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN);
      }
      break;
    case SWFDEC_BUTTON_DOWN:
      /* this is quite different for push and menu buttons */
      if (!movie->mouse_button) {
	if (!movie->mouse_in) {
	  state = SWFDEC_BUTTON_UP;
	  if (button->menubutton || was_in) {
	    swfdec_button_execute (button, movie, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP);
	    swfdec_button_execute (button, movie, SWFDEC_BUTTON_OVER_UP_TO_IDLE);
	  } else {
	    swfdec_button_execute (button, movie, SWFDEC_BUTTON_OUT_DOWN_TO_IDLE);
	  }
	} else {
	  state = SWFDEC_BUTTON_OVER;
	  swfdec_button_execute (button, movie, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP);
	}
      } else {
	if (movie->mouse_in) {
	  if (!was_in && !button->menubutton)
	    swfdec_button_execute (button, movie, SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN);
	} else if (was_in) {
	  if (button->menubutton) {
	    state = SWFDEC_BUTTON_UP;
	    swfdec_button_execute (button, movie, SWFDEC_BUTTON_OVER_DOWN_TO_IDLE);
	  } else {
	    swfdec_button_execute (button, movie, SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN);
	  }
	}
      }
      break;
    default:
      g_assert_not_reached ();
      break;
  }
  return state;
}

#if 0
static SwfdecMouseResult
swfdec_button_handle_mouse (SwfdecObject *object, double x, double y, int mouse_button)
{
  SwfdecRect inval;
  SwfdecButton *button = SWFDEC_BUTTON (object);
  SwfdecButtonState old_state = button->state;

  g_print ("mouse at %g %g\n", x, y);
  swfdec_button_change_state (button, x, y, mouse_button);
  swfdec_rect_init_empty (&inval);
  if (old_state != button->state) {
    guint i;
    g_print ("button state changed from %d to %d\n", old_state, button->state);
    for (i = 0; i < button->records->len; i++) {
      SwfdecButtonRecord *record;
      SwfdecObject *obj;
      SwfdecRect rect;

      record = &g_array_index (button->records, SwfdecButtonRecord, i);
      if ((record->states & old_state && (record->states & button->state) == 0) ||
	  ((record->states & old_state) == 0 && record->states & button->state)) {
	obj = swfdec_object_get (object->decoder, record->id);
	if (!obj) {
	  SWFDEC_WARNING ("couldn't get object with id %d", record->id);
	  continue;
	}

	swfdec_rect_transform (&rect, &obj->extents, &record->transform);
	swfdec_rect_union (&inval, &rect, &inval);
      }
    }
  }
  swfdec_object_invalidate (object, &inval);

  if (button->state == SWFDEC_BUTTON_DOWN)
    return SWFDEC_MOUSE_GRABBED;
  else if (button->state == SWFDEC_BUTTON_OVER)
    return SWFDEC_MOUSE_HIT;
  else
    return SWFDEC_MOUSE_MISSED;
}
#endif

void
swfdec_button_render (SwfdecButton *button, SwfdecButtonState state, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  SwfdecButtonRecord *record;
  unsigned int i;

  g_return_if_fail (SWFDEC_IS_BUTTON (button));

  for (i = 0; i < button->records->len; i++) {
    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->states & state) {
      SwfdecColorTransform color_trans;
      SwfdecRect rect;
      swfdec_color_transform_chain (&color_trans, &record->color_transform,
	  trans);
      cairo_save (cr);
      cairo_transform (cr, &record->transform);
      swfdec_rect_transform_inverse (&rect, inval, &record->transform);
      swfdec_object_render (record->object, cr, &color_trans, &rect);
      cairo_restore (cr);
    }
  }
}

static void
swfdec_button_class_init (SwfdecButtonClass * g_class)
{
  SwfdecObjectClass *object_class = SWFDEC_OBJECT_CLASS (g_class);

  object_class->mouse_in = swfdec_button_mouse_in;
}

