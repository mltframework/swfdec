
#include "swfdec_internal.h"

#include <swfdec_sprite.h>

static void swfdec_sprite_base_init (gpointer g_class);
static void swfdec_sprite_class_init (gpointer g_class, gpointer user_data);
static void swfdec_sprite_init (GTypeInstance *instance, gpointer g_class);
static void swfdec_sprite_dispose (GObject *object);
static SwfdecLayer * swfdec_sprite_prerender (SwfdecDecoder * s,
    SwfdecSpriteSegment * seg, SwfdecObject * object, SwfdecLayer * oldlayer);


GType _swfdec_sprite_type;

static GObjectClass *parent_class = NULL;

GType swfdec_sprite_get_type (void)
{
  if (!_swfdec_sprite_type) {
    static const GTypeInfo object_info = {
      sizeof (SwfdecSpriteClass),
      swfdec_sprite_base_init,
      NULL,
      swfdec_sprite_class_init,
      NULL,
      NULL,
      sizeof (SwfdecSprite),
      32,
      swfdec_sprite_init,
      NULL
    };
    _swfdec_sprite_type = g_type_register_static (SWFDEC_TYPE_OBJECT,
        "SwfdecSprite", &object_info, 0);
  }
  return _swfdec_sprite_type;
}

static void swfdec_sprite_base_init (gpointer g_class)
{
  //GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

}

static void swfdec_sprite_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  gobject_class->dispose = swfdec_sprite_dispose;

  parent_class = g_type_class_peek_parent (gobject_class);

  SWFDEC_OBJECT_CLASS (g_class)->prerender = swfdec_sprite_prerender;
}

static void swfdec_sprite_init (GTypeInstance *instance, gpointer g_class)
{

}

static void swfdec_sprite_dispose (GObject *object)
{
#if 0
  SwfdecSprite *sprite = SWFDEC_SPRITE (object);
  GList *g;

  for (g = g_list_first (sprite->layers); g; g = g_list_next (g)) {
    g_free (g->data);
  }
  g_list_free (sprite->layers);

  /* FIXME */
  //g_object_unref (G_OBJECT (s->main_sprite));
  //swfdec_render_free (s->render);
#endif
}


static SwfdecLayer *
swfdec_sprite_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object, SwfdecLayer * oldlayer)
{
  SwfdecLayer *layer;
  SwfdecLayer *child_layer;
  SwfdecLayer *old_child_layer;
  GList *g;
  //SwfdecSprite *child_decoder = SWFDEC_SPRITE (object);
  /* FIXME */
  //SwfdecSprite *sprite = child_decoder->main_sprite;
  SwfdecSprite *sprite = NULL;
  SwfdecSpriteSegment *tmpseg;
  SwfdecSpriteSegment *child_seg;
  int frame;

  if (oldlayer && oldlayer->seg == seg && sprite->n_frames == 1)
    return oldlayer;

  layer = swfdec_layer_new ();
  layer->seg = seg;

  /* Not sure why this is wrong.  Somehow the top-level transform
   * gets applied twice. */
  //art_affine_multiply(layer->transform, seg->transform, s->transform);
  art_affine_copy (layer->transform, seg->transform);

  if (oldlayer) {
    layer->frame_number = oldlayer->frame_number + 1;
    if (layer->frame_number >= sprite->n_frames)
      layer->frame_number = 0;
    SWFDEC_DEBUG (
        "iterating old sprite (depth=%d) old_frame=%d frame=%d n_frames=%d\n",
	seg->depth, oldlayer->frame_number, layer->frame_number,
	sprite->n_frames);
  } else {
    SWFDEC_LOG("iterating new sprite (depth=%d)", seg->depth);
    layer->frame_number = 0;
  }
  frame = layer->frame_number;

  layer->rect.x0 = 0;
  layer->rect.x1 = 0;
  layer->rect.y0 = 0;
  layer->rect.y1 = 0;

  SWFDEC_LOG("swfdec_sprite_prerender %d frame %d", object->id,
      layer->frame_number);

/* don't render sprites correctly */
return layer;
  for (g = g_list_last (sprite->layers); g; g = g_list_previous (g)) {
    child_seg = (SwfdecSpriteSegment *) g->data;

    if (child_seg->first_frame > frame)
      continue;
    if (child_seg->last_frame <= frame)
      continue;
    SWFDEC_LOG("prerendering layer %d", child_seg->depth);

    tmpseg = swfdec_spriteseg_dup (child_seg);
    art_affine_multiply (tmpseg->transform,
	child_seg->transform, layer->transform);

#if 0
    old_child_layer = swfdec_render_get_sublayer (layer,
	child_seg->depth, layer->frame_number - 1);
#endif
    old_child_layer = NULL;

    child_layer = swfdec_spriteseg_prerender (s, tmpseg, old_child_layer);
    if (child_layer) {
      layer->sublayers = g_list_append (layer->sublayers, child_layer);

      art_irect_union_to_masked (&layer->rect, &child_layer->rect, &s->irect);
    }

    swfdec_spriteseg_free (tmpseg);
  }

  return layer;
}

