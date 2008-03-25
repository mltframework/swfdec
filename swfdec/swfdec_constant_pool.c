/* Swfdec
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

#include "swfdec_constant_pool.h"
#include "swfdec_debug.h"

struct _SwfdecConstantPool {
  SwfdecAsContext *	context;	/* context we are attached to or NULL */
  guint			n_strings;	/* number of strings */
  char *		strings[1];	/* n_strings strings */
};

SwfdecConstantPool *
swfdec_constant_pool_new_from_action (const guint8 *data, guint len, guint version)
{
  guint i, n;
  SwfdecBits bits;
  SwfdecConstantPool *pool;

  swfdec_bits_init_data (&bits, data, len);

  n = swfdec_bits_get_u16 (&bits);
  if (n == 0)
    return NULL;

  pool = g_malloc0 (sizeof (SwfdecConstantPool) + (n - 1) * sizeof (char *));
  pool->n_strings = n;
  for (i = 0; i < n; i++) {
    pool->strings[i] = swfdec_bits_get_string (&bits, version);
    if (pool->strings[i] == NULL) {
      SWFDEC_ERROR ("not enough strings available");
      swfdec_constant_pool_free (pool);
      return NULL;
    }
  }
  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("constant pool didn't consume whole buffer (%u bytes leftover)", swfdec_bits_left (&bits) / 8);
  }
  return pool;
}

void
swfdec_constant_pool_attach_to_context (SwfdecConstantPool *pool, SwfdecAsContext *context)
{
  guint i;

  g_return_if_fail (pool != NULL);
  g_return_if_fail (pool->context == NULL);
  g_return_if_fail (SWFDEC_IS_AS_CONTEXT (context));

  pool->context = context;
  for (i = 0; i < pool->n_strings; i++) {
    pool->strings[i] = (char *) swfdec_as_context_give_string (context, pool->strings[i]);
  }
}

guint
swfdec_constant_pool_size (SwfdecConstantPool *pool)
{
  return pool->n_strings;
}

const char *
swfdec_constant_pool_get (SwfdecConstantPool *pool, guint i)
{
  g_assert (i < pool->n_strings);
  return pool->strings[i];
}

void
swfdec_constant_pool_free (SwfdecConstantPool *pool)
{
  if (pool->context == NULL) {
    guint i;
    for (i = 0; i < pool->n_strings; i++) {
      g_free (pool->strings[i]);
    }
  }
  g_free (pool);
}

