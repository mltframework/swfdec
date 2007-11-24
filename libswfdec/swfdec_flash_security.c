/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include <string.h>
#include "swfdec_flash_security.h"
#include "swfdec_debug.h"
#include "swfdec_security_allow.h"


G_DEFINE_TYPE (SwfdecFlashSecurity, swfdec_flash_security, SWFDEC_TYPE_SECURITY)

static gboolean
swfdec_flash_security_allow (SwfdecSecurity *guard, SwfdecSecurity *key)
{
  if (guard == key) {
    return TRUE;
  } else if (SWFDEC_IS_SECURITY_ALLOW (key)) {
    /* This only happens when calling functions (I hope) */
    return TRUE;
  } else if (SWFDEC_IS_FLASH_SECURITY (key)) {
    SWFDEC_FIXME ("implement security checking");
    return TRUE;
  } else {
    SWFDEC_ERROR ("unknown security %s, denying access", G_OBJECT_TYPE_NAME (key));
    return FALSE;
  }
}

static gboolean
swfdec_flash_security_match_domain (const SwfdecURL *guard, const SwfdecURL *key)
{
  return g_ascii_strcasecmp (swfdec_url_get_host (guard), swfdec_url_get_host (key)) == 0;
}

static void
swfdec_flash_security_allow_cross_domain (SwfdecFlashSecurity *sec,
    const SwfdecURL *url, SwfdecURLAllowFunc callback, gpointer user_data)
{
  //SwfdecAllowURLPending *pending;
  const char *host;

  host = swfdec_url_get_host (url);

  if (g_slist_find_custom (sec->crossdomain_allowed, host,
	(GCompareFunc)g_ascii_strcasecmp)) {
    callback (url, TRUE, user_data);
    return;
  }

  if (g_slist_find_custom (sec->crossdomain_denied, host,
	(GCompareFunc)g_ascii_strcasecmp)) {
    callback (url, FALSE, user_data);
    return;
  }

  // TODO
  callback (url, FALSE, user_data);
  /*pending = g_new (SwfdecAllowURLPending, 1);
  pending->url = url;
  pending->callback = callback;
  pending->user_data = user_data;

  sec->allowurl_pending = g_slist_prepend (sec->allowurl_pending, pending);

  if (g_slist_find_custom (sec->crossdomain_pending, host, g_ascii_strcasecmp))
    return;
  */
}

static void
swfdec_flash_security_allow_url (SwfdecSecurity *guard, const SwfdecURL *url,
    SwfdecURLAllowFunc callback, gpointer user_data)
{
  SwfdecFlashSecurity *sec = SWFDEC_FLASH_SECURITY (guard);
  gboolean allowed;

  switch (sec->sandbox) {
    case SWFDEC_SANDBOX_NONE:
      allowed = FALSE;
      break;
    case SWFDEC_SANDBOX_REMOTE:
      if (swfdec_url_is_local (url)) {
	allowed = FALSE;
      } else if (swfdec_flash_security_match_domain (sec->url, url)) {
	allowed = TRUE;
      } else {
	swfdec_flash_security_allow_cross_domain (sec, url, callback,
	    user_data);
	return;
      }
      break;
    case SWFDEC_SANDBOX_LOCAL_FILE:
      allowed = swfdec_url_is_local (url);
      break;
    case SWFDEC_SANDBOX_LOCAL_NETWORK:
      if (swfdec_url_is_local (url)) {
	allowed = FALSE;
      } else {
	swfdec_flash_security_allow_cross_domain (sec, url, callback,
	    user_data);
	return;
      }
      break;
    case SWFDEC_SANDBOX_LOCAL_TRUSTED:
      allowed = TRUE;
      break;
    default:
      g_assert_not_reached ();
  }

  callback (url, allowed, user_data);
}

static void
swfdec_flash_security_dispose (GObject *object)
{
  SwfdecFlashSecurity *sec = SWFDEC_FLASH_SECURITY (object);

  if (sec->url) {
    swfdec_url_free (sec->url);
    sec->url = NULL;
  }
  sec->sandbox = SWFDEC_SANDBOX_NONE;
  G_OBJECT_CLASS (swfdec_flash_security_parent_class)->dispose (object);
}

static void
swfdec_flash_security_class_init (SwfdecFlashSecurityClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecSecurityClass *security_class = SWFDEC_SECURITY_CLASS (klass);

  object_class->dispose = swfdec_flash_security_dispose;

  security_class->allow = swfdec_flash_security_allow;
  security_class->allow_url = swfdec_flash_security_allow_url;
}

static void
swfdec_flash_security_init (SwfdecFlashSecurity *sec)
{
}

void
swfdec_flash_security_set_url (SwfdecFlashSecurity *sec, const SwfdecURL *url)
{
  g_return_if_fail (SWFDEC_IS_FLASH_SECURITY (sec));
  g_return_if_fail (sec->url == NULL);
  g_return_if_fail (url != NULL);

  g_assert (sec->sandbox == SWFDEC_SANDBOX_NONE);
  sec->url = swfdec_url_copy (url);
  if (swfdec_url_is_local (url)) {
    sec->sandbox = SWFDEC_SANDBOX_LOCAL_FILE;
  } else {
    sec->sandbox = SWFDEC_SANDBOX_REMOTE;
  }
}

