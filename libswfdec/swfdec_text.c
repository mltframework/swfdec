
#include <swfdec_text.h>
#include "swfdec_internal.h"
#include <swfdec_font.h>


static void swfdec_text_base_init (gpointer g_class);
static void swfdec_text_class_init (gpointer g_class, gpointer user_data);
static void swfdec_text_init (GTypeInstance *instance, gpointer g_class);
static void swfdec_text_dispose (GObject *object);


GType _swfdec_text_type;

static GObjectClass *parent_class = NULL;

GType swfdec_text_get_type (void)
{
  if (!_swfdec_text_type) {
    static const GTypeInfo object_info = {
      sizeof (SwfdecTextClass),
      swfdec_text_base_init,
      NULL,
      swfdec_text_class_init,
      NULL,
      NULL,
      sizeof (SwfdecText),
      32,
      swfdec_text_init,
      NULL
    };
    _swfdec_text_type = g_type_register_static (SWFDEC_TYPE_OBJECT,
        "SwfdecText", &object_info, 0);
  }
  return _swfdec_text_type;
}

static void swfdec_text_base_init (gpointer g_class)
{
  //GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

}

static void swfdec_text_class_init (gpointer g_class, gpointer class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);

  gobject_class->dispose = swfdec_text_dispose;

  parent_class = g_type_class_peek_parent (gobject_class);

}

static void swfdec_text_init (GTypeInstance *instance, gpointer g_class)
{
  SwfdecText *text = SWFDEC_TEXT (instance);

  text->glyphs = g_array_new (FALSE, TRUE, sizeof (SwfdecTextGlyph));
}

static void swfdec_text_dispose (GObject *object)
{
  SwfdecText *text = SWFDEC_TEXT (object);

  g_array_free (text->glyphs, TRUE);
}


static int
define_text (SwfdecDecoder * s, int rgba)
{
  bits_t *bits = &s->b;
  int id;
  int rect[4];
  int n_glyph_bits;
  int n_advance_bits;
  SwfdecText *text = NULL;
  SwfdecTextGlyph glyph = { 0 };

  id = get_u16 (bits);
  text = g_object_new (SWFDEC_TYPE_TEXT, NULL);
  SWFDEC_OBJECT (text)->id = id;
  g_list_append (s->objects, text);

  glyph.color = 0xffffffff;

  get_rect (bits, rect);
  get_art_matrix (bits, SWFDEC_OBJECT (text)->trans);
  syncbits (bits);
  n_glyph_bits = get_u8 (bits);
  n_advance_bits = get_u8 (bits);

  //printf("  id = %d\n", id);
  //printf("  n_glyph_bits = %d\n", n_glyph_bits);
  //printf("  n_advance_bits = %d\n", n_advance_bits);

  while (peekbits (bits, 8) != 0) {
    int type;

    type = getbit (bits);
    if (type == 0) {
      /* glyph record */
      int n_glyphs;
      int i;

      n_glyphs = getbits (bits, 7);
      for (i = 0; i < n_glyphs; i++) {
	glyph.glyph = getbits (bits, n_glyph_bits);

	g_array_append_val (text->glyphs, glyph);
	glyph.x += getbits (bits, n_advance_bits);
      }
    } else {
      /* state change */
      int reserved;
      int has_font;
      int has_color;
      int has_y_offset;
      int has_x_offset;

      reserved = getbits (bits, 3);
      has_font = getbit (bits);
      has_color = getbit (bits);
      has_y_offset = getbit (bits);
      has_x_offset = getbit (bits);
      if (has_font) {
	glyph.font = get_u16 (bits);
	//printf("  font = %d\n",font);
      }
      if (has_color) {
	if (rgba) {
	  glyph.color = get_rgba (bits);
	} else {
	  glyph.color = get_color (bits);
	}
	//printf("  color = %08x\n",glyph.color);
      }
      if (has_x_offset) {
	glyph.x = get_u16 (bits);
	//printf("  x = %d\n",x);
      }
      if (has_y_offset) {
	glyph.y = get_u16 (bits);
	//printf("  y = %d\n",y);
      }
      if (has_font) {
	glyph.height = get_u16 (bits);
	//printf("  height = %d\n",height);
      }
      if (has_font || has_color) {
	g_array_append_val (text->glyphs, glyph);
      }
    }
    syncbits (bits);
  }
  get_u8 (bits);

  return SWF_OK;
}