#if 0
void
swfdec_sprite_render (SwfdecDecoder * s, SwfdecLayer * parent_layer,
    SwfdecObject * parent_object)
{
  SwfdecLayer *child_layer;
  SwfdecSprite *s2 = SWFDEC_SPRITE (parent_object);
  GList *g;

  SWFDEC_LOG("rendering sprite frame %d of %d",
      parent_layer->frame_number, s2->n_frames);
  for (g = g_list_first (parent_layer->sublayers); g; g = g_list_next (g)) {
    child_layer = (SwfdecLayer *) g->data;
    if (!child_layer)
      continue;
    swfdec_layer_render (s, child_layer);
  }
}
#endif

int
tag_func_define_sprite (SwfdecDecoder * s)
{
  bits_t *bits = &s->b;
  int id;
  SwfdecSprite *sprite;
  //int ret;

  id = get_u16 (bits);
  sprite = g_object_new (SWFDEC_TYPE_SPRITE, NULL);
  SWFDEC_OBJECT (sprite)->id = id;
  s->objects = g_list_append (s->objects, sprite);

  //sprite->main_sprite = s->main_sprite;
  //sprite->parse_sprite = sprite;

  SWFDEC_LOG("  ID: %d", id);

  sprite->n_frames = get_u16 (bits);
  SWFDEC_LOG("n_frames = %d", sprite->n_frames);

#if 0
  //sprite->state = SWF_STATE_PARSETAG;
  //sprite->no_render = 1;

  //sprite->width = s->width;
  //sprite->height = s->height;
  //memcpy (sprite->transform, s->transform, sizeof (double) * 6);

  //sprite->parse = *bits;

  /* massive hack */
  //sprite->objects = s->objects;

  ret = swf_parse (sprite);
  SWFDEC_LOG("  ret = %d", ret);

  *bits = sprite->parse;

  //sprite->frame_number = 0;

  //dump_layers(sprite);
#endif

  return SWF_OK;
}

SwfdecSpriteSegment *
swfdec_sprite_get_seg (SwfdecSprite * sprite, int depth, int frame)
{
  SwfdecSpriteSegment *seg;
  GList *g;

  for (g = g_list_first (sprite->layers); g; g = g_list_next (g)) {
    seg = (SwfdecSpriteSegment *) g->data;
    if (seg->depth == depth && seg->first_frame <= frame
	&& seg->last_frame > frame)
      return seg;
  }

  return NULL;
}

void
swfdec_sprite_add_seg (SwfdecSprite * sprite, SwfdecSpriteSegment * segnew)
{
  GList *g;
  SwfdecSpriteSegment *seg;

  for (g = g_list_first (sprite->layers); g; g = g_list_next (g)) {
    seg = (SwfdecSpriteSegment *) g->data;
    if (seg->depth < segnew->depth) {
      sprite->layers = g_list_insert_before (sprite->layers, g, segnew);
      return;
    }
  }

  sprite->layers = g_list_append (sprite->layers, segnew);
}

SwfdecSpriteSegment *
swfdec_spriteseg_new (void)
{
  SwfdecSpriteSegment *seg;

  seg = g_new0 (SwfdecSpriteSegment, 1);

  return seg;
}

SwfdecSpriteSegment *
swfdec_spriteseg_dup (SwfdecSpriteSegment * seg)
{
  SwfdecSpriteSegment *newseg;

  newseg = g_new (SwfdecSpriteSegment, 1);
  memcpy (newseg, seg, sizeof (*seg));

  return newseg;
}

void
swfdec_spriteseg_free (SwfdecSpriteSegment * seg)
{
  g_free (seg);
}

