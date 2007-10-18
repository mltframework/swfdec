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

static SwfdecSecurity *
swfdec_flash_security_allow (SwfdecSecurity *guard, SwfdecSecurity *key)
{
  if (guard == key) {
    return g_object_ref (guard);
  } else if (SWFDEC_IS_SECURITY_ALLOW (key)) {
    /* This only happens when calling functions (I hope) */
    return g_object_ref (guard);
  } else if (SWFDEC_IS_FLASH_SECURITY (key)) {
    /* FIXME: what do we do here? */
    return g_object_ref (key);
  } else {
    SWFDEC_ERROR ("unknown security %s, denying access", G_OBJECT_TYPE_NAME (key));
    return NULL;
  }
}

static gboolean
swfdec_flash_security_match_domain (const SwfdecURL *guard, const SwfdecURL *key)
{
  return g_ascii_strcasecmp (swfdec_url_get_host (guard), swfdec_url_get_host (key)) == 0;
}

static gboolean
swfdec_flash_security_allow_url (SwfdecSecurity *guard, const SwfdecURL *url)
{
  SwfdecFlashSecurity *sec = SWFDEC_FLASH_SECURITY (guard);

  switch (sec->sandbox) {
    case SWFDEC_SANDBOX_NONE:
      return FALSE;
    case SWFDEC_SANDBOX_REMOTE:
      if (swfdec_url_is_local (url))
	return FALSE;
      return swfdec_flash_security_match_domain (sec->url, url);
    case SWFDEC_SANDBOX_LOCAL_FILE:
      return swfdec_url_is_local (url);
    case SWFDEC_SANDBOX_LOCAL_NETWORK:
      return !swfdec_url_is_local (url);
    case SWFDEC_SANDBOX_LOCAL_TRUSTED:
      return TRUE;
  }
  g_assert_not_reached ();
  return FALSE;
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

