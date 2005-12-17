
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

}

static void
swfdec_button_dispose (SwfdecButton * button)
{
  int i;
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

static void
swfdec_button_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object)
{
  SwfdecButton *button = SWFDEC_BUTTON (object);
  SwfdecButtonRecord *record;
  SwfdecObjectClass *klass;
  int i;
  gboolean in_button;

  s->render->mouse_check = TRUE;
  s->render->mouse_in_button = FALSE;
  for (i = 0; i < button->records->len; i++) {
    SwfdecSpriteSegment *tmpseg;
    SwfdecObject *obj;

    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->hit) {
      obj = swfdec_object_get (s, record->segment->id);
      if (!obj)
        return;

      tmpseg = swfdec_spriteseg_dup (record->segment);
      swfdec_transform_multiply (&tmpseg->transform,
          &record->segment->transform, &seg->transform);

      klass = SWFDEC_OBJECT_GET_CLASS (obj);
      if (klass->render) {
        klass->render (s, tmpseg, obj);
      }
      swfdec_spriteseg_free (tmpseg);
    }
  }
  s->render->mouse_check = FALSE;
  in_button = s->render->mouse_in_button;
  if (in_button) {
    s->render->active_button = object;
    SWFDEC_DEBUG ("in button");
  }

  for (i = 0; i < button->records->len; i++) {
    SwfdecSpriteSegment *tmpseg;
    SwfdecObject *obj;

    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if ((!in_button && record->up) || (in_button && record->over)) {
      obj = swfdec_object_get (s, record->segment->id);
      if (!obj)
        return;

      tmpseg = swfdec_spriteseg_dup (record->segment);
      swfdec_transform_multiply (&tmpseg->transform,
          &record->segment->transform, &seg->transform);

      klass = SWFDEC_OBJECT_GET_CLASS (obj);
      if (klass->render) {
        klass->render (s, tmpseg, obj);
      }
      swfdec_spriteseg_free (tmpseg);
    }
  }
}

void
swfdec_button_execute (SwfdecDecoder *s, SwfdecButton *button)
{
  int i;
  SwfdecButtonAction *action;

  for(i=0;i<button->actions->len;i++){
    action = &g_array_index (button->actions, SwfdecButtonAction, i);

    SWFDEC_DEBUG ("button condition %04x", action->condition);
    if (action->condition & 0x0008) {
      s->execute_list = g_list_append (s->execute_list, action->buffer);
    }
  }
  
}

