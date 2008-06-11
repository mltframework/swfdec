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

#ifndef HAVE_CONFIG_H
#include "config.h"
#endif

#include <swfdec_sound_matrix.h>
#include <liboil/liboil.h>
#include <swfdec_debug.h>

void
swfdec_sound_matrix_init_identity (SwfdecSoundMatrix *sound)
{
  sound->ll = sound->rr = sound->volume = 100;
  sound->lr = sound->rl = 0;
}

int
swfdec_sound_matrix_get_pan (const SwfdecSoundMatrix *sound)
{
  g_return_val_if_fail (sound != NULL, 0);

  if (sound->ll == 100)
    return ABS (sound->rr) - 100;
  else
    return 100 - ABS (sound->ll);
}

void
swfdec_sound_matrix_set_pan (SwfdecSoundMatrix *sound, int pan)
{
  g_return_if_fail (sound != NULL);

  if (pan > 0) {
    sound->rr = 100;
    sound->ll = 100 - pan;
  } else {
    sound->ll = 100;
    sound->rr = 100 + pan;
  }
}

gboolean
swfdec_sound_matrix_is_identity (const SwfdecSoundMatrix *sound)
{
  g_return_val_if_fail (sound != NULL, FALSE);

  return sound->ll == 100 && sound->rr == 100 &&
    sound->lr == 0 && sound->rr == 0 && sound->volume == 100;
}

void
swfdec_sound_matrix_apply (const SwfdecSoundMatrix *sound,
    gint16 *dest, guint n_samples)
{
  guint i;
  int left, right;

  if (swfdec_sound_matrix_is_identity (sound))
    return;
  for (i = 0; i < n_samples; i++) {
    left = (sound->ll * dest[0] + sound->lr * dest[1]) / 100;
    left *= sound->volume / 100;
    right = (sound->rl * dest[0] + sound->rr * dest[1]) / 100;
    right *= sound->volume / 100;
    dest[0] = left;
    dest[1] = right;
    dest += 2;
  }
}


void
swfdec_sound_matrix_multiply (SwfdecSoundMatrix *dest, 
    const SwfdecSoundMatrix *a, const SwfdecSoundMatrix *b)
{
  int ll, lr, rl, rr;

  g_return_if_fail (dest != NULL);
  g_return_if_fail (a != NULL);
  g_return_if_fail (b != NULL);

  ll = (b->ll * a->ll + b->lr * a->rl) / 100;
  rl = (b->rl * a->ll + b->rr * a->rl) / 100;
  lr = (b->ll * a->lr + b->lr * a->rr) / 100;
  rr = (b->rl * a->lr + b->rr * a->rr) / 100;

  dest->volume = b->volume * a->volume / 100;
  dest->ll = ll;
  dest->rl = rl;
  dest->lr = lr;
  dest->rr = rr;
}

