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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zlib.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <liboil/liboil.h>

#include "swfdec.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_edittext.h"
#include "swfdec_js.h"
#include "swfdec_sound.h"
#include "swfdec_sprite.h"

enum {
  SWFDEC_STATE_INIT1 = 0,
  SWFDEC_STATE_INIT2,
  SWFDEC_STATE_PARSETAG,
  SWFDEC_STATE_EOF,
};

G_DEFINE_TYPE (SwfdecSwfDecoder, swfdec_swf_decoder, SWFDEC_TYPE_DECODER)

static void
swfdec_decoder_dispose (GObject *object)
{
  SwfdecSwfDecoder *s = SWFDEC_SWF_DECODER (object);

  g_list_foreach (s->characters, (GFunc) g_object_unref, NULL);
  g_list_free (s->characters);
  g_object_unref (s->main_sprite);
  g_hash_table_destroy (s->exports);

  if (s->z) {
    inflateEnd (s->z);
    g_free (s->z);
    swfdec_buffer_unref (s->uncompressed_buffer);
  }
  swfdec_buffer_queue_free (s->input_queue);

  if (s->jpegtables) {
    swfdec_buffer_unref (s->jpegtables);
  }

  g_free (s->password);

  G_OBJECT_CLASS (swfdec_swf_decoder_parent_class)->dispose (object);
}

static void *
zalloc (void *opaque, unsigned int items, unsigned int size)
{
  return g_malloc (items * size);
}

static void
zfree (void *opaque, void *addr)
{
  g_free (addr);
}

#if 0
static void
dumpbits (SwfdecBits * b)
{
  int i;

  printf ("    ");
  for (i = 0; i < 16; i++) {
    printf ("%02x ", swfdec_bits_get_u8 (b));
  }
  printf ("\n");
}
#endif

static gboolean
swfdec_swf_decoder_deflate_all (SwfdecSwfDecoder * s)
{
  int ret;
  SwfdecBuffer *buffer;
  SwfdecDecoder *dec = SWFDEC_DECODER (s);
  
  while ((buffer = swfdec_buffer_queue_pull_buffer (SWFDEC_DECODER (s)->queue))) {
    if (s->compressed) {
      int offset = s->z->total_out;

      s->z->next_in = buffer->data;
      s->z->avail_in = buffer->length;
      ret = inflate (s->z, Z_SYNC_FLUSH);
      if (ret < 0) {
	SWFDEC_ERROR ("error uncompressing data");
	return FALSE;
      }

      swfdec_buffer_unref (buffer);

      buffer = swfdec_buffer_new_subbuffer (s->uncompressed_buffer, offset,
	  s->z->total_out - offset);
    } 
    dec->bytes_loaded += buffer->length;
    swfdec_buffer_queue_push (s->input_queue, buffer);
  }

  return TRUE;
}

static void
swf_inflate_init (SwfdecSwfDecoder * s)
{
  SwfdecDecoder *dec = SWFDEC_DECODER (s);
  z_stream *z;
  int ret;

  z = g_new0 (z_stream, 1);
  s->z = z;
  z->zalloc = zalloc;
  z->zfree = zfree;
  ret = inflateInit (z);
  SWFDEC_DEBUG ("inflateInit returned %d", ret);

  s->uncompressed_buffer = swfdec_buffer_new_and_alloc (dec->bytes_total);
  z->next_out = s->uncompressed_buffer->data;
  z->avail_out = s->uncompressed_buffer->length;
  z->opaque = NULL;
}

static int
swf_parse_header1 (SwfdecSwfDecoder * s)
{
  SwfdecDecoder *dec = SWFDEC_DECODER (s);
  int sig1, sig2, sig3;
  SwfdecBuffer *buffer;

  /* NB: we're still reading from the original queue, since deflating is not initialized yet */
  buffer = swfdec_buffer_queue_pull (dec->queue, 8);
  if (buffer == NULL) {
    return SWFDEC_STATUS_NEEDBITS;
  }

  s->b.buffer = buffer;
  s->b.ptr = buffer->data;
  s->b.idx = 0;
  s->b.end = buffer->data + buffer->length;

  sig1 = swfdec_bits_get_u8 (&s->b);
  sig2 = swfdec_bits_get_u8 (&s->b);
  sig3 = swfdec_bits_get_u8 (&s->b);

  s->version = swfdec_bits_get_u8 (&s->b);
  dec->bytes_total = swfdec_bits_get_u32 (&s->b);

  swfdec_buffer_unref (buffer);
  SWFDEC_DECODER (s)->bytes_loaded = 8;

  if ((sig1 != 'F' && sig1 != 'C') || sig2 != 'W' || sig3 != 'S') {
    return SWFDEC_STATUS_ERROR;
  }

  s->compressed = (sig1 == 'C');
  if (s->compressed) {
    SWFDEC_DEBUG ("compressed");
    swf_inflate_init (s);
  } else {
    SWFDEC_DEBUG ("not compressed");
  }
  s->state = SWFDEC_STATE_INIT2;

  return SWFDEC_STATUS_OK;
}

