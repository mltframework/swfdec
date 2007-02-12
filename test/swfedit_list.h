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

#ifndef __SWFEDIT_LIST_H__
#define __SWFEDIT_LIST_H__

#include "swfdec_out.h"
#include "swfedit_tag.h"

G_BEGIN_DECLS

typedef struct _SwfeditList SwfeditList;
typedef struct _SwfeditListClass SwfeditListClass;

#define SWFEDIT_TYPE_LIST                    (swfedit_list_get_type())
#define SWFEDIT_IS_LIST(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFEDIT_TYPE_LIST))
#define SWFEDIT_IS_LIST_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFEDIT_TYPE_LIST))
#define SWFEDIT_LIST(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFEDIT_TYPE_LIST, SwfeditList))
#define SWFEDIT_LIST_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFEDIT_TYPE_LIST, SwfeditListClass))
#define SWFEDIT_LIST_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFEDIT_TYPE_LIST, SwfeditListClass))

struct _SwfeditList {
  SwfeditToken			token;

  const SwfeditTagDefinition *	def;	/* definition of our items */
  guint				n_defs;	/* number of items in def */
};

struct _SwfeditListClass {
  SwfeditTokenClass	token_class;
};

GType		swfedit_list_get_type	(void);

SwfeditList *	swfedit_list_new	(const SwfeditTagDefinition *	def);
SwfeditList *	swfedit_list_new_read	(SwfdecBits *			bits,
					 const SwfeditTagDefinition *	def);

SwfdecBuffer *	swfedit_list_write	(SwfeditList *			list);


G_END_DECLS

#endif