int
swfdec_spriteseg_place_object_2 (SwfdecDecoder * s)
{
  bits_t *bits = &s->b;
  int reserved;
  int has_compose;
  int has_name;
  int has_ratio;
  int has_color_transform;
  int has_matrix;
  int has_character;
  int move;
  int depth;
  SwfdecSpriteSegment *layer;
  SwfdecSpriteSegment *oldlayer;

  reserved = getbit (bits);
  has_compose = getbit (bits);
  has_name = getbit (bits);
  has_ratio = getbit (bits);
  has_color_transform = getbit (bits);
  has_matrix = getbit (bits);
  has_character = getbit (bits);
  move = getbit (bits);
  depth = get_u16 (bits);

  /* reserved is somehow related to sprites */
  SWFDEC_LOG("  reserved = %d", reserved);
  if (reserved) {
    SWFDEC_WARNING ("  reserved bits non-zero %d", reserved);
  }
  SWFDEC_LOG("  has_compose = %d", has_compose);
  SWFDEC_LOG("  has_name = %d", has_name);
  SWFDEC_LOG("  has_ratio = %d", has_ratio);
  SWFDEC_LOG("  has_color_transform = %d", has_color_transform);
  SWFDEC_LOG("  has_matrix = %d", has_matrix);
  SWFDEC_LOG("  has_character = %d", has_character);

  oldlayer = swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame);
  if (oldlayer) {
    oldlayer->last_frame = s->parse_sprite->parse_frame;
  }

  layer = swfdec_spriteseg_new ();

  layer->depth = depth;
  layer->first_frame = s->parse_sprite->parse_frame;
  layer->last_frame = s->parse_sprite->n_frames;

  swfdec_sprite_add_seg (s->parse_sprite, layer);

  if (has_character) {
    layer->id = get_u16 (bits);
    SWFDEC_LOG("  id = %d", layer->id);
  } else {
    if (oldlayer)
      layer->id = oldlayer->id;
  }
  if (has_matrix) {
    get_art_matrix (bits, layer->transform);
  } else {
    if (oldlayer)
      art_affine_copy (layer->transform, oldlayer->transform);
  }
  if (has_color_transform) {
    get_art_color_transform (bits, layer->color_mult, layer->color_add);
    syncbits (bits);
  } else {
    if (oldlayer) {
      memcpy (layer->color_mult, oldlayer->color_mult, sizeof (double) * 4);
      memcpy (layer->color_add, oldlayer->color_add, sizeof (double) * 4);
    } else {
      layer->color_mult[0] = 1;
      layer->color_mult[1] = 1;
      layer->color_mult[2] = 1;
      layer->color_mult[3] = 1;
      layer->color_add[0] = 0;
      layer->color_add[1] = 0;
      layer->color_add[2] = 0;
      layer->color_add[3] = 0;
    }
  }
  if (has_ratio) {
    layer->ratio = get_u16 (bits);
    SWFDEC_LOG("  ratio = %d", layer->ratio);
  } else {
    if (oldlayer)
      layer->ratio = oldlayer->ratio;
  }
  if (has_name) {
    free (get_string (bits));
  }
  if (has_compose) {
    int id;

    id = get_u16 (bits);
    SWFDEC_WARNING ("composing with %04x", id);
  }

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object (SwfdecDecoder * s)
{
  int depth;
  SwfdecSpriteSegment *seg;
  int id;

  id = get_u16 (&s->b);
  depth = get_u16 (&s->b);
  seg = swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame - 1);

  if (seg) {
    seg->last_frame = s->parse_sprite->parse_frame;
  } else {
    SWFDEC_WARNING ("could not find object to remove (depth %d, frame %d)",
        depth, s->parse_sprite->parse_frame - 1);
  }

  return SWF_OK;
}

int
swfdec_spriteseg_remove_object_2 (SwfdecDecoder * s)
{
  int depth;
  SwfdecSpriteSegment *seg;

  depth = get_u16 (&s->b);
  seg = swfdec_sprite_get_seg (s->parse_sprite, depth,
      s->parse_sprite->parse_frame - 1);

  if (seg) {
    seg->last_frame = s->parse_sprite->parse_frame;
  } else {
    SWFDEC_WARNING ("could not find object to remove (depth %d, frame %d)",
        depth, s->parse_sprite->parse_frame - 1);
  }

  return SWF_OK;
}
