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

#ifndef __SWFEDIT_TOKEN_H__
#define __SWFEDIT_TOKEN_H__

#include <gtk/gtk.h>
#include <swfdec/swfdec_rect.h>

G_BEGIN_DECLS

typedef enum {
  SWFEDIT_TOKEN_OBJECT,
  SWFEDIT_TOKEN_BINARY,
  SWFEDIT_TOKEN_BIT,
  SWFEDIT_TOKEN_UINT8,
  SWFEDIT_TOKEN_UINT16,
  SWFEDIT_TOKEN_UINT32,
  SWFEDIT_TOKEN_STRING,
  SWFEDIT_TOKEN_RECT,
  SWFEDIT_TOKEN_RGB,
  SWFEDIT_TOKEN_RGBA,
  SWFEDIT_TOKEN_MATRIX,
  SWFEDIT_TOKEN_CTRANS,
  SWFEDIT_TOKEN_SCRIPT,
  SWFEDIT_TOKEN_CLIPEVENTFLAGS,
  SWFEDIT_N_TOKENS
} SwfeditTokenType;

typedef enum {
  SWFEDIT_COLUMN_NAME,
  SWFEDIT_COLUMN_VALUE_VISIBLE,
  SWFEDIT_COLUMN_VALUE,
  SWFEDIT_COLUMN_VALUE_EDITABLE
} SwfeditColumn;

typedef struct _SwfeditTokenEntry SwfeditTokenEntry;

typedef struct _SwfeditToken SwfeditToken;
typedef struct _SwfeditTokenClass SwfeditTokenClass;

#define SWFEDIT_TYPE_TOKEN                    (swfedit_token_get_type())
#define SWFEDIT_IS_TOKEN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFEDIT_TYPE_TOKEN))
#define SWFEDIT_IS_TOKEN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFEDIT_TYPE_TOKEN))
#define SWFEDIT_TOKEN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFEDIT_TYPE_TOKEN, SwfeditToken))
#define SWFEDIT_TOKEN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFEDIT_TYPE_TOKEN, SwfeditTokenClass))
#define SWFEDIT_TOKEN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFEDIT_TYPE_TOKEN, SwfeditTokenClass))

struct _SwfeditTokenEntry {
  char *		name;
  SwfeditTokenType	type;
  gpointer		value;
  gboolean		visible;
};

struct _SwfeditToken {
  GObject		object;

  SwfeditToken *	parent;		/* parent of this token or NULL */
  gchar *		name;		/* name of token */
  GArray *		tokens;		/* of SwfeditTokenEntry */
};

struct _SwfeditTokenClass {
  GObjectClass		object_class;

  void			(* changed)		(SwfeditToken *		token,
						 guint			id);
};

GType		swfedit_token_get_type		(void);

gpointer	swfedit_token_new_token		(SwfeditTokenType	type);

SwfeditToken *	swfedit_token_new		(void);
void		swfedit_token_add		(SwfeditToken *		token,
						 const char *		name,
						 SwfeditTokenType	type,
						 gpointer		value);
void		swfedit_token_set		(SwfeditToken *		token,
						 guint			i,
						 gpointer		value);
void		swfedit_token_set_iter		(SwfeditToken *		token,
						 GtkTreeIter *		iter,
						 const char *		value);
void		swfedit_token_set_visible	(SwfeditToken *		token,
						 guint			i,
						 gboolean		visible);


G_END_DECLS

#endif
