#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "swfdec_internal.h"
#include <swfdec_button.h>
#include <string.h>
#include <js/jsapi.h>


SWFDEC_OBJECT_BOILERPLATE (SwfdecButton, swfdec_button)


     static void swfdec_button_base_init (gpointer g_class)
{

}

static void
swfdec_button_init (SwfdecButton * button)
{
  button->records = g_array_new (FALSE, TRUE, sizeof (SwfdecButtonRecord));
  button->actions = g_array_new (FALSE, TRUE, sizeof (SwfdecButtonAction));

  button->state = SWFDEC_BUTTON_UP;
}

static void
swfdec_button_dispose (SwfdecButton * button)
{
  unsigned int i;
  SwfdecButtonAction *action;

  for (i = 0; i < button->actions->len; i++) {
    action = &g_array_index (button->actions, SwfdecButtonAction, i);
    JS_DestroyScript (SWFDEC_OBJECT (button)->decoder->jscx, action->script);
  }
  g_array_free (button->records, TRUE);
  g_array_free (button->actions, TRUE);

  G_OBJECT_CLASS (parent_class)->dispose (G_OBJECT (button));
}

static gboolean
swfdec_button_has_mouse (SwfdecButton *button, double x, double y, int mouse_button)
{
  guint i;
  SwfdecButtonRecord *record;

  for (i = 0; i < button->records->len; i++) {
    SwfdecObject *obj;
    double tmpx, tmpy;

    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->states & SWFDEC_BUTTON_HIT) {
      obj = swfdec_object_get (SWFDEC_OBJECT (button)->decoder, record->id);
      if (!obj) {
        SWFDEC_WARNING ("couldn't get object with id %d", record->id);
	continue;
      }

      tmpx = x;
      tmpy = y;
      swfdec_matrix_transform_point_inverse (&record->transform, &tmpx, &tmpy);

      if (swfdec_object_handle_mouse (obj, tmpx, tmpy, mouse_button, TRUE))
	return TRUE;
    }
  }
  return FALSE;
}

static void
swfdec_button_execute (SwfdecButton *button, SwfdecButtonCondition condition)
{
  unsigned int i;
  SwfdecButtonAction *action;

  if (button->menubutton) {
    g_assert ((condition & (SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN \
	                  | SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN \
	                  | SWFDEC_BUTTON_OUT_DOWN_TO_IDLE)) == 0);
  } else {
    g_assert ((condition & (SWFDEC_BUTTON_IDLE_TO_OVER_DOWN \
	                  | SWFDEC_BUTTON_OVER_DOWN_TO_IDLE)) == 0);
  }

  for(i=0;i<button->actions->len;i++){
    action = &g_array_index (button->actions, SwfdecButtonAction, i);

    SWFDEC_LOG ("button condition %04x", action->condition);
    if (action->condition & condition) {
      swfdec_decoder_queue_script (SWFDEC_OBJECT (button)->decoder, action->script);
    }
  }
}


static void 
swfdec_button_change_state (SwfdecButton *button, double x, double y, int mouse_button)
{
  gboolean has_mouse;

  has_mouse = swfdec_button_has_mouse (button, x, y, mouse_button);
  switch (button->state) {
    case SWFDEC_BUTTON_UP:
      if (!has_mouse)
	break;
      if (mouse_button) {
	button->state = SWFDEC_BUTTON_DOWN;
	if (button->menubutton) {
	  swfdec_button_execute (button, SWFDEC_BUTTON_IDLE_TO_OVER_DOWN);
	} else {
	  /* simulate entering then clicking */
	  swfdec_button_execute (button, SWFDEC_BUTTON_IDLE_TO_OVER_UP);
	  swfdec_button_execute (button, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN);
	}
      } else {
	button->state = SWFDEC_BUTTON_OVER;
	swfdec_button_execute (button, SWFDEC_BUTTON_IDLE_TO_OVER_UP);
      }
      break;
    case SWFDEC_BUTTON_OVER:
      if (!has_mouse) {
	button->state = SWFDEC_BUTTON_UP;
	swfdec_button_execute (button, SWFDEC_BUTTON_OVER_UP_TO_IDLE);
      } else if (button->has_mouse) {
	button->state = SWFDEC_BUTTON_DOWN;
	swfdec_button_execute (button, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN);
      }
      break;
    case SWFDEC_BUTTON_DOWN:
      /* this is quite different for push and menu buttons */
      if (!mouse_button) {
	if (!has_mouse) {
	  button->state = SWFDEC_BUTTON_UP;
	  if (button->menubutton || button->has_mouse) {
	    swfdec_button_execute (button, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP);
	    swfdec_button_execute (button, SWFDEC_BUTTON_OVER_UP_TO_IDLE);
	  } else {
	    swfdec_button_execute (button, SWFDEC_BUTTON_OUT_DOWN_TO_IDLE);
	  }
	} else {
	  button->state = SWFDEC_BUTTON_OVER;
	  swfdec_button_execute (button, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP);
	}
      } else {
	if (has_mouse) {
	  if (!button->has_mouse && !button->menubutton)
	    swfdec_button_execute (button, SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN);
	} else if (button->has_mouse) {
	  if (button->menubutton) {
	    button->state = SWFDEC_BUTTON_UP;
	    swfdec_button_execute (button, SWFDEC_BUTTON_OVER_DOWN_TO_IDLE);
	  } else {
	    swfdec_button_execute (button, SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN);
	  }
	}
      }
      break;
    default:
      g_assert_not_reached ();
      break;
  }
  button->has_mouse = has_mouse;
}

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

static void
swfdec_button_render (SwfdecObject *object, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval)
{
  SwfdecButton *button = SWFDEC_BUTTON (object);
  SwfdecButtonRecord *record;
  unsigned int i;

  for (i = 0; i < button->records->len; i++) {
    SwfdecObject *obj;

    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->states & button->state) {
      obj = swfdec_object_get (object->decoder, record->id);
      if (obj) {
	SwfdecColorTransform color_trans;
	SwfdecRect rect;
	swfdec_color_transform_chain (&color_trans, &record->color_transform,
	    trans);
	cairo_transform (cr, &record->transform);
	swfdec_rect_transform_inverse (&rect, inval, &record->transform);
	swfdec_object_render (obj, cr, &color_trans, &rect);
      } else {
	SWFDEC_WARNING ("couldn't find object id %d", record->id);
      }
    }
  }
}

static void
swfdec_button_class_init (SwfdecButtonClass * g_class)
{
  SwfdecObjectClass *object_class = SWFDEC_OBJECT_CLASS (g_class);

  object_class->render = swfdec_button_render;
  object_class->handle_mouse = swfdec_button_handle_mouse;
}

