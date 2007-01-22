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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zlib.h>

#include "libswfdec/swfdec_bits.h"
#include "libswfdec/swfdec_buffer.h"
#include "libswfdec/swfdec_debug.h"
#include "libswfdec/swfdec_swf_decoder.h"
#include "swfedit_file.h"
#include "swfedit_tag.h"

G_DEFINE_TYPE (SwfeditFile, swfedit_file, SWFEDIT_TYPE_TOKEN)

static void
swfedit_file_dispose (GObject *object)
{
  SwfeditFile *file = SWFEDIT_FILE (object);

  g_list_foreach (file->tags, (GFunc) g_object_unref, NULL);
  g_list_free (file->tags);
  g_free (file->filename);

  G_OBJECT_CLASS (swfedit_file_parent_class)->dispose (object);
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

static SwfdecBuffer *
swfenc_file_inflate (SwfdecBits *bits, guint size)
{
  SwfdecBuffer *decoded, *encoded;
  z_stream z;
  int ret;

  encoded = swfdec_bits_get_buffer (bits, -1);
  if (encoded == NULL)
    return NULL;
  decoded = swfdec_buffer_new_and_alloc (size);
  z.zalloc = zalloc;
  z.zfree = zfree;
  z.opaque = NULL;
  z.next_in = encoded->data;
  z.avail_in = encoded->length;
  z.next_out = decoded->data;
  z.avail_out = decoded->length;
  ret = inflateInit (&z);
  SWFDEC_DEBUG ("inflateInit returned %d", ret);
  if (ret >= Z_OK) {
    ret = inflate (&z, Z_SYNC_FLUSH);
    SWFDEC_DEBUG ("inflate returned %d", ret);
  }
  inflateEnd (&z);
  swfdec_buffer_unref (encoded);
  if (ret < Z_OK) {
    swfdec_buffer_unref (decoded);
    return NULL;
  }
  return decoded;
}

static SwfdecBuffer *
swf_parse_header1 (SwfeditFile *file, SwfdecBits *bits, GError **error)
{
  guint sig1, sig2, sig3, bytes_total;

  sig1 = swfdec_bits_get_u8 (bits);
  sig2 = swfdec_bits_get_u8 (bits);
  sig3 = swfdec_bits_get_u8 (bits);
  if ((sig1 != 'F' && sig1 != 'C') || sig2 != 'W' || sig3 != 'S') {
    g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
	"This is not a SWF file");
    return NULL;
  }

  swfedit_token_add (SWFEDIT_TOKEN (file), "version", SWFEDIT_TOKEN_UINT8, 
      GUINT_TO_POINTER (swfdec_bits_get_u8 (bits)));
  bytes_total = swfdec_bits_get_u32 (bits);

  if (sig1 == 'C') {
    /* compressed */
    SwfdecBuffer *ret = swfenc_file_inflate (bits, bytes_total);
    if (ret == NULL)
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
	  "Unable to uncompress file");
    return ret;
  } else {
    SwfdecBuffer *ret = swfdec_bits_get_buffer (bits, -1);
    if (ret == NULL)
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
	  "File too small");
    return ret;
  }
}

static void
swf_parse_header2 (SwfeditFile *file, SwfdecBits *bits)
{
  SwfdecRect rect;

  swfdec_bits_get_rect (bits, &rect);
  swfdec_bits_syncbits (bits);
  swfedit_token_add (SWFEDIT_TOKEN (file), "rate", SWFEDIT_TOKEN_UINT16, 
      GUINT_TO_POINTER (swfdec_bits_get_u16 (bits)));
  swfedit_token_add (SWFEDIT_TOKEN (file), "frames", SWFEDIT_TOKEN_UINT16, 
      GUINT_TO_POINTER (swfdec_bits_get_u16 (bits)));
}

static gboolean
swfedit_file_parse (SwfeditFile *file, SwfdecBits *bits, GError **error)
{
  SwfdecBuffer *next;
  
  next = swf_parse_header1 (file, bits, error);
  if (next == NULL)
    return FALSE;
  swfdec_bits_init (bits, next);
  swf_parse_header2 (file, bits);

  while (swfdec_bits_left (bits)) {
    guint x = swfdec_bits_get_u16 (bits);
    G_GNUC_UNUSED guint tag = (x >> 6) & 0x3ff;
    guint tag_len = x & 0x3f;
    SwfdecBuffer *buffer;
    SwfeditTag *item;

    if (tag_len == 0x3f)
      tag_len = swfdec_bits_get_u32 (bits);
    if (tag == 0)
      break;
    if (tag_len > 0)
      buffer = swfdec_bits_get_buffer (bits, tag_len);
    else
      buffer = swfdec_buffer_new ();
    if (buffer == NULL) {
      g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED,
	  "Invalid contents in file");
      return FALSE;
    }
    item = swfedit_tag_new (SWFEDIT_TOKEN (file), tag, buffer);
    swfedit_token_add (SWFEDIT_TOKEN (file), 
	swfdec_swf_decoder_get_tag_name (tag), 
	SWFEDIT_TOKEN_OBJECT, item);
  }
  swfdec_buffer_unref (next);
  return TRUE;
}

static void
swfedit_file_class_init (SwfeditFileClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = swfedit_file_dispose;
}

static void
swfedit_file_init (SwfeditFile *s)
{
}

SwfeditFile *
swfedit_file_new (const char *filename, GError **error)
{
  SwfeditFile *file;
  SwfdecBuffer *buffer;
  SwfdecBits bits;

  buffer = swfdec_buffer_new_from_file (filename, error);
  if (buffer == NULL)
    return NULL;
  swfdec_bits_init (&bits, buffer);
  file = g_object_new (SWFEDIT_TYPE_FILE, NULL);
  file->filename = g_strdup (filename);
  if (!swfedit_file_parse (file, &bits, error)) {
    swfdec_buffer_unref (buffer);
    g_object_unref (file);
    return NULL;
  }
  swfdec_buffer_unref (buffer);
  return file;
}
