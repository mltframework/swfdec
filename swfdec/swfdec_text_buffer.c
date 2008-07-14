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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include "swfdec_text_buffer.h"
#include "swfdec_debug.h"
#include "swfdec_marshal.h"

typedef struct _SwfdecTextBufferFormat SwfdecTextBufferFormat;

struct _SwfdecTextBufferFormat {
  guint			start;
  SwfdecTextAttributes	attr;
};

static SwfdecTextBufferFormat *
swfdec_text_buffer_format_new (void)
{
  SwfdecTextBufferFormat *format;
  
  format = g_slice_new0 (SwfdecTextBufferFormat);
  swfdec_text_attributes_reset (&format->attr);
  return format;
}

static void
swfdec_text_buffer_format_free (SwfdecTextBufferFormat *format)
{
  swfdec_text_attributes_reset (&format->attr);
  g_slice_free (SwfdecTextBufferFormat, format);
}

/*** OBJECT ***/

enum {
  TEXT_CHANGED,
  CURSOR_CHANGED,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (SwfdecTextBuffer, swfdec_text_buffer, G_TYPE_OBJECT)

static void
swfdec_text_buffer_dispose (GObject *object)
{
  SwfdecTextBuffer *buffer = SWFDEC_TEXT_BUFFER (object);

  if (buffer->text != NULL) {
    g_string_free (buffer->text, TRUE);
    buffer->text = NULL;
  }
  g_sequence_free (buffer->attributes);
  swfdec_text_attributes_reset (&buffer->default_attributes);

  G_OBJECT_CLASS (swfdec_text_buffer_parent_class)->dispose (object);
}

static void
swfdec_text_buffer_class_init (SwfdecTextBufferClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_text_buffer_dispose;

  signals[TEXT_CHANGED] = g_signal_new ("text-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, g_cclosure_marshal_VOID__VOID,
      G_TYPE_NONE, 0);
  signals[CURSOR_CHANGED] = g_signal_new ("cursor-changed", G_TYPE_FROM_CLASS (klass),
      G_SIGNAL_RUN_LAST, 0, NULL, NULL, swfdec_marshal_VOID__ULONG_ULONG,
      G_TYPE_NONE, 2, G_TYPE_ULONG, G_TYPE_ULONG);
}

static void
swfdec_text_buffer_init (SwfdecTextBuffer *buffer)
{
  buffer->text = g_string_new ("");
  buffer->attributes = g_sequence_new ((GDestroyNotify) swfdec_text_buffer_format_free);
  swfdec_text_attributes_reset (&buffer->default_attributes);
}

SwfdecTextBuffer *
swfdec_text_buffer_new (void)
{
  return g_object_new (SWFDEC_TYPE_TEXT_BUFFER, NULL);
}

static void
swfdec_text_buffer_mark_one (gpointer entry, gpointer unused)
{
  SwfdecTextBufferFormat *format = entry;

  swfdec_text_attributes_mark (&format->attr);
}

void
swfdec_text_buffer_mark (SwfdecTextBuffer *buffer)
{
  g_return_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer));

  g_sequence_foreach (buffer->attributes, swfdec_text_buffer_mark_one, NULL);
}

static int
swfdec_text_buffer_format_compare (gconstpointer a, gconstpointer b, gpointer unused)
{
  return ((const SwfdecTextBufferFormat *) a)->start - 
    ((const SwfdecTextBufferFormat *) b)->start;
}

/* return iter to format relevant for position, that is the format pointed to
 * by iter will have a lower or equal start index than pos */
static GSequenceIter *
swfdec_text_buffer_get_iter_for_pos (SwfdecTextBuffer *buffer, guint pos)
{
  SwfdecTextBufferFormat format = { pos, };
  GSequenceIter *iter;

  iter = g_sequence_search (buffer->attributes, &format,
      swfdec_text_buffer_format_compare, NULL);
  if (g_sequence_iter_is_end (iter) ||
      ((SwfdecTextBufferFormat *) g_sequence_get (iter))->start > pos)
    iter = g_sequence_iter_prev (iter);

  return iter;
}

