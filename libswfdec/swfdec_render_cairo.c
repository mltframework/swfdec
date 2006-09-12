
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>
#include <cairo.h>

#include "swfdec_internal.h"

void
swfdec_shape_render (SwfdecDecoder *s, cairo_t *cr, 
    const SwfdecColorTransform *trans, SwfdecObject *obj, SwfdecRect *inval)
{
  SwfdecShape *shape = SWFDEC_SHAPE (obj);
  unsigned int i;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;

  if (obj->id == 7)
    return;

  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);
  cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
  for (i = 0; i < shape->fills->len; i++) {
    swf_color color;

    shapevec = g_ptr_array_index (shape->fills, i);
    shapevec2 = g_ptr_array_index (shape->fills2, i);

    color = swfdec_color_apply_transform (shapevec->color, trans);

    switch (shapevec->fill_type) {
      case 0x10:
        {
          SwfdecGradient *grad = shapevec->grad;
          int j;
	  cairo_pattern_t *pattern;
          cairo_matrix_t mat;

          mat = shapevec->fill_transform;
          if (cairo_matrix_invert (&mat)) {
	    g_assert_not_reached ();
	  }

          pattern = cairo_pattern_create_linear (-16384.0, 0, 16384.0, 0);
          cairo_pattern_set_matrix(pattern, &mat);
          for (j=0;j<grad->n_gradients;j++){
            color = swfdec_color_apply_transform (grad->array[j].color,
                trans);

            cairo_pattern_add_color_stop_rgba (pattern,
                grad->array[j].ratio/256.0,
                SWF_COLOR_R(color)/255.0, SWF_COLOR_G(color)/255.0,
                SWF_COLOR_B(color)/255.0, SWF_COLOR_A(color)/255.0);
          }
          cairo_set_source (cr, pattern);
	  cairo_pattern_destroy (pattern);
        }
        break;
      case 0x12:
        {
          SwfdecGradient *grad = shapevec->grad;
          int j;
          cairo_matrix_t mat;
	  cairo_pattern_t *pattern;

          mat = shapevec->fill_transform;
          if (cairo_matrix_invert (&mat)) {
	    g_assert_not_reached ();
	  }

          pattern = cairo_pattern_create_radial (0,0,0,
             0, 0, 16384);
          cairo_pattern_set_matrix(pattern, &mat);
          for (j=0;j<grad->n_gradients;j++){
            color = swfdec_color_apply_transform (grad->array[j].color,
		trans);

            cairo_pattern_add_color_stop_rgba (pattern,
                grad->array[j].ratio/256.0,
                SWF_COLOR_R(color)/255.0, SWF_COLOR_G(color)/255.0,
                SWF_COLOR_B(color)/255.0, SWF_COLOR_A(color)/255.0);
          }
          cairo_set_source (cr, pattern);
	  cairo_pattern_destroy (pattern);
        }
        break;
      case 0x40:
      case 0x41:
      case 0x42:
      case 0x43:
        {
          SwfdecImage *image;
          cairo_matrix_t mat;

          mat = shapevec->fill_transform;
          if (cairo_matrix_invert (&mat)) {
	    g_assert_not_reached ();
	  }

          image = (SwfdecImage *)swfdec_object_get (s, shapevec->fill_id);
          if (image && SWFDEC_IS_IMAGE (image)) {
	    cairo_surface_t *src_surface;
	    cairo_pattern_t *pattern;
            unsigned char *image_data = swfdec_handle_get_data(image->handle);

            src_surface = cairo_image_surface_create_for_data (image_data,
                CAIRO_FORMAT_ARGB32, image->width, image->height,
                image->rowstride);
            pattern = cairo_pattern_create_for_surface (src_surface);
	    cairo_surface_destroy (src_surface);
            cairo_pattern_set_matrix(pattern, &mat);
            if (shapevec->fill_type == 0x40 || shapevec->fill_type == 0x42) {
              cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
            } else {
              cairo_pattern_set_extend (pattern, CAIRO_EXTEND_NONE);
            }
            if (shapevec->fill_type == 0x40 || shapevec->fill_type == 0x41) {
              cairo_pattern_set_filter (pattern, CAIRO_FILTER_BILINEAR);
            } else {
              cairo_pattern_set_filter (pattern, CAIRO_FILTER_NEAREST);
            }
            cairo_set_source (cr, pattern);
	    cairo_pattern_destroy (pattern);
          } else {
            swfdec_color_set_source (cr, color);
          }
        }
        break;
      case 0x00:
        swfdec_color_set_source (cr, color);
        break;
      default:
        SWFDEC_ERROR("unhandled fill type 0x%02x", shapevec->fill_type);
        cairo_set_source_rgba (cr, 0.5, 0.5, 0.5, 0.5);
        break;
    }
    
    if (shapevec->path.num_data || shapevec2->path.num_data) {
      cairo_new_path (cr);
      if (shapevec->path.num_data)
	cairo_append_path (cr, &shapevec->path);
      if (shapevec2->path.num_data)
	cairo_append_path (cr, &shapevec2->path);
      cairo_fill (cr);
    }
  }

  for (i = 0; i < shape->lines->len; i++) {
    swf_color color;
    cairo_matrix_t matrix;

    shapevec = g_ptr_array_index (shape->lines, i);

    color = swfdec_color_apply_transform (shapevec->color, trans);
    cairo_set_source_rgba (cr, SWF_COLOR_R(color)/255.0,
        SWF_COLOR_G(color)/255.0, SWF_COLOR_B(color)/255.0,
        SWF_COLOR_A(color)/255.0);

    cairo_get_matrix (cr, &matrix);
    cairo_set_line_width (cr, shapevec->width);
    if (shapevec->path.num_data) {
      cairo_new_path (cr);
      cairo_append_path (cr, &shapevec->path);
      cairo_stroke (cr);
    }
  }

  //return TRUE;
}