static int
swf_parse_header2 (SwfdecSwfDecoder * s)
{
  int n;
  SwfdecBuffer *buffer;
  SwfdecRect rect;
  SwfdecDecoder *dec = SWFDEC_DECODER (s);

  buffer = swfdec_buffer_queue_peek (s->input_queue, 32);
  if (buffer == NULL) {
    return SWFDEC_STATUS_NEEDBITS;
  }

  s->b.buffer = buffer;
  s->b.ptr = buffer->data;
  s->b.idx = 0;
  s->b.end = buffer->data + buffer->length;

  swfdec_bits_get_rect (&s->b, &rect);
  if (rect.x0 != 0.0 || rect.y0 != 0.0)
    SWFDEC_ERROR ("SWF window doesn't start at 0 0 but at %g %g\n", rect.x0, rect.y0);
  dec->width = ceil (rect.x1 / SWFDEC_TWIPS_SCALE_FACTOR);
  dec->height = ceil (rect.y1 / SWFDEC_TWIPS_SCALE_FACTOR);
  swfdec_bits_syncbits (&s->b);
  dec->rate = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG ("rate = %g", dec->rate / 256.0);
  dec->frames_total = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG ("n_frames = %d", dec->frames_total);

  n = s->b.ptr - s->b.buffer->data;
  swfdec_buffer_unref (buffer);

  buffer = swfdec_buffer_queue_pull (s->input_queue, n);
  swfdec_buffer_unref (buffer);

  swfdec_sprite_set_n_frames (s->main_sprite, dec->frames_total, dec->rate);

  s->state = SWFDEC_STATE_PARSETAG;
  return SWFDEC_STATUS_CHANGE;
}

static SwfdecStatus
swfdec_swf_decoder_parse (SwfdecDecoder *dec)
{
  SwfdecSwfDecoder *s = SWFDEC_SWF_DECODER (dec);
  int ret = SWFDEC_STATUS_OK;
  unsigned char *endptr;
  SwfdecBuffer *buffer;

  s->b = s->parse;
  g_assert (dec->player);
  s->main_sprite->player = dec->player;

  switch (s->state) {
    case SWFDEC_STATE_INIT1:
      ret = swf_parse_header1 (s);
      break;
    case SWFDEC_STATE_INIT2:
      if (!swfdec_swf_decoder_deflate_all (s))
	return SWFDEC_STATUS_ERROR;
      ret = swf_parse_header2 (s);
      break;
    case SWFDEC_STATE_PARSETAG:
    {
      int header_length;
      int x;
      SwfdecTagFunc *func;
      int tag;
      int tag_len;

      if (!swfdec_swf_decoder_deflate_all (s))
	return SWFDEC_STATUS_ERROR;

      /* we're parsing tags */
      buffer = swfdec_buffer_queue_peek (s->input_queue, 2);
      if (buffer == NULL) {
	return SWFDEC_STATUS_NEEDBITS;
      }

      s->b.buffer = buffer;
      s->b.ptr = buffer->data;
      s->b.idx = 0;
      s->b.end = buffer->data + buffer->length;


      x = swfdec_bits_get_u16 (&s->b);
      tag = (x >> 6) & 0x3ff;
      SWFDEC_DEBUG ("tag %d %s", tag, swfdec_swf_decoder_get_tag_name (tag));
      tag_len = x & 0x3f;
      if (tag_len == 0x3f) {
	swfdec_buffer_unref (buffer);
	buffer = swfdec_buffer_queue_peek (s->input_queue, 6);
	if (buffer == NULL) {
	  return SWFDEC_STATUS_NEEDBITS;
	}
	s->b.buffer = buffer;
	s->b.ptr = buffer->data;
	s->b.idx = 0;
	s->b.end = buffer->data + buffer->length;

	swfdec_bits_get_u16 (&s->b);
	tag_len = swfdec_bits_get_u32 (&s->b);
	header_length = 6;
      } else {
	header_length = 2;
      }
      swfdec_buffer_unref (buffer);

      SWFDEC_INFO ("parsing at %d, tag %d %s, length %d",
	  swfdec_buffer_queue_get_offset (s->input_queue), tag,
	  swfdec_swf_decoder_get_tag_name (tag), tag_len);

      if (swfdec_buffer_queue_get_depth (s->input_queue) < tag_len +
	  header_length) {
	return SWFDEC_STATUS_NEEDBITS;
      }

      buffer = swfdec_buffer_queue_pull (s->input_queue, header_length);
      swfdec_buffer_unref (buffer);

      if (tag_len > 0) {
	buffer = swfdec_buffer_queue_pull (s->input_queue, tag_len);
	s->b.buffer = buffer;
	s->b.ptr = buffer->data;
	s->b.idx = 0;
	s->b.end = buffer->data + buffer->length;
	endptr = s->b.ptr + tag_len;
      } else {
	buffer = NULL;
	s->b.buffer = NULL;
	s->b.ptr = NULL;
	s->b.idx = 0;
	s->b.end = NULL;
	endptr = NULL;
      }
      func = swfdec_swf_decoder_get_tag_func (tag);
      if (func == NULL) {
	SWFDEC_WARNING ("tag function not implemented for %d %s",
	    tag, swfdec_swf_decoder_get_tag_name (tag));
      } else {
	s->parse_sprite = s->main_sprite;
	ret = func (s);
	s->parse_sprite = NULL;

	swfdec_bits_syncbits (&s->b);
	if (s->b.ptr < endptr) {
	  SWFDEC_WARNING
	      ("early finish (%d bytes) at %d, tag %d %s, length %d",
	      endptr - s->b.ptr,
	      swfdec_buffer_queue_get_offset (s->input_queue), tag,
	      swfdec_swf_decoder_get_tag_name (tag), tag_len);
	  //dumpbits (&s->b);
	}
	if (s->b.ptr > endptr) {
	  SWFDEC_WARNING
	      ("parse_overrun (%d bytes) at %d, tag %d %s, length %d",
	      s->b.ptr - endptr,
	      swfdec_buffer_queue_get_offset (s->input_queue), tag,
	      swfdec_swf_decoder_get_tag_name (tag), tag_len);
	}
      }

      if (tag == 0) {
	s->state = SWFDEC_STATE_EOF;
      }

      if (buffer)
	swfdec_buffer_unref (buffer);

    }
      break;
    case SWFDEC_STATE_EOF:
      return SWFDEC_STATUS_EOF;
  }

  /* copy state */
  dec->frames_loaded = s->main_sprite->parse_frame;

  return ret;
}

