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


typedef struct _SwfdecOut SwfdecOut;

struct _SwfdecOut {
  unsigned char *	data;
  unsigned char *	ptr;
  unsigned int		idx;
  unsigned char *	end;
};

#define SWFDEC_OUT_INITIAL (32)
#define SWFDEC_OUT_STEP (32)

SwfdecOut *	swfdec_out_open			(void);
SwfdecBuffer *	swfdec_out_close		(SwfdecOut *		out);

unsigned int	swfdec_out_get_bits		(SwfdecOut *		out);
unsigned int	swfdec_out_left	  		(SwfdecOut *		out);
void		swfdec_out_ensure_bits		(SwfdecOut *		out,
						 unsigned int		bits);
void		swfdec_out_prepare_bytes	(SwfdecOut *		out,
						 unsigned int		bytes);

void		swfdec_out_put_bit		(SwfdecOut *		out,
						 gboolean		bit);
void		swfdec_out_put_bits		(SwfdecOut *		out,
						 guint	  		bits,
						 guint			n_bits);
void		swfdec_out_put_sbits		(SwfdecOut *		out,
						 int	  		bits,
						 guint	  		n_bits);
void		swfdec_out_put_data		(SwfdecOut *		out,
						 const guint8 *		data,
						 guint			length);
void		swfdec_out_put_buffer		(SwfdecOut *		out,
						 SwfdecBuffer *		buffer);
void		swfdec_out_put_u8		(SwfdecOut *		out,
						 guint	  		i);
void		swfdec_out_put_u16		(SwfdecOut *		out,
						 guint			i);
void		swfdec_out_put_u32		(SwfdecOut *		out,
						 guint			i);
void		swfdec_out_put_string		(SwfdecOut *		out,
						 const char *		s);

void		swfdec_out_put_rgb		(SwfdecOut *		out,
						 SwfdecColor		color);
void		swfdec_out_put_rgba		(SwfdecOut *		out,
						 SwfdecColor		color);
void		swfdec_out_put_rect		(SwfdecOut *		out,
						 const SwfdecRect *	rect);
void		swfdec_out_put_matrix		(SwfdecOut *		out,
						 const cairo_matrix_t *	matrix);
void		swfdec_out_put_color_transform	(SwfdecOut *		out,
						 const SwfdecColorTransform *trans);


G_END_DECLS

#endif
