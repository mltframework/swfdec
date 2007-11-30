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
#include "swfdec_policy_loader.h"
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

typedef struct {
  SwfdecURL *		url;
  SwfdecURLAllowFunc	callback;
  gpointer		user_data;
} SwfdecAllowURLPending;

static void
swfdec_flash_security_call_pending (SwfdecFlashSecurity *sec, const char *host,
    gboolean allow)
{
  GSList *iter, *prev, *next;
  SwfdecAllowURLPending *pending;

  g_return_if_fail (SWFDEC_IS_FLASH_SECURITY (sec));
  g_return_if_fail (host != NULL);

  prev = NULL;
  for (iter = sec->allow_url_pending; iter != NULL; iter = next) {
    next = iter->next;
    pending = iter->data;

    if (!g_ascii_strcasecmp (swfdec_url_get_host (pending->url), host)) {
      pending->callback (pending->url, allow, pending->user_data);
      swfdec_url_free (pending->url);
      g_free (pending);
      g_slist_free_1 (iter);

      if (prev != NULL) {
	prev->next = iter->next;
	iter = prev;
	iter->next = next;
      } else {
	iter = NULL;
	sec->allow_url_pending = next;
      }
    }

    prev = iter;
  }
}

static void
swfdec_flash_security_policy_loader_done (SwfdecPolicyLoader *policy_loader,
    gboolean allow)
{
  SwfdecFlashSecurity *sec = policy_loader->sec;
  char *host = g_strdup (policy_loader->host);

  if (allow) {
    sec->crossdomain_allowed = g_slist_prepend (sec->crossdomain_allowed, host);
  } else {
    sec->crossdomain_denied = g_slist_prepend (sec->crossdomain_denied, host);
  }

  sec->policy_loaders = g_slist_remove (sec->policy_loaders, policy_loader);

  swfdec_flash_security_call_pending (sec, host, allow);

  swfdec_policy_loader_free (policy_loader);
}

static void
swfdec_flash_security_get_cross_domain_policy (SwfdecFlashSecurity *sec,
    const char *host)
{
  GSList *iter;
  SwfdecPolicyLoader *policy_loader;

  g_return_if_fail (SWFDEC_IS_FLASH_SECURITY (sec));
  g_return_if_fail (host != NULL);

  for (iter = sec->policy_loaders; iter != NULL; iter = iter->next) {
    policy_loader = iter->data;

    if (!g_ascii_strcasecmp (policy_loader->host, host))
      return;
  }

  policy_loader = swfdec_policy_loader_new (sec, host,
      swfdec_flash_security_policy_loader_done);
  if (policy_loader == NULL) {
    sec->crossdomain_denied = g_slist_prepend (sec->crossdomain_denied,
	g_strdup (host));
    swfdec_flash_security_call_pending (sec, host, FALSE);
    return;
  }

  sec->policy_loaders = g_slist_prepend (sec->policy_loaders, policy_loader);
}

static void
swfdec_flash_security_allow_cross_domain (SwfdecFlashSecurity *sec,
    const SwfdecURL *url, SwfdecURLAllowFunc callback, gpointer user_data)
{
  SwfdecAllowURLPending *pending;
  const char *host;

  g_assert (SWFDEC_IS_FLASH_SECURITY (sec));
  g_assert (url != NULL);
  g_assert (callback != NULL);

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

  pending = g_new (SwfdecAllowURLPending, 1);
  pending->url = swfdec_url_copy (url);
  pending->callback = callback;
  pending->user_data = user_data;

  sec->allow_url_pending = g_slist_prepend (sec->allow_url_pending, pending);

  swfdec_flash_security_get_cross_domain_policy (sec, host);
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
  GSList *iter;

  for (iter = sec->policy_loaders; iter != NULL; iter = iter->next) {
    swfdec_policy_loader_free (iter->data);
  }
  g_slist_free (sec->policy_loaders);
  sec->policy_loaders = NULL;

  for (iter = sec->crossdomain_allowed; iter != NULL; iter = iter->next) {
    g_free (iter->data);
  }
  g_slist_free (sec->crossdomain_allowed);
  sec->crossdomain_allowed = NULL;

  for (iter = sec->crossdomain_denied; iter != NULL; iter = iter->next) {
    g_free (iter->data);
  }
  g_slist_free (sec->crossdomain_denied);
  sec->crossdomain_denied = NULL;

  for (iter = sec->allow_url_pending; iter != NULL; iter = iter->next) {
    g_free (iter->data);
  }
  g_slist_free (sec->allow_url_pending);
  sec->allow_url_pending = NULL;

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

