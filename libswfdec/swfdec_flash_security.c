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
    return g_object_ref (guard);
  } else if (SWFDEC_IS_FLASH_SECURITY (key)) {
    SWFDEC_FIXME ("merging flash securities - how is this supposed to work?");
    return key;
  } else {
    SWFDEC_ERROR ("unknown security %s, denying access", G_OBJECT_TYPE_NAME (key));
    return NULL;
  }
}

static gboolean
swfdec_flash_security_allow_url (SwfdecSecurity *guard, const SwfdecURL *url)
{
  SwfdecFlashSecurity *sec = SWFDEC_FLASH_SECURITY (guard);

  if (swfdec_url_has_protocol (url, "http")) {
    return sec->allow_remote;
  } else if (swfdec_url_has_protocol (url, "file")) {
    return sec->allow_local;
  } else {
    SWFDEC_ERROR ("unknown protocol %s, denying access", swfdec_url_get_protocol (url));
    return FALSE;
  }
}

static void
swfdec_flash_security_class_init (SwfdecFlashSecurityClass *klass)
{
  SwfdecSecurityClass *security_class = SWFDEC_SECURITY_CLASS (klass);

  security_class->allow = swfdec_flash_security_allow;
  security_class->allow_url = swfdec_flash_security_allow_url;
}

static void
swfdec_flash_security_init (SwfdecFlashSecurity *sec)
{
}

/**
 * swfdec_flash_security_new:
 * @allow_local: %TRUE to allow playback of local files
 * @allow_remote: %TRUE to allow playback of remote files
 *
 * Creates a new Security object that allows everything. These objects are used 
 * by default when no other security object is in use. This is particularly 
 * useful for script engines that are not security sensitive or code injection
 * via debugging.
 *
 * Returns: a new #SwfdecSecurity object
 **/
SwfdecSecurity *
swfdec_flash_security_new (gboolean allow_local, gboolean allow_remote)
{
  SwfdecFlashSecurity *ret;
  
  ret = g_object_new (SWFDEC_TYPE_FLASH_SECURITY, NULL);
  ret->allow_local = allow_local;
  ret->allow_remote = allow_remote;
  return SWFDEC_SECURITY (ret);
}
