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

#ifndef __SWFDEC_OUT_H__
#define __SWFDEC_OUT_H__

#include <swfdec/swfdec_buffer.h>
#include <swfdec/swfdec_color.h>
#include <swfdec/swfdec_rect.h>

G_BEGIN_DECLS


typedef struct _SwfdecBots SwfdecBots;

struct _SwfdecBots {
  unsigned char *	data;
  unsigned char *	ptr;
  unsigned int		idx;
  unsigned char *	end;
};

#define SWFDEC_OUT_INITIAL (32)
#define SWFDEC_OUT_STEP (32)

SwfdecBots *	swfdec_bots_open		(void);
SwfdecBuffer *	swfdec_bots_close		(SwfdecBots *		bots);
void		swfdec_bots_free		(SwfdecBots *		bots);

gsize		swfdec_bots_get_bits		(SwfdecBots *		bots);
gsize		swfdec_bots_get_bytes		(SwfdecBots *		bots);
gsize		swfdec_bots_left		(SwfdecBots *		bots);
void		swfdec_bots_ensure_bits		(SwfdecBots *		bots,
						 unsigned int		bits);
void		swfdec_bots_prepare_bytes	(SwfdecBots *		bots,
						 unsigned int		bytes);

void		swfdec_bots_put_bit		(SwfdecBots *		bots,
						 gboolean		bit);
void		swfdec_bots_put_bits		(SwfdecBots *		bots,
						 guint	  		bits,
						 guint			n_bits);
void		swfdec_bots_put_sbits		(SwfdecBots *		bots,
						 int	  		bits,
						 guint	  		n_bits);
void		swfdec_bots_put_data		(SwfdecBots *		bots,
						 const guint8 *		data,
						 guint			length);
void		swfdec_bots_put_buffer		(SwfdecBots *		bots,
						 SwfdecBuffer *		buffer);
void		swfdec_bots_put_bots		(SwfdecBots *		bots,
						 SwfdecBots *		other);
void		swfdec_bots_put_u8		(SwfdecBots *		bots,
						 guint	  		i);
void		swfdec_bots_put_u16		(SwfdecBots *		bots,
						 guint			i);
void		swfdec_bots_put_s16		(SwfdecBots *		bots,
						 gint			i);
void		swfdec_bots_put_u32		(SwfdecBots *		bots,
						 guint			i);
void		swfdec_bots_put_string		(SwfdecBots *		bots,
						 const char *		s);
void		swfdec_bots_put_double		(SwfdecBots *		bots,
						 double			value);
void		swfdec_bots_put_float		(SwfdecBots *		bots,
						 float			f);

void		swfdec_bots_put_rgb		(SwfdecBots *		bots,
						 SwfdecColor		color);
void		swfdec_bots_put_rgba		(SwfdecBots *		bots,
						 SwfdecColor		color);
void		swfdec_bots_put_rect		(SwfdecBots *		bots,
						 const SwfdecRect *	rect);
void		swfdec_bots_put_matrix		(SwfdecBots *		bots,
						 const cairo_matrix_t *	matrix);
void		swfdec_bots_put_color_transform	(SwfdecBots *		bots,
						 const SwfdecColorTransform *trans);


G_END_DECLS

#endif
