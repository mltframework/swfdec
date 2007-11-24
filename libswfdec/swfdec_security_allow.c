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
#include "swfdec_security_allow.h"
#include "swfdec_debug.h"


G_DEFINE_TYPE (SwfdecSecurityAllow, swfdec_security_allow, SWFDEC_TYPE_SECURITY)

static gboolean
swfdec_security_allow_allow (SwfdecSecurity *guard, SwfdecSecurity *key)
{
  return TRUE;
}

static void
swfdec_security_allow_allow_url (SwfdecSecurity *guard, const SwfdecURL *url,
    SwfdecURLAllowFunc callback, gpointer user_data)
{
  callback (url, TRUE, user_data);
}

static void
swfdec_security_allow_class_init (SwfdecSecurityAllowClass *klass)
{
  SwfdecSecurityClass *security_class = SWFDEC_SECURITY_CLASS (klass);

  security_class->allow = swfdec_security_allow_allow;
  security_class->allow_url = swfdec_security_allow_allow_url;
}

static void
swfdec_security_allow_init (SwfdecSecurityAllow *security_allow)
{
}

/**
 * swfdec_security_allow_new:
 *
 * Creates a new Security object that allows everything. These objects are used 
 * by default when no other security object is in use. This is particularly 
 * useful for script engines that are not security sensitive or code injection
 * via debugging.
 *
 * Returns: a new #SwfdecSecurity object
 **/
SwfdecSecurity *
swfdec_security_allow_new (void)
{
  static SwfdecSecurity *singleton = NULL;
  
  /* FIXME: not threadsafe */
  if (singleton)
    return g_object_ref (singleton);

  singleton = g_object_new (SWFDEC_TYPE_SECURITY_ALLOW, NULL);
  g_object_add_weak_pointer (G_OBJECT (singleton), (gpointer) &singleton);
  return singleton;
}