static void
swfdec_swf_decoder_class_init (SwfdecSwfDecoderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  SwfdecDecoderClass *decoder_class = SWFDEC_DECODER_CLASS (class);

  object_class->dispose = swfdec_decoder_dispose;

  decoder_class->parse = swfdec_swf_decoder_parse;
}

static void
swfdec_swf_decoder_init (SwfdecSwfDecoder *s)
{
  s->main_sprite = g_object_new (SWFDEC_TYPE_SPRITE, NULL);

  s->input_queue = swfdec_buffer_queue_new ();
  s->exports = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
}

gpointer
swfdec_swf_decoder_get_export (SwfdecSwfDecoder * s, const char *name)
{
  gpointer result;

  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (s), NULL);
  g_return_val_if_fail (name != NULL, NULL);

  result = g_hash_table_lookup (s->exports, name);
  return result;
}

gpointer
swfdec_swf_decoder_get_character (SwfdecSwfDecoder * s, int id)
{
  SwfdecCharacter *character;
  GList *g;

  g_return_val_if_fail (SWFDEC_IS_SWF_DECODER (s), NULL);

  for (g = s->characters; g; g = g_list_next (g)) {
    character = SWFDEC_CHARACTER (g->data);
    if (character->id == id)
      return character;
  }

  return NULL;
}

/**
 * swfdec_swf_decoder_create_character:
 * @s: a #SwfdecDecoder
 * @id: id of the character
 * @type: the required type for the character
 *
 * Gets the character of the requested @type and with the given @id from @s.
 * If there is already a different character with the given id, return NULL.
 * If the character doesn't exist yet, create it.
 *
 * Returns: The requested character or NULL on failure;
 **/
gpointer
swfdec_swf_decoder_create_character (SwfdecSwfDecoder * s, int id, GType type)
{
  SwfdecCharacter *result;

  g_return_val_if_fail (SWFDEC_IS_DECODER (s), NULL);
  g_return_val_if_fail (id >= 0, NULL);
  g_return_val_if_fail (g_type_is_a (type, SWFDEC_TYPE_CHARACTER), NULL);

  SWFDEC_INFO ("  id = %d", id);
  result = swfdec_swf_decoder_get_character (s, id);
  if (result) {
    SWFDEC_WARNING ("character with id %d already exists", id);
    return NULL;
  }
  result = g_object_new (type, NULL);
  result->id = id;
  s->characters = g_list_prepend (s->characters, result);
  if (SWFDEC_IS_SPRITE (result)) {
    g_assert (SWFDEC_DECODER (s)->player);
    SWFDEC_SPRITE (result)->player = SWFDEC_DECODER (s)->player;
  }

  return result;
}
