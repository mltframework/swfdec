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
#include "swfdec_as_context.h"
#include "swfdec_debug.h"

SWFDEC_AS_NATIVE (111, 100, swfdec_print_job_start)
void
swfdec_print_job_start (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("PrintJob.start");
}

SWFDEC_AS_NATIVE (111, 101, swfdec_print_job_addPage)
void
swfdec_print_job_addPage (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("PrintJob.addPage");
}

SWFDEC_AS_NATIVE (111, 102, swfdec_print_job_send)
void
swfdec_print_job_send (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("PrintJob.send");
}

static void
swfdec_print_job_get_orientation (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("PrintJob.orientation (get)");
}

static void
swfdec_print_job_get_pageHeight (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("PrintJob.pageHeight (get)");
}

static void
swfdec_print_job_get_pageWidth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("PrintJob.pageWidth (get)");
}

static void
swfdec_print_job_get_paperHeight (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("PrintJob.paperHeight (get)");
}

static void
swfdec_print_job_get_paperWidth (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("PrintJob.paperWidth (get)");
}

static void
swfdec_print_job_init_properties (SwfdecAsContext *cx)
{
  SwfdecAsValue val;
  SwfdecAsObject *xml, *proto;

  // FIXME: We should only initialize if the prototype Object has not been
  // initialized by any object's constructor with native properties
  // (TextField, TextFormat, XML, XMLNode at least)

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (cx));

  swfdec_as_object_get_variable (cx->global, SWFDEC_AS_STR_PrintJob, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  xml = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_get_variable (xml, SWFDEC_AS_STR_prototype, &val);
  if (!SWFDEC_AS_VALUE_IS_OBJECT (&val))
    return;
  proto = SWFDEC_AS_VALUE_GET_OBJECT (&val);

  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_orientation,
      swfdec_print_job_get_orientation, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_pageHeight,
      swfdec_print_job_get_pageHeight, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_pageWidth,
      swfdec_print_job_get_pageWidth, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_paperHeight,
      swfdec_print_job_get_paperHeight, NULL);
  swfdec_as_object_add_native_variable (proto, SWFDEC_AS_STR_paperWidth,
      swfdec_print_job_get_paperWidth, NULL);
}

SWFDEC_AS_NATIVE (111, 0, swfdec_print_job_construct)
void
swfdec_print_job_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("PrintJob");

  swfdec_print_job_init_properties (cx);
}