#if 0
void
swfdec_text_render (SwfdecDecoder * s, SwfdecSpriteSegment * seg,
    SwfdecObject * object)
{
  unsigned int i;
  SwfdecText *text;
  SwfdecShapeVec *shapevec;
  SwfdecShapeVec *shapevec2;
  SwfdecObject *fontobj;
  SwfdecLayer *layer;
  cairo_t *cr = s->backend_private;

  layer = swfdec_layer_new ();
  layer->seg = seg;
  cairo_matrix_multiply (&layer->transform, &seg->transform, &s->transform);

  layer->rect.x0 = 0;
  layer->rect.x1 = 0;
  layer->rect.y0 = 0;
  layer->rect.y1 = 0;

  text = SWFDEC_TEXT (object);
  for (i = 0; i < text->glyphs->len; i++) {
    SwfdecTextGlyph *glyph;
    SwfdecShape *shape;
    cairo_matrix_t glyph_trans, trans, pos;
    swf_color color;

    glyph = &g_array_index (text->glyphs, SwfdecTextGlyph, i);

    fontobj = swfdec_object_get (s, glyph->font);
    if (fontobj == NULL)
      continue;

    shape = swfdec_font_get_glyph (SWFDEC_FONT (fontobj), glyph->glyph);
    if (shape == NULL) {
      SWFDEC_ERROR ("failed getting glyph %d\n", glyph->glyph);
      continue;
    }

    cairo_matrix_init_translate (&pos,
	glyph->x * SWF_SCALE_FACTOR, glyph->y * SWF_SCALE_FACTOR);
    pos.xx = glyph->height * SWF_TEXT_SCALE_FACTOR;
    pos.yy = glyph->height * SWF_TEXT_SCALE_FACTOR;
    cairo_matrix_multiply (&glyph_trans, &pos, &object->transform);
    cairo_matrix_multiply (&trans, &glyph_trans, &layer->transform);

    layer->fills = g_array_set_size (layer->fills, layer->fills->len + 1);

    shapevec = g_ptr_array_index (shape->fills, 0);
    shapevec2 = g_ptr_array_index (shape->fills2, 0);

    color = swfdec_color_apply_transform (glyph->color, &seg->color_transform);
    cairo_set_source_rgba (cr, SWF_COLOR_R(color)/255.0,
        SWF_COLOR_G(color)/255.0, SWF_COLOR_B(color)/255.0,
        SWF_COLOR_A(color)/255.0);

    cairo_save (cr);

    cairo_set_fill_rule (cr, CAIRO_FILL_RULE_EVEN_ODD);
    draw_x (cr, shapevec->path, shapevec2->path);
    cairo_fill (cr);

    cairo_restore (cr);
  }

  swfdec_layer_free (layer);
}
#endif

void
swf_config_colorspace (SwfdecDecoder * s)
{
  switch (s->colorspace) {
    case SWF_COLORSPACE_RGB565:
      s->stride = s->width * 2;
      s->bytespp = 2;
//      s->callback = (void *) art_rgb565_svp_alpha_callback;
//      s->compose_callback = (void *) art_rgb565_svp_alpha_callback;
//      s->fillrect = art_rgb565_fillrect;
      break;
    case SWF_COLORSPACE_RGB888:
    default:
      s->stride = s->width * 4;
      s->bytespp = 4;
//      s->callback = (void *) art_rgb_svp_alpha_callback;
//      s->compose_callback = (void *) art_rgb_svp_alpha_compose_callback;
//      s->fillrect = art_rgb_fillrect;
      break;
  }
}

void
swfdec_render_layervec_free (SwfdecLayerVec * layervec)
{
#if 0
  if (layervec->svp) {
    art_svp_free (layervec->svp);
  }
#endif
}

