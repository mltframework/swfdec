/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		 2006-2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_SHAPE_PARSER_H_
#define _SWFDEC_SHAPE_PARSER_H_

#include <swfdec/swfdec_bits.h>
#include <swfdec/swfdec_draw.h>

G_BEGIN_DECLS


typedef struct _SwfdecShapeParser SwfdecShapeParser;
typedef SwfdecDraw * (* SwfdecParseDrawFunc) (SwfdecBits *bits, gpointer *data);

SwfdecShapeParser *	swfdec_shape_parser_new		(SwfdecParseDrawFunc	parse_fill,
							 SwfdecParseDrawFunc	parse_line,
							 gpointer		data);
GSList *		swfdec_shape_parser_reset	(SwfdecShapeParser *	parser);
GSList *		swfdec_shape_parser_free	(SwfdecShapeParser *	parser);

void			swfdec_shape_parser_parse	(SwfdecShapeParser *	parser,
							 SwfdecBits *		bits);
void			swfdec_shape_parser_parse_morph	(SwfdecShapeParser *	parser,
							 SwfdecBits *		bits1,
							 SwfdecBits *		bits2);


G_END_DECLS
#endif
