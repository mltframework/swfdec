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

#ifndef __SWFDEC_SOUND_MATRIX_H__
#define __SWFDEC_SOUND_MATRIX_H__

#include <glib.h>

G_BEGIN_DECLS

typedef struct _SwfdecSoundMatrix SwfdecSoundMatrix;

struct _SwfdecSoundMatrix
{
  int			ll;		/* percentage of left channel */
  int			rl;		/* "percentage" of left channel on right speaker */
  int			lr;		/* "percentage" of right channel on left speaker */
  int			rr;		/* percentage of right channel */
  int			volume;		/* don't ask me why volume is seperate */
};

void		swfdec_sound_matrix_init_identity	(SwfdecSoundMatrix *		sound);

gboolean	swfdec_sound_matrix_is_identity		(const SwfdecSoundMatrix *	sound);
gboolean	swfdec_sound_matrix_is_equal		(const SwfdecSoundMatrix *	a,
							 const SwfdecSoundMatrix *	b);

int		swfdec_sound_matrix_get_pan		(const SwfdecSoundMatrix *	sound);
void		swfdec_sound_matrix_set_pan		(SwfdecSoundMatrix *		sound,
							 int				pan);

void		swfdec_sound_matrix_apply		(const SwfdecSoundMatrix *	sound,
							 gint16 *			dest,
							 guint				n_samples);

void		swfdec_sound_matrix_multiply		(SwfdecSoundMatrix *		dest,
							 const SwfdecSoundMatrix *	a,
							 const SwfdecSoundMatrix *	b);


G_END_DECLS
#endif

