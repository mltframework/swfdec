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

#ifndef __SWFDEC_AMF_H__
#define __SWFDEC_AMF_H__

#include <swfdec/swfdec_as_context.h>
#include <swfdec/swfdec_bits.h>

typedef enum {
  SWFDEC_AMF_NUMBER = 0,
  SWFDEC_AMF_BOOLEAN = 1,
  SWFDEC_AMF_STRING = 2,
  SWFDEC_AMF_OBJECT = 3,
  SWFDEC_AMF_MOVIECLIP = 4,
  SWFDEC_AMF_NULL = 5,
  SWFDEC_AMF_UNDEFINED = 6,
  SWFDEC_AMF_REFERENCE = 7,
  SWFDEC_AMF_MIXED_ARRAY = 8,
  SWFDEC_AMF_END_OBJECT = 9,
  SWFDEC_AMF_ARRAY = 10,
  SWFDEC_AMF_DATE = 11,
  SWFDEC_AMF_BIG_STRING = 12,
  /* what is 13? */
  SWFDEC_AMF_RECORDSET = 14,
  SWFDEC_AMF_XML = 15,
  SWFDEC_AMF_CLASS = 16,
  SWFDEC_AMF_FLASH9 = 17,
  /* add more items here */
  SWFDEC_AMF_N_TYPES
} SwfdecAmfType;

gboolean	swfdec_amf_parse_one		(SwfdecAsContext *	context, 
						 SwfdecBits *		bits,
						 SwfdecAmfType		expected_type,
						 SwfdecAsValue *	rval);
guint		swfdec_amf_parse		(SwfdecAsContext *	context, 
						 SwfdecBits *		bits,
						 guint			n_items,
						 ...);


#endif
