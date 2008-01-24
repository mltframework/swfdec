/* Swfdec
 * Copyright (C) 2006 Benjamin Otte <otte@gnome.org>
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

#include "swfdec/swfdec_ringbuffer.h"

#define SIZE 5

#define ERROR(...) G_STMT_START { \
  g_printerr ("ERROR (line %u): ", __LINE__); \
  g_printerr (__VA_ARGS__); \
  g_printerr ("\n"); \
  errors++; \
} G_STMT_END

static guint
check_set_size (void)
{
  guint errors = 0;
  guint i;
  double* cur;
  SwfdecRingBuffer *buf;

  buf = swfdec_ring_buffer_new_for_type (double, SIZE);
  swfdec_ring_buffer_set_size (buf, SIZE * 2);
  if (swfdec_ring_buffer_get_size (buf) != SIZE * 2) {
    ERROR ("size is %u and not %u", swfdec_ring_buffer_get_size (buf), SIZE * 2);
  }

  /* fill half of the ringbuffer, then clear it again */
  for (i = 0; i < SIZE; i++) {
    cur = swfdec_ring_buffer_push (buf);
    if (cur == NULL) {
      ERROR ("Could not push element %u, even though size is %u", i, SIZE * 2);
    }
  }
  if (swfdec_ring_buffer_get_n_elements (buf) != SIZE) {
    ERROR ("buffer does contain %u elements, not %u", swfdec_ring_buffer_get_n_elements (buf), SIZE);
  }
  for (i = 0; i < SIZE; i++) {
    cur = swfdec_ring_buffer_pop (buf);
    if (cur == NULL) {
      ERROR ("Could not pop element %u, even though size is %u", i, SIZE * 2);
    }
  }
  if (swfdec_ring_buffer_get_n_elements (buf) != 0) {
    ERROR ("buffer does contain %u elements, not %u", swfdec_ring_buffer_get_n_elements (buf), 0);
  }

  /* fill ringbuffer (should be from middle to middle), double size, check it's still correct */
  for (i = 0; i < SIZE * 2; i++) {
    cur = swfdec_ring_buffer_push (buf);
    if (cur == NULL) {
      ERROR ("Could not push element %u, even though size is %u", i, SIZE * 2);
    } else {
      *cur = i;
    }
  }
  if (swfdec_ring_buffer_get_n_elements (buf) != SIZE * 2) {
    ERROR ("buffer does contain %u elements, not %u", swfdec_ring_buffer_get_n_elements (buf), SIZE * 2);
  }
  swfdec_ring_buffer_set_size (buf, SIZE * 4);
  if (swfdec_ring_buffer_get_size (buf) != SIZE * 4) {
    ERROR ("size is %u and not %u", swfdec_ring_buffer_get_size (buf), SIZE * 4);
  }
  if (swfdec_ring_buffer_get_n_elements (buf) != SIZE * 2) {
    ERROR ("buffer does contain %u elements, not %u", swfdec_ring_buffer_get_n_elements (buf), SIZE * 2);
  }
  for (i = 0; i < SIZE * 2; i++) {
    cur = swfdec_ring_buffer_peek_nth (buf, i);
    if (cur == NULL) {
      ERROR ("element %u cannot be peeked", i);
    } else if (*cur != i) {
      ERROR ("element %u has value %g, not %u", i, *cur, i);
    }
  }

  for (i = 0; i < SIZE * 2; i++) {
    cur = swfdec_ring_buffer_pop (buf);
    if (cur == NULL) {
      ERROR ("element %u cannot be popped", i);
    } else if (*cur != i) {
      ERROR ("element %u has value %g, not %u", i, *cur, i);
    }
  }

  swfdec_ring_buffer_free (buf);
  return errors;
}

static guint
check_push_remove (void)
{
  guint errors = 0;
  guint i;
  guint* cur;
  SwfdecRingBuffer *buf;

  buf = swfdec_ring_buffer_new_for_type (guint, SIZE);
  if (swfdec_ring_buffer_get_size (buf) != SIZE) {
    ERROR ("size is %u and not %u", swfdec_ring_buffer_get_size (buf), SIZE);
  }

  for (i = 0; i < SIZE; i++) {
    cur = swfdec_ring_buffer_push (buf);
    if (cur != NULL) {
      *cur = i;
    } else {
      ERROR ("Could not push element %u, even though size is %u", i, SIZE);
    }
  }
  if (swfdec_ring_buffer_get_n_elements (buf) != SIZE) {
    ERROR ("buffer does contain %u elements, not %u", swfdec_ring_buffer_get_n_elements (buf), SIZE);
  } if (swfdec_ring_buffer_push (buf) != NULL) {
    ERROR ("Could push more elements than allowed");
  }

  for (i = 0; i < SIZE; i++) {
    cur = swfdec_ring_buffer_peek_nth (buf, i);
    if (cur == NULL) {
      ERROR ("element %u cannot be peeked", i);
    } else if (*cur != i) {
      ERROR ("element %u has value %u, not %u", i, *cur, i);
    }
  }

  for (i = 0; i < SIZE; i++) {
    cur = swfdec_ring_buffer_pop (buf);
    if (cur == NULL) {
      ERROR ("element %u cannot be popped", i);
    } else if (*cur != i) {
      ERROR ("element %u has value %u, not %u", i, *cur, i);
    }
  }
  if (swfdec_ring_buffer_pop (buf) != NULL) {
    ERROR ("Could pop more elements than allowed");
  }

  swfdec_ring_buffer_free (buf);
  return errors;
}

int
main (int argc, char **argv)
{
  guint errors = 0;

  errors += check_push_remove ();
  errors += check_set_size ();

  g_print ("TOTAL ERRORS: %u\n", errors);
  return errors;
}

