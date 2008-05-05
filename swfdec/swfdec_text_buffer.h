/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_TEXT_BUFFER_H_
#define _SWFDEC_TEXT_BUFFER_H_

#include <glib-object.h>
#include <swfdec/swfdec_text_attributes.h>

G_BEGIN_DECLS

typedef struct _SwfdecTextBuffer SwfdecTextBuffer;
typedef GSequenceIter SwfdecTextBufferIter;
typedef struct _SwfdecTextBufferClass SwfdecTextBufferClass;

#define SWFDEC_TYPE_TEXT_BUFFER                    (swfdec_text_buffer_get_type())
#define SWFDEC_IS_TEXT_BUFFER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXT_BUFFER))
#define SWFDEC_IS_TEXT_BUFFER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXT_BUFFER))
#define SWFDEC_TEXT_BUFFER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXT_BUFFER, SwfdecTextBuffer))
#define SWFDEC_TEXT_BUFFER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXT_BUFFER, SwfdecTextBufferClass))

struct _SwfdecTextBuffer
{
  GObject		object;

  GString *		text;		/* the text in this buffer */
  GSequence *		attributes;	/* the attributes that apply to this text */

  gsize			cursor_start;	/* byte offset into text for start of cursor */
  gsize			cursor_end;	/* if some text is selected smaller or bigger */
};

struct _SwfdecTextBufferClass
{
  GObjectClass		object_class;
};

GType			swfdec_text_buffer_get_type		(void);

SwfdecTextBuffer *	swfdec_text_buffer_new			(void);
void			swfdec_text_buffer_mark			(SwfdecTextBuffer *	buffer);

void			swfdec_text_buffer_insert_text	  	(SwfdecTextBuffer *	buffer,
								 gsize			pos,
							 	const char *		text);
#define swfdec_text_buffer_append_text(buffer, text) \
  swfdec_text_buffer_insert_text ((buffer), swfdec_text_buffer_get_length (buffer), (text));
void			swfdec_text_buffer_delete_text	  	(SwfdecTextBuffer *	buffer,
								 gsize			pos,
							 	 gsize			length);

const char *		swfdec_text_buffer_get_text		(SwfdecTextBuffer *	buffer);
gsize			swfdec_text_buffer_get_length		(SwfdecTextBuffer *	buffer);
const SwfdecTextAttributes *
			swfdec_text_buffer_get_attributes	(SwfdecTextBuffer *	buffer,
							 	 gsize			pos);
void			swfdec_text_buffer_set_attributes	(SwfdecTextBuffer *	buffer, 
								 gsize			start,
								 gsize			length,
								 const SwfdecTextAttributes *attr,
								 guint			mask);
guint			swfdec_text_buffer_get_unique		(SwfdecTextBuffer *	buffer, 
								 gsize			start,
								 gsize			length);

SwfdecTextBufferIter *	swfdec_text_buffer_get_iter		(SwfdecTextBuffer *	buffer,
								 gsize			pos);
SwfdecTextBufferIter *	swfdec_text_buffer_iter_next		(SwfdecTextBuffer *	buffer,
								 SwfdecTextBufferIter *	iter);
const SwfdecTextAttributes *
			swfdec_text_buffer_iter_get_attributes	(SwfdecTextBuffer *	buffer,
								 SwfdecTextBufferIter *	iter);
gsize			swfdec_text_buffer_iter_get_start	(SwfdecTextBuffer *	buffer,
								 SwfdecTextBufferIter *	iter);

gsize			swfdec_text_buffer_get_cursor		(SwfdecTextBuffer *	buffer);
gboolean		swfdec_text_buffer_has_selection	(SwfdecTextBuffer *	buffer);
void			swfdec_text_buffer_get_selection	(SwfdecTextBuffer *	buffer,
								 gsize *		start,
								 gsize *		end);
void			swfdec_text_buffer_set_cursor		(SwfdecTextBuffer *	buffer,
								 gsize			start,
								 gsize			end);


G_END_DECLS
#endif
