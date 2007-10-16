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
#include "swfdec_load_object.h"
#include "swfdec_as_internal.h"
#include "swfdec_debug.h"
#include "swfdec_loader_internal.h"
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"

SWFDEC_AS_NATIVE (301, 0, swfdec_load_object_load)
void
swfdec_load_object_load (SwfdecAsContext *cx, SwfdecAsObject *obj, guint argc,
    SwfdecAsValue *argv, SwfdecAsValue *rval)
{
  const char *url;

  if (argc < 1) {
    SWFDEC_AS_VALUE_SET_BOOLEAN (rval, FALSE);
    return;
  }

  url = swfdec_as_value_to_string (cx, &argv[0]);
  swfdec_load_object_new (obj, url, SWFDEC_LOADER_REQUEST_DEFAULT, NULL);

  SWFDEC_AS_VALUE_SET_BOOLEAN (rval, TRUE);
}
