
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

static void swfdec_button_class_init (SwfdecButtonClass *g_class)
{
  SWFDEC_OBJECT_CLASS (g_class)->render = swfdec_button_render;
}

static void swfdec_button_init (SwfdecButton *button)
{
  button->records = g_array_new (FALSE, TRUE, sizeof(SwfdecButtonRecord));

}

static void swfdec_button_dispose (SwfdecButton *button)
{
  int i;
  SwfdecButtonRecord *record;

  for (i = 0; i < button->records->len; i++) {
    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    swfdec_spriteseg_free (record->segment);
  }
  g_array_free (button->records, TRUE);
}

static void
swfdec_button_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object)
{
  SwfdecButton *button = SWFDEC_BUTTON (object);
  SwfdecButtonRecord *record;
  int i;

  for (i = 0; i < button->records->len; i++) {
    SwfdecSpriteSegment *tmpseg;
    SwfdecObject *obj;

    record = &g_array_index (button->records, SwfdecButtonRecord, i);
    if (record->up) {
      obj = swfdec_object_get (s, record->segment->id);
      if (!obj) return;

      tmpseg = swfdec_spriteseg_dup (record->segment);
      swfdec_transform_multiply (&tmpseg->transform,
          &record->segment->transform, &seg->transform);

      swfdec_spriteseg_render (s, tmpseg);
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
  int condition;
  SwfdecTransform trans;
  SwfdecColorTransform color_trans;
  SwfdecButton *button;
  unsigned char *endptr;

  endptr = bits->ptr + bits->buffer->length;

  id = swfdec_bits_get_u16 (bits);
  button = swfdec_object_new (SWFDEC_TYPE_BUTTON);
  SWFDEC_OBJECT (button)->id = id;
  s->objects = g_list_append (s->objects, button);

  SWFDEC_LOG("  ID: %d", id);

  flags = swfdec_bits_get_u8 (bits);
  offset = swfdec_bits_get_u16 (bits);

  SWFDEC_LOG("  flags = %d", flags);
  SWFDEC_LOG("  offset = %d", offset);

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

    SWFDEC_LOG("  reserved = %d", reserved);
    if (reserved) {
      SWFDEC_WARNING ("reserved is supposed to be 0");
    }
    SWFDEC_LOG("hit_test=%d down=%d over=%d up=%d character=%d layer=%d",
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
    offset = swfdec_bits_get_u16 (bits);
    condition = swfdec_bits_get_u16 (bits);

    SWFDEC_LOG("  offset = %d", offset);
    SWFDEC_LOG("  condition = 0x%04x", condition);

    get_actions (s, bits);
  }

  return SWF_OK;
}
