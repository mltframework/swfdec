
#include "swfdec_internal.h"
#include <swfdec_button.h>


static void swfdec_button_base_init (gpointer g_class);
static void swfdec_button_class_init (gpointer g_class, gpointer user_data);
static void swfdec_button_init (GTypeInstance *instance, gpointer g_class);
static void swfdec_button_dispose (GObject *object);

static SwfdecLayer * swfdec_button_prerender (SwfdecDecoder * s,
    SwfdecSpriteSegment * seg, SwfdecObject * object, SwfdecLayer * oldlayer);


GType _swfdec_button_type;

static GObjectClass *parent_class = NULL;

GType swfdec_button_get_type (void)
{
  if (!_swfdec_button_type) {
    static const GTypeInfo object_info = {
      sizeof (SwfdecButtonClass),
      swfdec_button_base_init,
      NULL,
      swfdec_button_class_init,
      NULL,
      NULL,
      sizeof (SwfdecButton),
      32,
      swfdec_button_init,
      NULL
    };
    _swfdec_button_type = g_type_register_static (SWFDEC_TYPE_OBJECT,
        "SwfdecButton", &object_info, 0);
  }
  return _swfdec_button_type;
}

static void swfdec_button_base_init (gpointer g_class)
{
  //GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

}

static void swfdec_button_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  gobject_class->dispose = swfdec_button_dispose;

  parent_class = g_type_class_peek_parent (gobject_class);

  SWFDEC_OBJECT_CLASS (g_class)->prerender = swfdec_button_prerender;
  //SWFDEC_OBJECT_CLASS (g_class)->render = swfdec_button_render;
}

static void swfdec_button_init (GTypeInstance *instance, gpointer g_class)
{

}

static void swfdec_button_dispose (GObject *object)
{
  int i;
  SwfdecButton *button = SWFDEC_BUTTON(object);

  for (i = 0; i < 3; i++) {
    if (button->state[i]) {
      swfdec_spriteseg_free (button->state[i]);
    }
  }
}

static SwfdecLayer *
swfdec_button_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object, SwfdecLayer * oldlayer)
{
  SwfdecButton *button = SWFDEC_BUTTON (object);
  SwfdecObject *obj;
  SwfdecLayer *layer;
  SwfdecSpriteSegment *tmpseg;

  if (oldlayer && oldlayer->seg == seg)
    return oldlayer;

  layer = swfdec_layer_new ();
  layer->seg = seg;

  layer->rect.x0 = 0;
  layer->rect.x1 = 0;
  layer->rect.y0 = 0;
  layer->rect.y1 = 0;

  art_affine_multiply (layer->transform, seg->transform, s->transform);
  if (button->state[0]) {
    SwfdecLayer *child_layer = NULL;

    obj = swfdec_object_get (s, button->state[0]->id);
    if (!obj)
      return NULL;

    tmpseg = swfdec_spriteseg_dup (button->state[0]);
    art_affine_multiply (tmpseg->transform,
	button->state[0]->transform, seg->transform);

    child_layer = swfdec_spriteseg_prerender (s, tmpseg, NULL);

    if (child_layer) {
      layer->sublayers = g_list_append (layer->sublayers, child_layer);

      art_irect_union_to_masked (&layer->rect, &child_layer->rect, &s->irect);
    }
    swfdec_spriteseg_free (tmpseg);
  }

  return layer;
}

void
swfdec_button_render (SwfdecDecoder * s, SwfdecLayer * layer,
    SwfdecObject * object)
{
  int i;
  SwfdecLayerVec *layervec;

  for (i = 0; i < layer->fills->len; i++) {
    layervec = &g_array_index (layer->fills, SwfdecLayerVec, i);
    swfdec_layervec_render (s, layervec);
  }
  for (i = 0; i < layer->lines->len; i++) {
    layervec = &g_array_index (layer->lines, SwfdecLayerVec, i);
    swfdec_layervec_render (s, layervec);
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
  double trans[6];
  double color_add[4], color_mult[4];
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

    //swfdec_bits_get_art_matrix(bits, trans);
    swfdec_bits_get_art_matrix (bits, trans);
    swfdec_bits_syncbits (bits);
    SWFDEC_LOG("bits->ptr %p", bits->ptr);
    swfdec_bits_get_art_color_transform (bits, color_mult, color_add);
    swfdec_bits_syncbits (bits);

    SWFDEC_LOG("bits->ptr %p", bits->ptr);

    if (up) {
      if (button->state[0]) {
	SWFDEC_WARNING ("button->state already set");
	swfdec_spriteseg_free (button->state[0]);
      }
      button->state[0] = swfdec_spriteseg_new ();
      button->state[0]->id = character;
      art_affine_copy (button->state[0]->transform, trans);
      memcpy (button->state[0]->color_mult, color_mult, 4 * sizeof (double));
      memcpy (button->state[0]->color_add, color_add, 4 * sizeof (double));
    }
    if (over) {
      if (button->state[1]) {
	SWFDEC_WARNING ("button->state already set");
	swfdec_spriteseg_free (button->state[1]);
      }
      button->state[1] = swfdec_spriteseg_new ();
      button->state[1]->id = character;
      art_affine_copy (button->state[1]->transform, trans);
      memcpy (button->state[1]->color_mult, color_mult, 4 * sizeof (double));
      memcpy (button->state[1]->color_add, color_add, 4 * sizeof (double));
    }
    if (down) {
      if (button->state[2]) {
	SWFDEC_WARNING ("button->state already set");
	swfdec_spriteseg_free (button->state[2]);
      }
      button->state[2] = swfdec_spriteseg_new ();
      button->state[2]->id = character;
      art_affine_copy (button->state[2]->transform, trans);
      memcpy (button->state[2]->color_mult, color_mult, 4 * sizeof (double));
      memcpy (button->state[2]->color_add, color_add, 4 * sizeof (double));
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
