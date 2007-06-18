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

#ifndef __SWFDEC_SWF_DECODER_H__
#define __SWFDEC_SWF_DECODER_H__

#include <glib.h>
#include <zlib.h>

#include <libswfdec/swfdec_decoder.h>
#include <libswfdec/swfdec_bits.h>
#include <libswfdec/swfdec_types.h>
#include <libswfdec/swfdec_rect.h>

G_BEGIN_DECLS

//typedef struct _SwfdecSwfDecoder SwfdecSwfDecoder;
typedef struct _SwfdecSwfDecoderClass SwfdecSwfDecoderClass;
typedef int (* SwfdecTagFunc) (SwfdecSwfDecoder *, guint);
typedef struct _SwfdecRootExportData SwfdecRootExportData;

typedef enum {
  SWFDEC_ROOT_ACTION_EXPORT,		/* contains a SwfdecExportData */
  SWFDEC_ROOT_ACTION_INIT_SCRIPT,	/* contains a SwfdecScript */
} SwfdecRootActionType;

typedef struct _SwfdecRootAction SwfdecRootAction;
struct _SwfdecRootAction {
  guint type;
  gpointer data;
};

struct _SwfdecRootExportData {
  char *		name;
  SwfdecCharacter *	character;
};

#define SWFDEC_TYPE_SWF_DECODER                    (swfdec_swf_decoder_get_type())
#define SWFDEC_IS_SWF_DECODER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SWF_DECODER))
#define SWFDEC_IS_SWF_DECODER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SWF_DECODER))
#define SWFDEC_SWF_DECODER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SWF_DECODER, SwfdecSwfDecoder))
#define SWFDEC_SWF_DECODER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SWF_DECODER, SwfdecSwfDecoderClass))
#define SWFDEC_SWF_DECODER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_SWF_DECODER, SwfdecSwfDecoderClass))

struct _SwfdecSwfDecoder
{
  SwfdecDecoder decoder;

  int version;

  int compressed;
  z_stream z;
  SwfdecBuffer *uncompressed_buffer;
  SwfdecBufferQueue *input_queue;

  int state;				/* where we are in the top-level state engine */
  SwfdecBits parse;			/* where we are in global parsing */
  SwfdecBits b;				/* temporary state while parsing */

  /* defined objects */
  GHashTable *characters;		/* list of all objects with an id (called characters) */
  SwfdecSprite *main_sprite;
  SwfdecSprite *parse_sprite;
  GArray **root_actions;		/* actions to be executed by the root sprite */
  GHashTable *scripts;			/* buffer -> script mapping for all scripts */

  gboolean protection;			/* TRUE is this file is protected and may not be edited */
  char *password;			/* MD5'd password to open for editing or NULL if may not be opened */

  SwfdecBuffer *jpegtables;
  char *url;
};

struct _SwfdecSwfDecoderClass {
  SwfdecDecoderClass decoder_class;
};

GType		swfdec_swf_decoder_get_type		(void);

gpointer	swfdec_swf_decoder_get_character	(SwfdecSwfDecoder *	s, 
							 guint	        	id);
gpointer	swfdec_swf_decoder_create_character	(SwfdecSwfDecoder *	s,
							 guint	        	id,
							 GType			type);

void		swfdec_swf_decoder_add_root_action	(SwfdecSwfDecoder *	s,
							 SwfdecRootActionType	type,
							 gpointer		data);
void		swfdec_swf_decoder_add_script		(SwfdecSwfDecoder *	s,
							 SwfdecScript *		script);
SwfdecScript *	swfdec_swf_decoder_get_script		(SwfdecSwfDecoder *	s,
							 guint8 *		data);

SwfdecTagFunc swfdec_swf_decoder_get_tag_func (int tag);
const char *swfdec_swf_decoder_get_tag_name (int tag);
int swfdec_swf_decoder_get_tag_flag (int tag);

G_END_DECLS

#endif