#ifdef G_DISABLE_ASSERT
#define CHECK_ATTRIBUTES(x)
#else
static void
CHECK_ATTRIBUTES (SwfdecTextBuffer *buffer)
{
  guint last;
  GSequenceIter *iter;
  SwfdecTextBufferFormat *format;

  iter = g_sequence_get_begin_iter (buffer->attributes);
  if (g_sequence_iter_is_end (iter)) {
    g_assert (buffer->text->len == 0);
    return;
  }
  format = g_sequence_get (iter);
  g_assert (format->start == 0);
  g_assert (buffer->text->len > 0);
  last = 0;
  for (iter = g_sequence_iter_next (iter); !g_sequence_iter_is_end (iter); iter = g_sequence_iter_next (iter)) {
    format = g_sequence_get (iter);
    g_assert (format->start > last);
    g_assert (format->start < buffer->text->len);
    last = format->start;
  }
}
#endif

/* removes duplicates in the range [iter, end) */
static void
swfdec_text_buffer_remove_duplicates (GSequenceIter *iter, GSequenceIter *end)
{
  SwfdecTextBufferFormat *format, *next;

  g_assert (iter != end);

  format = g_sequence_get (iter);
  for (iter = g_sequence_iter_next (iter); iter != end; iter = g_sequence_iter_next (iter)) {
    next = g_sequence_get (iter);
    if (swfdec_text_attributes_diff (&format->attr, &next->attr) == 0) {
      GSequenceIter *prev = g_sequence_iter_prev (iter);
      g_sequence_remove (iter);
      iter = prev;
    } else {
      format = next;
    }
  }
}

void
swfdec_text_buffer_insert_text (SwfdecTextBuffer *buffer,
    gsize pos, const char *text)
{
  gsize len;
  GSequenceIter *iter;
  SwfdecTextBufferFormat *format;

  g_return_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (pos <= buffer->text->len);
  g_return_if_fail (text != NULL);

  len = strlen (text);
  if (len == 0)
    return;
  if (pos == buffer->text->len) {
    g_string_insert_len (buffer->text, pos, text, len);
    format = swfdec_text_buffer_format_new ();
    format->start = pos;
    swfdec_text_attributes_copy (&format->attr, 
	&buffer->default_attributes, SWFDEC_TEXT_ATTRIBUTES_MASK);
    iter = g_sequence_append (buffer->attributes, format);
    swfdec_text_buffer_remove_duplicates (g_sequence_iter_prev (iter),
	g_sequence_iter_next (iter));
  } else {
    g_string_insert_len (buffer->text, pos, text, len);
    iter = g_sequence_get_end_iter (buffer->attributes);
    for (;;) {
      iter = g_sequence_iter_prev (iter);
      format = g_sequence_get (iter);
      if (format->start <= pos)
	break;
      format->start += len;
    }
  }
  CHECK_ATTRIBUTES (buffer);

  /* adapt cursor */
  if (buffer->cursor_start >= pos)
    buffer->cursor_start += len;
  if (buffer->cursor_end >= pos)
    buffer->cursor_end += len;

  g_signal_emit (buffer, signals[TEXT_CHANGED], 0);
  g_signal_emit (buffer, signals[CURSOR_CHANGED], 0,
      (gulong) MIN (buffer->cursor_start, buffer->cursor_end),
      (gulong) MAX (buffer->cursor_start, buffer->cursor_end));
}

void
swfdec_text_buffer_delete_text (SwfdecTextBuffer *buffer, 
    gsize pos, gsize length)
{
  GSequenceIter *iter, *prev;
  SwfdecTextBufferFormat *format;
  gsize end;

  g_return_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (pos + length <= buffer->text->len);
  g_return_if_fail (length > 0);

  g_string_erase (buffer->text, pos, length);
  end = pos + length;
  prev = NULL;
  for (iter = swfdec_text_buffer_get_iter_for_pos (buffer, pos);
      !g_sequence_iter_is_end (iter);
      iter = g_sequence_iter_next (iter)) {
    format = g_sequence_get (iter);
    if (format->start < pos)
      continue;
    if (format->start > end) {
      format->start -= length;
      continue;
    }
    if (prev)
      g_sequence_remove (prev);
    format->start = pos;
    prev = iter;
  }
  if (prev && pos == buffer->text->len)
    g_sequence_remove (prev);
  CHECK_ATTRIBUTES (buffer);

  /* adapt cursor */
  if (buffer->cursor_start > end)
    buffer->cursor_start -= length;
  else if (buffer->cursor_start > pos)
    buffer->cursor_start = pos;
  if (buffer->cursor_end > end)
    buffer->cursor_end -= length;
  else if (buffer->cursor_end > pos)
    buffer->cursor_end = pos;

  g_signal_emit (buffer, signals[TEXT_CHANGED], 0);
  g_signal_emit (buffer, signals[CURSOR_CHANGED], 0,
      (gulong) MIN (buffer->cursor_start, buffer->cursor_end),
      (gulong) MAX (buffer->cursor_start, buffer->cursor_end));
}

