
#include "swfdec_internal.h"
#include <swfdec_button.h>


static void swfdec_button_base_init (gpointer g_class);
static void swfdec_button_class_init (gpointer g_class, gpointer user_data);
static void swfdec_button_init (GTypeInstance *instance, gpointer g_class);
static void swfdec_button_dispose (GObject *object);


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
    _swfdec_button_type = g_type_register_static (G_TYPE_OBJECT,
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

SwfdecLayer *
swfdec_button_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object, SwfdecLayer * oldlayer)
{
  SwfdecButton *button = object->priv;
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
  bits_t *bits = &s->b;
  int id;
  int flags;
  int offset;
  int condition;
  SwfdecObject *object;
  double trans[6];
  double color_add[4], color_mult[4];
  SwfdecButton *button;
  unsigned char *endptr;

  endptr = bits->ptr + s->tag_len;

  id = get_u16 (bits);
  object = swfdec_object_new (s, id);

  button = g_new0 (SwfdecButton, 1);
  object->type = SWF_OBJECT_BUTTON;
  object->priv = button;

  SWF_DEBUG (0, "  ID: %d\n", object->id);

  flags = get_u8 (bits);
  offset = get_u16 (bits);

  SWF_DEBUG (0, "  flags = %d\n", flags);
  SWF_DEBUG (0, "  offset = %d\n", offset);

  while (peek_u8 (bits)) {
    int reserved;
    int hit_test;
    int down;
    int over;
    int up;
    int character;
    int layer;

    syncbits (bits);
    reserved = getbits (bits, 4);
    hit_test = getbit (bits);
    down = getbit (bits);
    over = getbit (bits);
    up = getbit (bits);
    character = get_u16 (bits);
    layer = get_u16 (bits);

    SWF_DEBUG (0, "  reserved = %d\n", reserved);
    if (reserved) {
      SWF_DEBUG (4, "reserved is supposed to be 0\n");
    }
    SWF_DEBUG (0, "  hit_test = %d\n", hit_test);
    SWF_DEBUG (0, "  down = %d\n", down);
    SWF_DEBUG (0, "  over = %d\n", over);
    SWF_DEBUG (0, "  up = %d\n", up);
    SWF_DEBUG (0, "  character = %d\n", character);
    SWF_DEBUG (0, "  layer = %d\n", layer);

    SWF_DEBUG (0, "bits->ptr %p\n", bits->ptr);

    //get_art_matrix(bits, trans);
    get_art_matrix (bits, trans);
    syncbits (bits);
    SWF_DEBUG (0, "bits->ptr %p\n", bits->ptr);
    get_art_color_transform (bits, color_mult, color_add);
    syncbits (bits);

    SWF_DEBUG (0, "bits->ptr %p\n", bits->ptr);

    if (up) {
      if (button->state[0]) {
	SWF_DEBUG (4, "button->state already set\n");
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
	SWF_DEBUG (4, "button->state already set\n");
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
	SWF_DEBUG (4, "button->state already set\n");
	swfdec_spriteseg_free (button->state[2]);
      }
      button->state[2] = swfdec_spriteseg_new ();
      button->state[2]->id = character;
      art_affine_copy (button->state[2]->transform, trans);
      memcpy (button->state[2]->color_mult, color_mult, 4 * sizeof (double));
      memcpy (button->state[2]->color_add, color_add, 4 * sizeof (double));
    }

  }
  get_u8 (bits);

  while (offset != 0) {
    offset = get_u16 (bits);
    condition = get_u16 (bits);

    SWF_DEBUG (0, "  offset = %d\n", offset);
    SWF_DEBUG (0, "  condition = 0x%04x\n", condition);

    get_actions (s, bits);
  }

  return SWF_OK;
}
