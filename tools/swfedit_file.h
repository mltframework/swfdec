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

#ifndef __SWFEDIT_FILE_H__
#define __SWFEDIT_FILE_H__

#include <swfdec/swfdec_rect.h>
#include "swfedit_token.h"

G_BEGIN_DECLS

typedef struct _SwfeditFile SwfeditFile;
typedef struct _SwfeditFileClass SwfeditFileClass;

#define SWFEDIT_TYPE_FILE                    (swfedit_file_get_type())
#define SWFEDIT_IS_FILE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFEDIT_TYPE_FILE))
#define SWFEDIT_IS_FILE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFEDIT_TYPE_FILE))
#define SWFEDIT_FILE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFEDIT_TYPE_FILE, SwfeditFile))
#define SWFEDIT_FILE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFEDIT_TYPE_FILE, SwfeditFileClass))
#define SWFEDIT_FILE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFEDIT_TYPE_FILE, SwfeditFileClass))

struct _SwfeditFile {
  SwfeditToken		token;

  char *		filename;	/* name this file is saved to */
};

struct _SwfeditFileClass {
  SwfeditTokenClass	token_class;
};

GType		swfedit_file_get_type		(void);

SwfeditFile *	swfedit_file_new		(const char *	filename,
						 GError **	error);

gboolean	swfedit_file_save		(SwfeditFile *	file,
						 GError **	error);
guint		swfedit_file_get_version	(SwfeditFile *	file);


G_END_DECLS

#endif
