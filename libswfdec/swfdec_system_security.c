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

SWFDEC_AS_NATIVE (12, 2, swfdec_system_security_loadPolicyFile)
void
swfdec_system_security_loadPolicyFile (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.security.loadPolicyFile (static)");
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
  SWFDEC_STUB ("System.security.sandboxType (static, get)");
}

SWFDEC_AS_NATIVE (12, 6, swfdec_system_security_set_sandboxType)
void
swfdec_system_security_set_sandboxType (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("System.security.sandboxType (static, set)");
}
