
#include "swfdec_internal.h"
#include <swfdec_button.h>
#include <string.h>

static void
swfdec_button_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object);

SWFDEC_OBJECT_BOILERPLATE (SwfdecButton, swfdec_button)


     static void swfdec_button_base_init (gpointer g_class)
{

}

static void
swfdec_button_class_init (SwfdecButtonClass * g_class)
{
  SWFDEC_OBJECT_CLASS (g_class)->render = swfdec_button_render;
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
  SwfdecButtonRecord *record;
  SwfdecButtonAction *action;

  for (i = 0; i < button->records->len; i++) {
    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    swfdec_spriteseg_free (record->segment);
  }
  for (i = 0; i < button->actions->len; i++) {
    action = &g_array_index (button->actions, SwfdecButtonAction, i);
    swfdec_buffer_unref (action->buffer);
  }
  g_array_free (button->records, TRUE);
  g_array_free (button->actions, TRUE);
}

static gboolean
swfdec_button_has_mouse (SwfdecDecoder * s, SwfdecButton *button)
{
  guint i;
  SwfdecButtonRecord *record;

  for (i = 0; i < button->records->len; i++) {
    SwfdecSpriteSegment *tmpseg;
    SwfdecObject *obj;

    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->states & SWFDEC_BUTTON_HIT) {
      obj = swfdec_object_get (s, record->segment->id);
      if (!obj)
        return FALSE;

      tmpseg = swfdec_spriteseg_dup (record->segment);
      //swfdec_transform_multiply (&tmpseg->transform,
      //    &record->segment->transform, &seg->transform);

      if (swfdec_decoder_has_mouse (s, tmpseg, obj)) {
	swfdec_spriteseg_free (tmpseg);
	return TRUE;
      }
      swfdec_spriteseg_free (tmpseg);
    }
  }
  return FALSE;
}

static void
swfdec_button_execute (SwfdecDecoder *s, SwfdecButton *button, SwfdecButtonCondition condition)
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

    SWFDEC_DEBUG ("button condition %04x", action->condition);
    if (action->condition & condition) {
      s->execute_list = g_list_append (s->execute_list, action->buffer);
    }
  }
}


static void 
swfdec_button_change_state (SwfdecDecoder *s, SwfdecButton *button)
{
  gboolean has_mouse;

  if (s->mouse_grab && s->mouse_grab != (SwfdecObject *) button)
    return;

  has_mouse = swfdec_button_has_mouse (s, button);
  switch (button->state) {
    case SWFDEC_BUTTON_UP:
      if (!has_mouse)
	break;
      if (s->mouse_button) {
	button->state = SWFDEC_BUTTON_DOWN;
	if (button->menubutton) {
	  swfdec_button_execute (s, button, SWFDEC_BUTTON_IDLE_TO_OVER_DOWN);
	} else {
	  /* simulate entering then clicking */
	  swfdec_button_execute (s, button, SWFDEC_BUTTON_IDLE_TO_OVER_UP);
	  swfdec_button_execute (s, button, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN);
	  swfdec_decoder_grab_mouse (s, SWFDEC_OBJECT (button));
	}
      } else {
	button->state = SWFDEC_BUTTON_OVER;
	swfdec_button_execute (s, button, SWFDEC_BUTTON_IDLE_TO_OVER_UP);
      }
      break;
    case SWFDEC_BUTTON_OVER:
      if (!has_mouse) {
	button->state = SWFDEC_BUTTON_UP;
	swfdec_button_execute (s, button, SWFDEC_BUTTON_OVER_UP_TO_IDLE);
      } else if (button->has_mouse) {
	button->state = SWFDEC_BUTTON_DOWN;
	swfdec_button_execute (s, button, SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN);
	swfdec_decoder_grab_mouse (s, SWFDEC_OBJECT (button));
      }
      break;
    case SWFDEC_BUTTON_DOWN:
      /* this is quite different for push and menu buttons */
      if (!s->mouse_button) {
	if (!has_mouse) {
	  button->state = SWFDEC_BUTTON_UP;
	  if (button->menubutton || button->has_mouse) {
	    swfdec_button_execute (s, button, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP);
	    swfdec_button_execute (s, button, SWFDEC_BUTTON_OVER_UP_TO_IDLE);
	  } else {
	    swfdec_button_execute (s, button, SWFDEC_BUTTON_OUT_DOWN_TO_IDLE);
	  }
	} else {
	  button->state = SWFDEC_BUTTON_OVER;
	  swfdec_button_execute (s, button, SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP);
	}
      } else {
	if (has_mouse) {
	  if (!button->has_mouse && !button->menubutton)
	    swfdec_button_execute (s, button, SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN);
	} else if (button->has_mouse) {
	  if (button->menubutton) {
	    button->state = SWFDEC_BUTTON_UP;
	    swfdec_button_execute (s, button, SWFDEC_BUTTON_OVER_DOWN_TO_IDLE);
	  } else {
	    swfdec_button_execute (s, button, SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN);
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

static void
swfdec_button_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object)
{
  SwfdecButton *button = SWFDEC_BUTTON (object);
  SwfdecButtonRecord *record;
  SwfdecObjectClass *klass;
  unsigned int i;

  swfdec_button_change_state (s, button);

  for (i = 0; i < button->records->len; i++) {
    SwfdecSpriteSegment *tmpseg;
    SwfdecObject *obj;

    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->states & button->state) {
      obj = swfdec_object_get (s, record->segment->id);
      if (!obj)
        return;

      tmpseg = swfdec_spriteseg_dup (record->segment);
      cairo_matrix_multiply (&tmpseg->transform,
          &record->segment->transform, &seg->transform);

      klass = SWFDEC_OBJECT_GET_CLASS (obj);
      if (klass->render) {
        klass->render (s, tmpseg, obj);
      }
      swfdec_spriteseg_free (tmpseg);
    }
  }
}

