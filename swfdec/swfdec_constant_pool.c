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
#include "swfdec_bits.h"
#include "swfdec_debug.h"

struct _SwfdecConstantPool {
  SwfdecAsContext *	context;	/* context we are attached to or NULL */
  SwfdecBuffer *	buffer;		/* the buffer the strings were read from */
  guint			refcount;	/* reference count */
  guint			n_strings;	/* number of strings */
  char *		strings[1];	/* n_strings strings */
};

/**
 * swfdec_constant_pool_new:
 * @context: a context to attach strings to or %NULL
 * @buffer: buffer to read constant pool from
 * @version: the Flash version to use when reading the strings
 *
 * Creates a new constant pool from the given @buffer. If the buffer is in the 
 * wrong format, %NULL is returned. If a @context was given, the strings are 
 * added to the context. This means the strings in the pool will be 
 * garbage-collected already. When using a non-%NULL @context, the pool does 
 * will not add a reference to the context. It is your responsibility to hold a
 * reference to the context while you use the pool.
 *
 * Returns: a new constant pool or %NULL
 **/
SwfdecConstantPool *
swfdec_constant_pool_new (SwfdecAsContext *context, SwfdecBuffer *buffer, guint version)
{
  guint i, n;
  gsize size;
  SwfdecBits bits;
  SwfdecConstantPool *pool;

  g_return_val_if_fail (context == NULL || SWFDEC_IS_AS_CONTEXT (context), NULL);
  g_return_val_if_fail (buffer != NULL, NULL);

  /* try to find the pool in the context's cache */
  if (context) {
    pool = g_hash_table_lookup (context->constant_pools, buffer->data);
    if (pool)
      return swfdec_constant_pool_ref (pool);
  }

  swfdec_bits_init (&bits, buffer);

  n = swfdec_bits_get_u16 (&bits);

  size = sizeof (SwfdecConstantPool) + (MAX (1, n) - 1) * sizeof (char *);
  pool = g_slice_alloc0 (size);
  pool->n_strings = n;
  for (i = 0; i < n; i++) {
    pool->strings[i] = swfdec_bits_get_string (&bits, version);
    if (pool->strings[i] == NULL) {
      SWFDEC_ERROR ("not enough strings available");
      g_slice_free1 (size, pool);
      return NULL;
    }
    if (context)
      pool->strings[i] = (char *) swfdec_as_context_give_string (context, pool->strings[i]);
  }
  if (swfdec_bits_left (&bits)) {
    SWFDEC_WARNING ("constant pool didn't consume whole buffer (%u bytes leftover)", swfdec_bits_left (&bits) / 8);
  }
  pool->buffer = swfdec_buffer_ref (buffer);
  pool->refcount = 1;
  /* put pool into the context's cache */
  if (context) {
    pool->context = context;
    g_hash_table_insert (context->constant_pools, buffer->data, pool);
  }
  return pool;
}

/**
 * swfdec_constant_pool_ref:
 * @pool: a constant pool
 *
 * Increases the constant pool's reference by one.
 *
 * Returns: the passed in @pool.
 **/
SwfdecConstantPool *
swfdec_constant_pool_ref (SwfdecConstantPool *pool)
{
  g_return_val_if_fail (SWFDEC_IS_CONSTANT_POOL (pool), NULL);

  pool->refcount++;

  return pool;
}

/**
 * swfdec_constant_pool_unref:
 * @pool: the pool to unref
 *
 * Removes a reference from the pool. If no more references are left, the pool
 * will be freed.
 **/
void
swfdec_constant_pool_unref (SwfdecConstantPool *pool)
{
  g_return_if_fail (SWFDEC_IS_CONSTANT_POOL (pool));
  g_return_if_fail (pool->refcount > 0);

  pool->refcount--;
  if (pool->refcount)
    return;

  if (pool->context) {
    g_hash_table_remove (pool->context->constant_pools, pool->buffer->data);
  } else {
    guint i;
    for (i = 0; i < pool->n_strings; i++) {
      g_free (pool->strings[i]);
    }
  }
  swfdec_buffer_unref (pool->buffer);
  g_slice_free1 (sizeof (SwfdecConstantPool) + (MAX (1, pool->n_strings) - 1) * sizeof (char *), pool);
}

/**
 * swfdec_constant_pool_size:
 * @pool: a pool
 *
 * Queries the number of strings in this pool.
 *
 * Returns: The number of strings in this @pool
 **/
guint
swfdec_constant_pool_size (SwfdecConstantPool *pool)
{
  g_return_val_if_fail (SWFDEC_IS_CONSTANT_POOL (pool), 0);

  return pool->n_strings;
}

/**
 * swfdec_constant_pool_get:
 * @pool: a pool
 * @i: index of the string to get
 *
 * Gets the requested string from the pool. The index must not excess the 
 * number of elements in the pool. If the constant pool was created with a 
 * context attached, the returned string will be garbage-collected already.
 *
 * Returns: the string at position @i
 **/
const char *
swfdec_constant_pool_get (SwfdecConstantPool *pool, guint i)
{
  g_return_val_if_fail (SWFDEC_IS_CONSTANT_POOL (pool), NULL);
  g_return_val_if_fail (i < pool->n_strings, NULL);

  return pool->strings[i];
}

/**
 * swfdec_constant_pool_get_buffer:
 * @pool: a constant pool
 *
 * Gets the buffer the pool was created from
 *
 * Returns: the buffer this pool was created from.
 **/
SwfdecBuffer *
swfdec_constant_pool_get_buffer (SwfdecConstantPool *pool)
{
  g_return_val_if_fail (SWFDEC_IS_CONSTANT_POOL (pool), NULL);

  return pool->buffer;
}

