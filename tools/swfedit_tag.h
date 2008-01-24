/* Swfedit
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

#ifndef __SWFEDIT_TAG_H__
#define __SWFEDIT_TAG_H__

#include <swfdec/swfdec_buffer.h>
#include <swfdec/swfdec_bits.h>
#include "swfdec_out.h"
#include "swfedit_token.h"

G_BEGIN_DECLS

typedef struct _SwfeditTag SwfeditTag;
typedef struct _SwfeditTagClass SwfeditTagClass;
typedef struct _SwfeditTagDefinition SwfeditTagDefinition;

struct _SwfeditTagDefinition {
  const char *	  	name;			/* name to use for this field */
  SwfeditTokenType     	type;			/* type of this field */
  guint			n_items;		/* 1-indexed field to look at for item count (or 0 to use 1 item) */
  gconstpointer		hint;			/* hint to pass to field when creating */
};

#define SWFEDIT_TYPE_TAG                    (swfedit_tag_get_type())
#define SWFEDIT_IS_TAG(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFEDIT_TYPE_TAG))
#define SWFEDIT_IS_TAG_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFEDIT_TYPE_TAG))
#define SWFEDIT_TAG(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFEDIT_TYPE_TAG, SwfeditTag))
#define SWFEDIT_TAG_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFEDIT_TYPE_TAG, SwfeditTagClass))
#define SWFEDIT_TAG_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFEDIT_TYPE_TAG, SwfeditTagClass))

struct _SwfeditTag {
  SwfeditToken		token;

  guint			tag;		/* tag type */
};

struct _SwfeditTagClass {
  SwfeditTokenClass	token_class;
};

GType		swfedit_tag_get_type	(void);

SwfeditTag *	swfedit_tag_new		(SwfeditToken *		parent,
					 guint			tag,
					 SwfdecBuffer *		buffer);

SwfdecBuffer *	swfedit_tag_write	(SwfeditToken *		token);
void		swfedit_tag_write_token	(SwfeditToken *		token,
					 SwfdecOut *		out,
					 guint			i);
void		swfedit_tag_add_token	(SwfeditToken *		token,
					 const char *		name,
					 SwfeditTokenType	type,
					 gconstpointer		hint);
void		swfedit_tag_read_token	(SwfeditToken *		token, 
					 SwfdecBits *		bits,
					 const char *		name,
					 SwfeditTokenType	type,
					 gconstpointer		hint);
void		swfedit_tag_read_tag	(SwfeditToken *		token,
					 SwfdecBits *		bits, 
					 const SwfeditTagDefinition *def);

G_END_DECLS

#endif
