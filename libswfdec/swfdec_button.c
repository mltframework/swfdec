
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
  for (i = 0; i < button->records->len; i++) {
    action = &g_array_index (button->actions, SwfdecButtonAction, i);
    swfdec_buffer_unref (action->buffer);
  }
  g_array_free (button->records, TRUE);
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

int
tag_func_define_button_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  int flags;
  int offset;
  SwfdecTransform trans;
  SwfdecColorTransform color_trans;
  SwfdecButton *button;
  unsigned char *endptr;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  button = swfdec_object_new (SWFDEC_TYPE_BUTTON);
  SWFDEC_OBJECT (button)->id = id;
  s->objects = g_list_append (s->objects, button);

  SWFDEC_LOG ("  ID: %d", id);

  flags = swfdec_bits_get_u8 (bits);
  offset = swfdec_bits_get_u16 (bits);

  SWFDEC_LOG ("  flags = %d", flags);
  SWFDEC_LOG ("  offset = %d", offset);

  while (swfdec_bits_peek_u8 (bits)) {
    int reserved;
    int hit_test;
    int down;
    int over;
    int up;
    int character;
    int layer;
    SwfdecButtonRecord record = { 0 };

    swfdec_bits_syncbits (bits);
    reserved = swfdec_bits_getbits (bits, 4);
    hit_test = swfdec_bits_getbit (bits);
    down = swfdec_bits_getbit (bits);
    over = swfdec_bits_getbit (bits);
    up = swfdec_bits_getbit (bits);
    character = swfdec_bits_get_u16 (bits);
    layer = swfdec_bits_get_u16 (bits);

    SWFDEC_LOG ("  reserved = %d", reserved);
    if (reserved) {
      SWFDEC_WARNING ("reserved is supposed to be 0");
    }
    SWFDEC_LOG ("hit_test=%d down=%d over=%d up=%d character=%d layer=%d",
        hit_test, down, over, up, character, layer);

    swfdec_bits_get_transform (bits, &trans);
    swfdec_bits_syncbits (bits);
    swfdec_bits_get_color_transform (bits, &color_trans);
    swfdec_bits_syncbits (bits);

    record.hit = hit_test;
    record.up = up;
    record.over = over;
    record.down = down;

    record.segment = swfdec_spriteseg_new ();
    record.segment->id = character;
    memcpy (&record.segment->transform, &trans, sizeof (SwfdecTransform));
    memcpy (&record.segment->color_transform, &color_trans,
        sizeof (SwfdecColorTransform));

    g_array_append_val (button->records, record);
  }
  swfdec_bits_get_u8 (bits);

  while (offset != 0) {
    SwfdecButtonAction action = { 0 };
    int len;

    offset = swfdec_bits_get_u16 (bits);
    action.condition = swfdec_bits_get_u16 (bits);

    if (offset) {
      len = offset - 4;
    } else {
      len = bits->end - bits->ptr;
    }

    SWFDEC_LOG ("  offset = %d", offset);

    action.buffer = swfdec_buffer_new_subbuffer (bits->buffer,
        bits->ptr - bits->buffer->data, len);

    bits->ptr += len;

    g_array_append_val (button->actions, action);
  }
  SWFDEC_DEBUG ("number of actions %d", button->actions->len);

  return SWF_OK;
}

int
tag_func_define_button (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int id;
  SwfdecTransform trans;
  SwfdecColorTransform color_trans;
  SwfdecButton *button;
  unsigned char *endptr;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  button = swfdec_object_new (SWFDEC_TYPE_BUTTON);
  SWFDEC_OBJECT (button)->id = id;
  s->objects = g_list_append (s->objects, button);

  SWFDEC_LOG ("  ID: %d", id);

  while (swfdec_bits_peek_u8 (bits)) {
    int reserved;
    int hit_test;
    int down;
    int over;
    int up;
    int character;
    int layer;
    SwfdecButtonRecord record = { 0 };

    swfdec_bits_syncbits (bits);
    reserved = swfdec_bits_getbits (bits, 4);
    hit_test = swfdec_bits_getbit (bits);
    down = swfdec_bits_getbit (bits);
    over = swfdec_bits_getbit (bits);
    up = swfdec_bits_getbit (bits);
    character = swfdec_bits_get_u16 (bits);
    layer = swfdec_bits_get_u16 (bits);

    SWFDEC_LOG ("  reserved = %d", reserved);
    if (reserved) {
      SWFDEC_WARNING ("reserved is supposed to be 0");
    }
    SWFDEC_LOG ("hit_test=%d down=%d over=%d up=%d character=%d layer=%d",
        hit_test, down, over, up, character, layer);

    swfdec_bits_get_transform (bits, &trans);
    swfdec_bits_syncbits (bits);
    swfdec_bits_get_color_transform (bits, &color_trans);
    swfdec_bits_syncbits (bits);

    record.hit = hit_test;
    record.up = up;
    record.over = over;
    record.down = down;

    record.segment = swfdec_spriteseg_new ();
    record.segment->id = character;
    memcpy (&record.segment->transform, &trans, sizeof (SwfdecTransform));
    memcpy (&record.segment->color_transform, &color_trans,
        sizeof (SwfdecColorTransform));

    g_array_append_val (button->records, record);
  }
  swfdec_bits_get_u8 (bits);

  {
    SwfdecButtonAction action = { 0 };
    int len;

    action.condition = 0x08; /* over to down */

    len = bits->end - bits->ptr;

    action.buffer = swfdec_buffer_new_subbuffer (bits->buffer,
        bits->ptr - bits->buffer->data, len);

    bits->ptr += len;

    g_array_append_val (button->actions, action);
  }
  SWFDEC_DEBUG ("number of actions %d", button->actions->len);

  return SWF_OK;
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
      swfdec_action_script_execute (s, action->buffer);
    }
  }
  
}

