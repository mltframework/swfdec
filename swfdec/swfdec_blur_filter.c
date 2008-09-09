/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#include "swfdec_blur_filter.h"
#include "swfdec_debug.h"

G_DEFINE_TYPE (SwfdecBlurFilter, swfdec_blur_filter, SWFDEC_TYPE_FILTER)

static void
swfdec_blur_filter_class_init (SwfdecBlurFilterClass *klass)
{
}

static void
swfdec_blur_filter_init (SwfdecBlurFilter *filter)
{
  filter->x = 4;
  filter->y = 4;
  filter->quality = 1;
}

void
swfdec_blur_filter_invalidate (SwfdecBlurFilter *filter)
{
  g_return_if_fail (SWFDEC_IS_BLUR_FILTER (filter));
}

