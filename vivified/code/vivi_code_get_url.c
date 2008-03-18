/* Vivified
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

#include "vivi_code_get_url.h"

G_DEFINE_TYPE (ViviCodeGetUrl, vivi_code_get_url, VIVI_TYPE_CODE_STATEMENT)

static void
vivi_code_get_url_dispose (GObject *object)
{
  ViviCodeGetUrl *get_url = VIVI_CODE_GET_URL (object);

  g_object_unref (get_url->target);
  g_object_unref (get_url->url);

  G_OBJECT_CLASS (vivi_code_get_url_parent_class)->dispose (object);
}

static char *
vivi_code_get_url_to_code (ViviCodeToken *token)
{
  ViviCodeGetUrl *url = VIVI_CODE_GET_URL (token);
  char *t, *u, *ret;
  const char *name;

  if (url->variables) {
    name = "loadVariables";
  } else if (url->internal) {
    name = "loadMovie";
  } else {
    name = "getURL";
  }
  u = vivi_code_token_to_code (VIVI_CODE_TOKEN (url->url));
  t = vivi_code_token_to_code (VIVI_CODE_TOKEN (url->target));
  if (url->method == 0) {
    ret = g_strdup_printf ("%s (%s, %s);\n", name, u, t);
  } else {
    ret = g_strdup_printf ("%s (%s, %s, %s);\n", name, u, t,
	url->method == 2 ? "\"POST\"" : "\"GET\"");
  }
  g_free (t);
  g_free (u);

  return ret;
}

static void
vivi_code_get_url_class_init (ViviCodeGetUrlClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  ViviCodeTokenClass *token_class = VIVI_CODE_TOKEN_CLASS (klass);

  object_class->dispose = vivi_code_get_url_dispose;

  token_class->to_code = vivi_code_get_url_to_code;
}

static void
vivi_code_get_url_init (ViviCodeGetUrl *token)
{
}

ViviCodeToken *
vivi_code_get_url_new (ViviCodeValue *target, ViviCodeValue *url,
    SwfdecLoaderRequest method, gboolean internal, gboolean variables)
{
  ViviCodeGetUrl *ret;

  g_return_val_if_fail (VIVI_IS_CODE_VALUE (target), NULL);
  g_return_val_if_fail (VIVI_IS_CODE_VALUE (url), NULL);
  g_return_val_if_fail (method < 4, NULL);

  ret = g_object_new (VIVI_TYPE_CODE_GET_URL, NULL);
  ret->target = target;
  ret->url = url;
  ret->method = method;
  ret->internal = internal;
  ret->variables = variables;

  return VIVI_CODE_TOKEN (ret);
}

