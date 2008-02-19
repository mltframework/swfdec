/* Swfdec
 * Copyright (C) 2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#include "swfdec_as_internal.h"
#include "swfdec_debug.h"
#include "swfdec_as_strings.h"
#include "swfdec_resource.h"
#include "swfdec_player_internal.h"
#include "swfdec_policy_file.h"

// properties
SWFDEC_AS_NATIVE (12, 0, swfdec_system_security_allowDomain)
void
swfdec_system_security_allowDomain (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.security.allowDomain (static)");
}

SWFDEC_AS_NATIVE (12, 1, swfdec_system_security_allowInsecureDomain)
void
swfdec_system_security_allowInsecureDomain (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.security.allowInsecureDomain (static)");
}

static void
swfdec_system_security_do_loadPolicyFile (gpointer url, gpointer player)
{
  swfdec_policy_file_new (player, url);
}

SWFDEC_AS_NATIVE (12, 2, swfdec_system_security_loadPolicyFile)
void
swfdec_system_security_loadPolicyFile (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SwfdecPlayer *player;
  const char *url_string;
  SwfdecURL *url;

  SWFDEC_AS_CHECK (0, NULL, "s", &url_string);

  player = SWFDEC_PLAYER (cx);
  url = swfdec_player_create_url (player, url_string);
  swfdec_player_request_resource (player, swfdec_system_security_do_loadPolicyFile, 
      url, (GDestroyNotify) swfdec_url_free);
}

SWFDEC_AS_NATIVE (12, 3, swfdec_system_security_chooseLocalSwfPath)
void
swfdec_system_security_chooseLocalSwfPath (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.security.chooseLocalSwfPath (static)");
}

SWFDEC_AS_NATIVE (12, 4, swfdec_system_security_escapeDomain)
void
swfdec_system_security_escapeDomain (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.security.escapeDomain (static)");
}

SWFDEC_AS_NATIVE (12, 5, swfdec_system_security_get_sandboxType)
void
swfdec_system_security_get_sandboxType (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  switch (SWFDEC_SANDBOX (cx->global)->type) {
    case SWFDEC_SANDBOX_REMOTE:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_remote);
      break;

    case SWFDEC_SANDBOX_LOCAL_FILE:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_localWithFile);
      break;

    case SWFDEC_SANDBOX_LOCAL_NETWORK:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_localWithNetwork);
      break;

    case SWFDEC_SANDBOX_LOCAL_TRUSTED:
      SWFDEC_AS_VALUE_SET_STRING (ret, SWFDEC_AS_STR_localTrusted);
      break;

    case SWFDEC_SANDBOX_NONE:
    default:
      g_return_if_reached ();
  }
}

SWFDEC_AS_NATIVE (12, 6, swfdec_system_security_set_sandboxType)
void
swfdec_system_security_set_sandboxType (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  // read-only
}

// PolicyFileResolver

SWFDEC_AS_NATIVE (15, 0, swfdec_system_security_policy_file_resolver_resolve)
void
swfdec_system_security_policy_file_resolver_resolve (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.security.PolicyFileResolver.resolve");
}