const char *
swfdec_text_buffer_get_text (SwfdecTextBuffer *buffer)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), NULL);
  
  return buffer->text->str;
}

gsize
swfdec_text_buffer_get_length (SwfdecTextBuffer *buffer)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), 0);
  
  return buffer->text->len;
}

const SwfdecTextAttributes *
swfdec_text_buffer_get_attributes (SwfdecTextBuffer *buffer, gsize pos)
{
  GSequenceIter *iter;
  SwfdecTextBufferFormat *format;

  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (pos <= buffer->text->len, NULL);

  if (pos == buffer->text->len)
    return &buffer->default_attributes;

  iter = swfdec_text_buffer_get_iter_for_pos (buffer, pos);
  format = g_sequence_get (iter);
  return &format->attr;
}

guint
swfdec_text_buffer_get_unique (SwfdecTextBuffer *buffer, 
    gsize start, gsize length)
{
  guint result = SWFDEC_TEXT_ATTRIBUTES_MASK;
  SwfdecTextBufferFormat *format, *cur;
  GSequenceIter *iter;
  gsize end;

  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), 0);
  g_return_val_if_fail (start + length <= buffer->text->len, 0);

  if (start == buffer->text->len)
    return result;

  end = start + length;
  iter = swfdec_text_buffer_get_iter_for_pos (buffer, start);
  format = g_sequence_get (iter);
  for (iter = g_sequence_iter_next (iter); !g_sequence_iter_is_end (iter); iter = g_sequence_iter_next (iter)) {
    cur = g_sequence_get (iter);
    if (cur->start >= end)
      break;
    result &= ~swfdec_text_attributes_diff (&format->attr, &cur->attr);
  }

  return result;
}

static GSequenceIter *
swfdec_text_buffer_copy_format (SwfdecTextBuffer *buffer, GSequenceIter *iter, 
    gsize pos)
{
  SwfdecTextBufferFormat *format, *new_format;
 
  format = g_sequence_get (iter);
  new_format = swfdec_text_buffer_format_new ();
  new_format->start = pos;
  swfdec_text_attributes_copy (&new_format->attr, &format->attr, 
      SWFDEC_TEXT_ATTRIBUTES_MASK);
  return g_sequence_insert_before (g_sequence_iter_next (iter), new_format);
}

void
swfdec_text_buffer_set_attributes (SwfdecTextBuffer *buffer, gsize start,
    gsize length, const SwfdecTextAttributes *attr, guint mask)
{
  SwfdecTextBufferFormat *format;
  GSequenceIter *start_iter, *iter, *end_iter;
  gsize end;

  g_return_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start + length <= buffer->text->len);
  g_return_if_fail (length > 0);
  g_return_if_fail (attr != NULL);
  g_return_if_fail (mask != 0);

  /* ensure start and end position have an attribtues entry. If not, create one */
  end = start + length;
  start_iter = swfdec_text_buffer_get_iter_for_pos (buffer, start);
  format = g_sequence_get (start_iter);
  if (format->start < start) {
    start_iter = swfdec_text_buffer_copy_format (buffer, start_iter, start);
  }
  if (end == buffer->text->len) {
    end_iter = g_sequence_get_end_iter (buffer->attributes);
  } else {
    end_iter = swfdec_text_buffer_get_iter_for_pos (buffer, end);
    format = g_sequence_get (end_iter);
    if (format->start < end) {
      end_iter = swfdec_text_buffer_copy_format (buffer, end_iter, end);
    }
  }
  /* start_iter points to first item to modify, end_iter points after last item to modify */

  /* modify all formats in range [start_iter, end_iter) */
  for (iter = start_iter; iter != end_iter; iter = g_sequence_iter_next (iter)) {
    format = g_sequence_get (iter);
    swfdec_text_attributes_copy (&format->attr, attr, mask);
  }

  /* remove entries that are identical now */
  swfdec_text_buffer_remove_duplicates (g_sequence_iter_prev (start_iter), 
      g_sequence_iter_next (end_iter));
  CHECK_ATTRIBUTES (buffer);

  g_signal_emit (buffer, signals[TEXT_CHANGED], 0);
}

