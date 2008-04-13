/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "swfdec_renderer.h"
#include "swfdec_renderer_internal.h"
#include "swfdec_cache.h"
#include "swfdec_player_internal.h"

struct _SwfdecRendererPrivate {
  cairo_surface_t *	surface;	/* the surface we assume we render to */
  SwfdecCache *		cache;		/* the cache we use for cached items */
  GHashTable *		cache_lookup;	/* gpointer => GList mapping */
};

/*** GTK-DOC ***/

/**
 * SECTION:SwfdecRenderer
 * @title: SwfdecRenderer
 * @short_description: provide accelerated rendering and caching abilities
 *
 * The #SwfdecRenderer object is used internally to improve rendering done by
 * Swfdec.
 *
 * The first thing #SwfdecRenderer does is provide a way to cache data relevant
 * to rendering.
 *
 * The second thing it does is provide access to the surface that is used for 
 * rendering, even when not in the process of rendering. This is relevant for
 * font backends, as different surfaces provide different native fonts. See
 * swfdec_player_set_default_backend() for details about this.
 *
 * The third thing it does is provide a list of virtual functions for critical
 * operations that you can optimize using subclasses to provide faster 
 * implementations. Note that a working default implementation is provided, so
 * you only need to override the functions you care about. 
 * See #SwfdecRendererClass for details about these functions.
 */


/*** OBJECT ***/

G_DEFINE_TYPE (SwfdecRenderer, swfdec_renderer, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_SURFACE
};

static void
swfdec_renderer_dispose (GObject *object)
{
  SwfdecRendererPrivate *priv = SWFDEC_RENDERER (object)->priv;

  if (priv->surface) {
    cairo_surface_destroy (priv->surface);
    priv->surface = NULL;
  }
  if (priv->cache_lookup) {
    GHashTableIter iter;
    GList *walk;
    gpointer list;
    g_hash_table_iter_init (&iter, priv->cache_lookup);
    while (g_hash_table_iter_next (&iter, NULL, &list)) {
      for (walk = list; walk; walk = walk->next) {
	g_object_remove_weak_pointer (walk->data, &walk->data);
      }
      g_list_free (list);
    }
    g_hash_table_destroy (priv->cache_lookup);
    priv->cache_lookup = NULL;
  }
  if (priv->cache) {
    g_object_unref (priv->cache);
    priv->cache = NULL;
  }

  G_OBJECT_CLASS (swfdec_renderer_parent_class)->dispose (object);
}

static void
swfdec_renderer_get_property (GObject *object, guint param_id, GValue *value,
    GParamSpec *pspec)
{
  SwfdecRendererPrivate *priv = SWFDEC_RENDERER (object)->priv;

  switch (param_id) {
    case PROP_SURFACE:
      g_value_set_pointer (value, priv->surface);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_renderer_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecRendererPrivate *priv = SWFDEC_RENDERER (object)->priv;

  switch (param_id) {
    case PROP_SURFACE:
      priv->surface = g_value_get_pointer (value);
      g_assert (priv->surface != NULL);
      cairo_surface_reference (priv->surface);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static cairo_surface_t *
swfdec_renderer_do_create_similar (SwfdecRenderer *renderer, cairo_surface_t *surface)
{
  SwfdecRendererPrivate *priv;
  cairo_surface_t *result;
  cairo_t *cr;

  priv = renderer->priv;
  if (cairo_surface_get_type (priv->surface) == CAIRO_SURFACE_TYPE_IMAGE)
    return surface;

  result = cairo_surface_create_similar (priv->surface,
      cairo_surface_get_content (surface),
      cairo_image_surface_get_width (surface),
      cairo_image_surface_get_height (surface));
  cr = cairo_create (result);

  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);

  cairo_destroy (cr);
  cairo_surface_destroy (surface);
  return result;
}

static cairo_surface_t *
swfdec_renderer_do_create_for_data (SwfdecRenderer *renderer, guint8 *data,
    cairo_format_t format, guint width, guint height, guint rowstride)
{
  static const cairo_user_data_key_t key;
  cairo_surface_t *surface;

  surface = cairo_image_surface_create_for_data (data, format, width, height, rowstride);
  cairo_surface_set_user_data (surface, &key, data, g_free);
  return swfdec_renderer_create_similar (renderer, surface);
}

static void
swfdec_renderer_class_init (SwfdecRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SwfdecRendererPrivate));

  object_class->dispose = swfdec_renderer_dispose;
  object_class->get_property = swfdec_renderer_get_property;
  object_class->set_property = swfdec_renderer_set_property;

  /* FIXME: should be g_param_spec_size(), but no such thing exists */
  g_object_class_install_property (object_class, PROP_SURFACE,
      g_param_spec_pointer ("surface", "surface", "cairo surface in use by this renderer",
	  G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));

  klass->create_similar = swfdec_renderer_do_create_similar;
  klass->create_for_data = swfdec_renderer_do_create_for_data;
}