int
tag_func_define_text (SwfdecDecoder * s)
{
  return define_text (s, 0);
}

int
tag_func_define_text_2 (SwfdecDecoder * s)
{
  return define_text (s, 1);
}


SwfdecLayer *
swfdec_text_prerender (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object, SwfdecLayer * oldlayer)
{
  int i, j;
  SwfdecText *text;
  SwfdecLayerVec *layervec;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;
  SwfdecObject *fontobj;
  SwfdecLayer *layer;
  SwfdecTextGlyph *glyph;

  if (oldlayer && oldlayer->seg == seg)
    return oldlayer;

  layer = swfdec_layer_new ();
  layer->seg = seg;
  art_affine_multiply (layer->transform, seg->transform, s->transform);

  layer->rect.x0 = 0;
  layer->rect.x1 = 0;
  layer->rect.y0 = 0;
  layer->rect.y1 = 0;

  text = SWFDEC_TEXT (object);
  for (i = 0; i < text->glyphs->len; i++) {
    glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);

    fontobj = swfdec_object_get (s, glyph->font);
    if (fontobj == NULL)
      continue;

    for (j = 0; j < text->glyphs->len; j++) {
      ArtVpath *vpath, *vpath0, *vpath1;
      ArtBpath *bpath0, *bpath1;
      SwfdecTextGlyph *glyph;
      SwfdecShape *shape;
      double glyph_trans[6];
      double trans[6];
      double pos[6];

      glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, j);

      shape = swfdec_font_get_glyph (SWFDEC_FONT (fontobj), glyph->glyph);
      art_affine_translate (pos,
	  glyph->x * SWF_SCALE_FACTOR, glyph->y * SWF_SCALE_FACTOR);
      pos[0] = glyph->height * SWF_TEXT_SCALE_FACTOR;
      pos[3] = glyph->height * SWF_TEXT_SCALE_FACTOR;
      art_affine_multiply (glyph_trans, pos, object->trans);
      art_affine_multiply (trans, glyph_trans, layer->transform);
      if (s->subpixel)
	art_affine_subpixel (trans);

      layer->fills = g_array_set_size (layer->fills, layer->fills->len + 1);
      layervec =
	  &g_array_index (layer->fills, SwfdecLayerVec, layer->fills->len - 1);

      shapevec = g_ptr_array_index (shape->fills, 0);
      shapevec2 = g_ptr_array_index (shape->fills2, 0);
      layervec->color = transform_color (glyph->color,
	  seg->color_mult, seg->color_add);

      bpath0 =
	  art_bpath_affine_transform (&g_array_index (shapevec->path, ArtBpath,
	      0), trans);
      bpath1 =
	  art_bpath_affine_transform (&g_array_index (shapevec2->path, ArtBpath,
	      0), trans);
      vpath0 = art_bez_path_to_vec (bpath0, s->flatness);
      vpath1 = art_bez_path_to_vec (bpath1, s->flatness);
      vpath1 = art_vpath_reverse_free (vpath1);
      vpath = art_vpath_cat (vpath0, vpath1);
      art_vpath_bbox_irect (vpath, &layervec->rect);
      layervec->svp = art_svp_from_vpath (vpath);
      art_svp_make_convex (layervec->svp);
      art_irect_union_to_masked (&layer->rect, &layervec->rect, &s->irect);

      art_free (bpath0);
      art_free (bpath1);
      art_free (vpath0);
      art_free (vpath1);
      art_free (vpath);
    }
  }

  return layer;
}

void
swfdec_text_render (SwfdecDecoder * s, SwfdecLayer * layer,
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
