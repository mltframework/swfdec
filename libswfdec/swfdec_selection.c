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

SWFDEC_AS_NATIVE (600, 0, swfdec_selection_getBeginIndex)
void
swfdec_selection_getBeginIndex (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.getBeginIndex (static)");
}

SWFDEC_AS_NATIVE (600, 1, swfdec_selection_getEndIndex)
void
swfdec_selection_getEndIndex (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.getEndIndex (static)");
}

SWFDEC_AS_NATIVE (600, 2, swfdec_selection_getCaretIndex)
void
swfdec_selection_getCaretIndex (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.getCaretIndex (static)");
}

SWFDEC_AS_NATIVE (600, 3, swfdec_selection_getFocus)
void
swfdec_selection_getFocus (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.getFocus (static)");
}

SWFDEC_AS_NATIVE (600, 4, swfdec_selection_setFocus)
void
swfdec_selection_setFocus (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.setFocus (static)");
}

SWFDEC_AS_NATIVE (600, 5, swfdec_selection_setSelection)
void
swfdec_selection_setSelection (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Selection.setSelection (static)");
}