static void
swfdec_renderer_init (SwfdecRenderer *renderer)
{
  SwfdecRendererPrivate *priv;

  renderer->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (renderer, SWFDEC_TYPE_RENDERER, SwfdecRendererPrivate);
  
  priv->cache = swfdec_cache_new (8 * 1024 * 1024);
  priv->cache_lookup = g_hash_table_new (g_direct_hash, g_direct_equal);
}

/*** INTERNAL API ***/

void
swfdec_renderer_add_cache (SwfdecRenderer *renderer, gboolean replace,
    gpointer key, SwfdecCached *cached)
{
  SwfdecRendererPrivate *priv;
  GList *list;

  g_return_if_fail (SWFDEC_IS_RENDERER (renderer));
  g_return_if_fail (key != NULL);
  g_return_if_fail (SWFDEC_IS_CACHED (cached));

  priv = renderer->priv;
  list = g_hash_table_lookup (priv->cache_lookup, key);
  if (replace) {
    GList *walk;
    for (walk = list; walk; walk = walk->next) {
      if (walk->data) {
	g_object_remove_weak_pointer (walk->data, &walk->data);
	swfdec_cached_unuse (walk->data);
      }
    }
    g_list_free (list);
    list = NULL;
  }
  list = g_list_prepend (list, cached);
  /* NB: empty list entries mean object was deleted */
  g_object_add_weak_pointer (G_OBJECT (cached), &list->data);
  g_hash_table_insert (priv->cache_lookup, key, list);
  swfdec_cache_add (priv->cache, cached);
}

SwfdecCached *
swfdec_renderer_get_cache (SwfdecRenderer *renderer, gpointer key, 
    SwfdecRendererSearchFunc func, gpointer data)
{
  SwfdecRendererPrivate *priv;
  GList *list, *org, *walk;
  SwfdecCached *result = NULL;

  g_return_val_if_fail (SWFDEC_IS_RENDERER (renderer), NULL);
  g_return_val_if_fail (key != NULL, NULL);

  priv = renderer->priv;
  org = g_hash_table_lookup (priv->cache_lookup, key);
  list = org;
  walk = list;
  while (walk) {
    /* NB: empty list entries mean object was deleted */
    if (walk->data == NULL) {
      GList *next = walk->next;
      list = g_list_delete_link (list, walk);
      walk = next;
      continue;
    }
    if (!func || func (walk->data, data)) {
      result = walk->data;
      break;
    }
    walk = walk->next;
  }
  if (org != list)
    g_hash_table_insert (priv->cache_lookup, key, list);
  return result;
}

SwfdecRenderer *
swfdec_renderer_new_default (SwfdecPlayer *player)
{
  cairo_surface_t *surface;
  SwfdecRenderer *renderer;

  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  surface = cairo_image_surface_create (CAIRO_FORMAT_RGB24, 1, 1);
  renderer = swfdec_renderer_new_for_player (surface, player);
  cairo_surface_destroy (surface);
  return renderer;
}

