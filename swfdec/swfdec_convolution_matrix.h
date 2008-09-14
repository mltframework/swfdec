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

#ifndef _SWFDEC_CONVOLUTION_MATRIX_H_
#define _SWFDEC_CONVOLUTION_MATRIX_H_

#include <glib.h>

G_BEGIN_DECLS


typedef struct _SwfdecConvolutionMatrix SwfdecConvolutionMatrix;

SwfdecConvolutionMatrix *
		swfdec_convolution_matrix_new		(guint			  	width, 
							 guint				height);
void		swfdec_convolution_matrix_free		(SwfdecConvolutionMatrix *	matrix);

void		swfdec_convolution_matrix_set		(SwfdecConvolutionMatrix *	matrix,
							 guint				x,
							 guint				y,
							 double				value);
double		swfdec_convolution_matrix_get		(SwfdecConvolutionMatrix *	matrix,
							 guint				x,
							 guint				y);
guint		swfdec_convolution_matrix_get_width	(SwfdecConvolutionMatrix *	matrix);
guint		swfdec_convolution_matrix_get_height	(SwfdecConvolutionMatrix *	matrix);
void		swfdec_convolution_matrix_apply		(SwfdecConvolutionMatrix *	matrix,
							 guint				width,
							 guint				height, 
							 guint8 *			tdata,
							 guint				tstride,
							 const guint8 *			sdata,
							 guint				sstride);


G_END_DECLS
#endif
