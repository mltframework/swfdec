
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

}

static void swfdec_button_dispose (SwfdecButton *button)
{
  int i;

  for (i = 0; i < 3; i++) {
    if (button->state[i]) {
      swfdec_spriteseg_free (button->state[i]);
    }
  }
}

static void
swfdec_button_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object)
{
  SwfdecButton *button = SWFDEC_BUTTON (object);

  if (button->state[0]) {
    SwfdecSpriteSegment *tmpseg;
    SwfdecObject *obj;

    obj = swfdec_object_get (s, button->state[0]->id);
    if (!obj) return;

    tmpseg = swfdec_spriteseg_dup (button->state[0]);
    swfdec_transform_multiply (&tmpseg->transform,
	&button->state[0]->transform, &seg->transform);

    swfdec_spriteseg_render (s, tmpseg);
    swfdec_spriteseg_free (tmpseg);
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

  endptr = bits->ptr + s->tag_len;

  id = swfdec_bits_get_u16 (bits);
  button = g_object_new (SWFDEC_TYPE_BUTTON, NULL);
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
    SWFDEC_LOG("  hit_test = %d", hit_test);
    SWFDEC_LOG("  down = %d", down);
    SWFDEC_LOG("  over = %d", over);
    SWFDEC_LOG("  up = %d", up);
    SWFDEC_LOG("  character = %d", character);
    SWFDEC_LOG("  layer = %d", layer);

    SWFDEC_LOG("bits->ptr %p", bits->ptr);

    swfdec_bits_get_transform (bits, &trans);
    swfdec_bits_syncbits (bits);
    SWFDEC_LOG("bits->ptr %p", bits->ptr);
    swfdec_bits_get_color_transform (bits, &color_trans);
    swfdec_bits_syncbits (bits);

    SWFDEC_LOG("bits->ptr %p", bits->ptr);

    if (up) {
      if (button->state[0]) {
	SWFDEC_WARNING ("button->state already set");
	swfdec_spriteseg_free (button->state[0]);
      }
      button->state[0] = swfdec_spriteseg_new ();
      button->state[0]->id = character;
      memcpy (&button->state[0]->transform, &trans, sizeof (SwfdecTransform));
      memcpy (&button->state[0]->color_transform, &color_trans,
          sizeof (SwfdecColorTransform));
    }
    if (over) {
      if (button->state[1]) {
	SWFDEC_WARNING ("button->state already set");
	swfdec_spriteseg_free (button->state[1]);
      }
      button->state[1] = swfdec_spriteseg_new ();
      button->state[1]->id = character;
      memcpy (&button->state[1]->transform, &trans, sizeof (SwfdecTransform));
      memcpy (&button->state[1]->color_transform, &color_trans,
          sizeof (SwfdecColorTransform));
    }
    if (down) {
      if (button->state[2]) {
	SWFDEC_WARNING ("button->state already set");
	swfdec_spriteseg_free (button->state[2]);
      }
      button->state[2] = swfdec_spriteseg_new ();
      button->state[2]->id = character;
      memcpy (&button->state[2]->transform, &trans, sizeof (SwfdecTransform));
      memcpy (&button->state[2]->color_transform, &color_trans,
          sizeof (SwfdecColorTransform));
    }

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
