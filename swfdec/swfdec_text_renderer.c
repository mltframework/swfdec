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

SWFDEC_AS_NATIVE (2150, 1, swfdec_text_renderer_setAdvancedAntialiasingTable)
void
swfdec_text_renderer_setAdvancedAntialiasingTable (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("TextRenderer.setAdvancedAntialiasingTable (static)");
}

SWFDEC_AS_NATIVE (2150, 2, swfdec_text_renderer_get_antiAliasType)
void
swfdec_text_renderer_get_antiAliasType (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("TextRenderer.antiAliasType (static, get, not-really-named)");
}

SWFDEC_AS_NATIVE (2150, 3, swfdec_text_renderer_set_antiAliasType)
void
swfdec_text_renderer_set_antiAliasType (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("TextRenderer.antiAliasType (static, set, not-really-named)");
}

SWFDEC_AS_NATIVE (2150, 4, swfdec_text_renderer_get_maxLevel)
void
swfdec_text_renderer_get_maxLevel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("TextRenderer.maxLevel (static, get)");
}

SWFDEC_AS_NATIVE (2150, 5, swfdec_text_renderer_set_maxLevel)
void
swfdec_text_renderer_set_maxLevel (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("TextRenderer.maxLevel (static, set)");
}

SWFDEC_AS_NATIVE (2150, 10, swfdec_text_renderer_get_displayMode)
void
swfdec_text_renderer_get_displayMode (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("TextRenderer.displayMode (static, get)");
}

SWFDEC_AS_NATIVE (2150, 11, swfdec_text_renderer_set_displayMode)
void
swfdec_text_renderer_set_displayMode (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("TextRenderer.displayMode (static, set)");
}

SWFDEC_AS_NATIVE (2150, 0, swfdec_text_renderer_construct)
void
swfdec_text_renderer_construct (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("TextRenderer");
}