static const cairo_user_data_key_t cr_key;

void
swfdec_renderer_attach (SwfdecRenderer *renderer, cairo_t *cr)
{
  g_return_if_fail (SWFDEC_IS_RENDERER (renderer));
  g_return_if_fail (cr != NULL);

  g_object_ref (renderer);
  if (cairo_set_user_data (cr, &cr_key, renderer, g_object_unref) != CAIRO_STATUS_SUCCESS) {
    g_warning ("could not attach user data");
  }
}

SwfdecRenderer *
swfdec_renderer_get (cairo_t *cr)
{
  g_return_val_if_fail (cr != NULL, NULL);

  return cairo_get_user_data (cr, &cr_key);
}

/**
 * swfdec_renderer_create_similar:
 * @renderer: a renderer
 * @surface: an image surface; this function takes ownership of the passed-in image.
 *
 * Creates a surface with the same contents and size as the given image 
 * @surface, but optimized for use in @renderer. You should use this function 
 * before caching a surface.
 *
 * Returns: A surface with the same contents as @surface. You own a reference to the 
 *          returned surface.
 **/
cairo_surface_t *
swfdec_renderer_create_similar (SwfdecRenderer *renderer, cairo_surface_t *surface)
{
  SwfdecRendererClass *klass;

  g_return_val_if_fail (SWFDEC_IS_RENDERER (renderer), NULL);
  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (cairo_surface_get_type (surface) == CAIRO_SURFACE_TYPE_IMAGE, NULL);

  klass = SWFDEC_RENDERER_GET_CLASS (renderer);
  return klass->create_similar (renderer, surface);
}

/* FIXME: get data parameter const */
cairo_surface_t *
swfdec_renderer_create_for_data (SwfdecRenderer *renderer, guint8 *data,
    cairo_format_t format, guint width, guint height, guint rowstride)
{
  SwfdecRendererClass *klass;

  g_return_val_if_fail (SWFDEC_IS_RENDERER (renderer), NULL);
  g_return_val_if_fail (data != NULL, NULL);
  g_return_val_if_fail (width > 0, NULL);
  g_return_val_if_fail (height > 0, NULL);
  g_return_val_if_fail (rowstride > 0, NULL);

  klass = SWFDEC_RENDERER_GET_CLASS (renderer);
  return klass->create_for_data (renderer, data, format, width, height, rowstride);
}

/*** PUBLIC API ***/

/**
 * swfdec_renderer_new:
 * @surface: a cairo surface
 *
 * Creates a new renderer to be used with the given @surface.
 *
 * Returns: a new renderer
 **/
SwfdecRenderer *
swfdec_renderer_new (cairo_surface_t *surface)
{
  g_return_val_if_fail (surface != NULL, NULL);

  return g_object_new (SWFDEC_TYPE_RENDERER, "surface", surface, NULL);
}

SwfdecRenderer *
swfdec_renderer_new_for_player (cairo_surface_t *surface, SwfdecPlayer *player)
{
  SwfdecRendererPrivate *priv;
  SwfdecRenderer *renderer;

  g_return_val_if_fail (surface != NULL, NULL);
  g_return_val_if_fail (SWFDEC_IS_PLAYER (player), NULL);

  renderer = swfdec_renderer_new (surface);
  priv = renderer->priv;
  g_object_unref (priv->cache);
  priv->cache = g_object_ref (player->priv->cache);

  return renderer;
}

/**
 * swfdec_renderer_get_surface:
 * @renderer: a renderer
 *
 * Gets the surface that was used when creating this surface.
 *
 * Returns: the surface used by this renderer
 **/
cairo_surface_t *
swfdec_renderer_get_surface (SwfdecRenderer *renderer)
{
  g_return_val_if_fail (SWFDEC_IS_RENDERER (renderer), NULL);

  return renderer->priv->surface;
}

