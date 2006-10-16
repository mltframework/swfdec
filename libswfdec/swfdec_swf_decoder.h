/* Swfdec
 * Copyright (C) 2003-2006 David Schleef <ds@schleef.org>
 *		 2005-2006 Eric Anholt <eric@anholt.net>
 *		      2006 Benjamin Otte <otte@gnome.org>
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


//typedef struct _SwfdecSwfDecoder SwfdecSwfDecoder;
typedef struct _SwfdecSwfDecoderClass SwfdecSwfDecoderClass;
typedef int SwfdecTagFunc (SwfdecSwfDecoder *);

#define SWFDEC_TYPE_SWF_DECODER                    (swfdec_swf_decoder_get_type())
#define SWFDEC_IS_DECODER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_DECODER))
#define SWFDEC_IS_DECODER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_DECODER))
#define SWFDEC_SWF_DECODER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_DECODER, SwfdecSwfDecoder))
#define SWFDEC_SWF_DECODER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_DECODER, SwfdecSwfDecoderClass))
#define SWFDEC_SWF_DECODER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_DECODER, SwfdecSwfDecoderClass))

struct _SwfdecSwfDecoder
{
  SwfdecDecoder decoder;

  int version;
  int width, height;
  unsigned int rate;			/* divide by 256 to get iterations per second */
  unsigned int n_frames;


  /* End of legacy elements */

  int compressed;
  z_stream *z;
  SwfdecBuffer *uncompressed_buffer;
  SwfdecBufferQueue *input_queue;

  int state;				/* where we are in the top-level state engine */
  SwfdecBits parse;			/* where we are in global parsing */
  SwfdecBits b;				/* temporary state while parsing */

  /* defined objects */
  GList *characters;			/* list of all objects with an id (called characters) */

  SwfdecSprite *main_sprite;
  SwfdecSprite *parse_sprite;

  gboolean protection;			/* TRUE is this file is protected and may not be edited */
  char *password;			/* MD5'd password to open for editing or NULL if may not be opened */


  SwfdecBuffer *jpegtables;

  char *url;

  /* global state */
  GList *movies;			/* list of all running movie clips */
  GQueue *gotos;			/* gotoAndFoo + iterations */
};

struct _SwfdecSwfDecoderClass {
  SwfdecDecoderClass decoder_class;
};

GType		swfdec_swf_decoder_get_type		();

gpointer	swfdec_swf_decoder_get_character	(SwfdecSwfDecoder *	s, 
							 int			id);
gpointer	swfdec_swf_decoder_create_character	(SwfdecSwfDecoder *	s,
							 int			id,
							 GType			type);

SwfdecSwfDecoder *swf_init (void);
SwfdecSwfDecoder *swfdec_swf_decoder_new (void);

int swf_addbits (SwfdecSwfDecoder * s, unsigned char *bits, int len);
int swf_parse (SwfdecSwfDecoder * s);
int swf_parse_header (SwfdecSwfDecoder * s);
int swf_parse_tag (SwfdecSwfDecoder * s);
int tag_func_ignore (SwfdecSwfDecoder * s);

SwfdecTagFunc *swfdec_swf_decoder_get_tag_func (int tag);
const char *swfdec_swf_decoder_get_tag_name (int tag);
int swfdec_swf_decoder_get_tag_flag (int tag);

#endif
