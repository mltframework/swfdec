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

#include "swfdec_as_gcable.h"

#include "swfdec_as_context.h"

gpointer
swfdec_as_gcable_alloc (SwfdecAsContext *context, gsize size)
{
  SwfdecAsGcable *mem;
  guint8 diff;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (size > sizeof (SwfdecAsGcable), NULL);

  swfdec_as_context_use_mem (context, size);
  mem = g_slice_alloc0 (size);
  if (G_LIKELY ((GPOINTER_TO_SIZE (mem) & SWFDEC_AS_GC_FLAG_MASK) == 0))
    return mem;

  g_slice_free1 (size, mem);
  mem = g_slice_alloc0 (size + SWFDEC_AS_GC_FLAG_MASK);
  diff = GPOINTER_TO_SIZE (mem) & SWFDEC_AS_GC_FLAG_MASK;
  if (diff == 0)
    return mem;

  diff = SWFDEC_AS_GC_FLAG_MASK + 1 - diff;
  mem += diff;
  g_assert ((GPOINTER_TO_SIZE (mem) & SWFDEC_AS_GC_FLAG_MASK) == 0);
  SWFDEC_AS_GCABLE_SET_FLAG (mem, SWFDEC_AS_GC_ALIGN);
  ((guint8 *) mem)[-1] = diff;

  return mem;
}

void
swfdec_as_gcable_free (SwfdecAsContext *context, gpointer mem, gsize size)
{
  SwfdecAsGcable *gc;

  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));
  g_return_if_fail (mem != NULL);
  g_return_if_fail (size > sizeof (SwfdecAsGcable));

  gc = mem;
  swfdec_as_context_unuse_mem (context, size);
  if (G_UNLIKELY (SWFDEC_AS_GCABLE_FLAG_IS_SET (gc, SWFDEC_AS_GC_ALIGN))) {
    mem = ((guint8 *) mem) - ((guint8 *) mem)[-1];
    size += SWFDEC_AS_GC_FLAG_MASK;
  }
  g_slice_free1 (size, mem);
}

SwfdecAsGcable *
swfdec_as_gcable_collect (SwfdecAsContext *context, SwfdecAsGcable *gc,
    SwfdecAsGcableDestroyNotify notify)
{
  SwfdecAsGcable *prev, *cur, *tmp;

  g_return_val_if_fail (SWFDEC_IS_AS_CONTEXT (context), NULL);

  prev = NULL;
  cur = gc;
  for (;;) {
    while (cur) {
      if (SWFDEC_AS_GCABLE_FLAG_IS_SET (cur, SWFDEC_AS_GC_MARK | SWFDEC_AS_GC_ROOT)) {
	if (SWFDEC_AS_GCABLE_FLAG_IS_SET (cur, SWFDEC_AS_GC_MARK))
	  SWFDEC_AS_GCABLE_UNSET_FLAG (cur, SWFDEC_AS_GC_MARK);
	break;
      }
      tmp = SWFDEC_AS_GCABLE_NEXT (cur);
      notify (context, cur);
      cur = tmp;
    }
    if (prev)
      SWFDEC_AS_GCABLE_SET_NEXT (prev, cur);
    else
      gc = cur;
    prev = cur;
    if (prev == NULL)
      break;
    cur = SWFDEC_AS_GCABLE_NEXT (cur);
  }

  return gc;
}