const SwfdecTextAttributes *
swfdec_text_buffer_get_default_attributes (SwfdecTextBuffer *buffer)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), NULL);

  return &buffer->default_attributes;
}

void
swfdec_text_buffer_set_default_attributes (SwfdecTextBuffer *buffer, 
    const SwfdecTextAttributes *attr, guint mask)
{
  g_return_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (attr != NULL);

  swfdec_text_attributes_copy (&buffer->default_attributes, attr, mask);
}

void
swfdec_text_buffer_reset_default_attributes (SwfdecTextBuffer *buffer)
{
  g_return_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer));

  swfdec_text_attributes_reset (&buffer->default_attributes);
}

/*** ITERATOR ***/

SwfdecTextBufferIter *
swfdec_text_buffer_get_iter (SwfdecTextBuffer *buffer, gsize pos)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (pos <= buffer->text->len, NULL);

  if (pos == buffer->text->len)
    return NULL;

  return swfdec_text_buffer_get_iter_for_pos (buffer, pos);
}

SwfdecTextBufferIter *
swfdec_text_buffer_iter_next (SwfdecTextBuffer *buffer, SwfdecTextBufferIter *iter)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), NULL);
  g_return_val_if_fail (iter != NULL, NULL);

  iter = g_sequence_iter_next (iter);
  return g_sequence_iter_is_end (iter) ? NULL : iter;
}

const SwfdecTextAttributes *
swfdec_text_buffer_iter_get_attributes (SwfdecTextBuffer *buffer, SwfdecTextBufferIter *iter)
{
  SwfdecTextBufferFormat *format;

  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), NULL);

  if (iter == NULL)
    return &buffer->default_attributes;

  format = g_sequence_get (iter);
  return &format->attr;
}

gsize	
swfdec_text_buffer_iter_get_start (SwfdecTextBuffer *buffer, SwfdecTextBufferIter *iter)
{
  SwfdecTextBufferFormat *format;

  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), 0);
  g_return_val_if_fail (iter != NULL, 0);

  format = g_sequence_get (iter);
  return format->start;
}

/*** CURSOR API ***/

gsize
swfdec_text_buffer_get_cursor (SwfdecTextBuffer *buffer)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), 0);

  return buffer->cursor_end;
}

gboolean
swfdec_text_buffer_has_selection (SwfdecTextBuffer *buffer)
{
  g_return_val_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer), FALSE);

  return buffer->cursor_start != buffer->cursor_end;
}

void
swfdec_text_buffer_get_selection (SwfdecTextBuffer *buffer, gsize *start, gsize *end)
{
  g_return_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer));

  if (start)
    *start = MIN (buffer->cursor_start, buffer->cursor_end);
  if (end)
    *end = MAX (buffer->cursor_start, buffer->cursor_end);
}

void
swfdec_text_buffer_set_cursor (SwfdecTextBuffer *buffer, gsize start, gsize end)
{
  g_return_if_fail (SWFDEC_IS_TEXT_BUFFER (buffer));
  g_return_if_fail (start <= swfdec_text_buffer_get_length (buffer));
  g_return_if_fail (end <= swfdec_text_buffer_get_length (buffer));

  if (buffer->cursor_start == start &&
      buffer->cursor_end == end)
    return;

  buffer->cursor_start = start;
  buffer->cursor_end = end;
  g_signal_emit (buffer, signals[CURSOR_CHANGED], 0,
      (gulong) MIN (buffer->cursor_start, buffer->cursor_end),
      (gulong) MAX (buffer->cursor_start, buffer->cursor_end));
}

