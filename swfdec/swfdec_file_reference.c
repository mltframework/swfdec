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
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"

SWFDEC_AS_NATIVE (2204, 0, swfdec_file_reference_browse)
void
swfdec_file_reference_browse (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.browse");
}

SWFDEC_AS_NATIVE (2204, 1, swfdec_file_reference_upload)
void
swfdec_file_reference_upload (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.upload");
}

SWFDEC_AS_NATIVE (2204, 2, swfdec_file_reference_download)
void
swfdec_file_reference_download (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.download");
}

SWFDEC_AS_NATIVE (2204, 3, swfdec_file_reference_cancel)
void
swfdec_file_reference_cancel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.cancel");
}

static void
swfdec_file_reference_get_creationDate (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.creationDate (get)");
}

static void
swfdec_file_reference_get_creator (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.creator (get)");
}

static void
swfdec_file_reference_get_modificationDate (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.modificationDate (get)");
}

static void
swfdec_file_reference_get_name (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.name (get)");
}

static void
swfdec_file_reference_get_size (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.size (get)");
}

static void
swfdec_file_reference_get_type (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("FileReference.type (get)");
}

// Note: this is not actually the flash.net.FileReference function, but is
// called from within that function
SWFDEC_AS_NATIVE (2204, 200, swfdec_file_reference_construct)
void
swfdec_file_reference_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecAsObject *target;
  SwfdecAsValue val;

  SWFDEC_STUB ("FileReference");

  if (argc > 0 && SWFDEC_AS_VALUE_IS_COMPOSITE (argv[0])) {
    target = SWFDEC_AS_VALUE_GET_COMPOSITE (argv[0]);
  } else {
    target = object;
  }
  if (target == NULL)
    return;

  swfdec_as_object_add_native_variable (target, SWFDEC_AS_STR_creationDate,
      swfdec_file_reference_get_creationDate, NULL);
  swfdec_as_object_add_native_variable (target, SWFDEC_AS_STR_creator,
      swfdec_file_reference_get_creator, NULL);
  swfdec_as_object_add_native_variable (target, SWFDEC_AS_STR_modificationDate,
      swfdec_file_reference_get_modificationDate, NULL);
  swfdec_as_object_add_native_variable (target, SWFDEC_AS_STR_name,
      swfdec_file_reference_get_name, NULL);
  swfdec_as_object_add_native_variable (target, SWFDEC_AS_STR_size,
      swfdec_file_reference_get_size, NULL);
  swfdec_as_object_add_native_variable (target, SWFDEC_AS_STR_type,
      swfdec_file_reference_get_type, NULL);

  SWFDEC_AS_VALUE_SET_STRING (&val, SWFDEC_AS_STR_undefined);
  swfdec_as_object_set_variable (target, SWFDEC_AS_STR_postData, &val);
}
