
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>

#include "swfdec_internal.h"
#ifdef HAVE_LIBART
#include "swfdec_render_libart.h"
#endif


SwfdecLayer *
swfdec_layer_new (void)
{
  SwfdecLayer *layer;

  layer = g_new0 (SwfdecLayer, 1);

  layer->fills = g_array_new (FALSE, TRUE, sizeof (SwfdecLayerVec));
  layer->lines = g_array_new (FALSE, TRUE, sizeof (SwfdecLayerVec));

  return layer;
}

void
swfdec_layer_free (SwfdecLayer * layer)
{
  int i;
  SwfdecLayerVec *layervec;

  if (!layer) {
    g_warning ("layer==NULL");
    return;
  }

  for (i = 0; i < layer->fills->len; i++) {
    layervec = &g_array_index (layer->fills, SwfdecLayerVec, i);
    swfdec_render_layervec_free (layervec);
    if (layervec->compose) {
      g_free (layervec->compose);
    }
  }
  g_array_free (layer->fills, TRUE);
  for (i = 0; i < layer->lines->len; i++) {
    layervec = &g_array_index (layer->lines, SwfdecLayerVec, i);
    swfdec_render_layervec_free (layervec);
    if (layervec->compose) {
      g_free (layervec->compose);
    }
  }
  g_array_free (layer->lines, TRUE);

  if (layer->sublayers) {
    GList *g;

    for (g = g_list_first (layer->sublayers); g; g = g_list_next (g)) {
      swfdec_layer_free ((SwfdecLayer *) g->data);
    }
    g_list_free (layer->sublayers);
  }

  g_free (layer);
}

SwfdecLayer *
swfdec_render_get_sublayer (SwfdecLayer * layer, int depth, int frame)
{
  SwfdecLayer *l;
  GList *g;

  if (!layer)
    return NULL;

  for (g = g_list_first (layer->sublayers); g; g = g_list_next (g)) {
    l = (SwfdecLayer *) g->data;
#if 0
    printf ("compare %d==%d %d <= %d < %d\n",
	l->seg->depth, depth, l->seg->first_frame, frame, l->last_frame);
#endif
    if (l->seg->depth == depth && l->first_frame <= frame
	&& frame < l->last_frame)
      return l;
  }

  return NULL;
}

