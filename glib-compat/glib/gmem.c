/* GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


#include "config.h"
#include <stdlib.h>
#include <glib.h>


gpointer g_malloc         (gulong	 n_bytes)
{
  return malloc(n_bytes);
}

gpointer g_malloc0        (gulong	 n_bytes)
{
  gpointer ptr;

  ptr = malloc(n_bytes);
  memset(ptr,0,n_bytes);
  return ptr;
}

gpointer g_realloc        (gpointer	 mem,
			   gulong	 n_bytes)
{
  return realloc(mem,n_bytes);
}

void	 g_free	          (gpointer	 mem)
{
  if(mem == NULL)return;
  free(mem);
}

gpointer g_try_malloc     (gulong	 n_bytes)
{
  return malloc(n_bytes);
}

gpointer g_try_realloc    (gpointer	 mem,
			   gulong	 n_bytes)
{
  return realloc(mem,n_bytes);
}


struct _GMemChunk {
  int size;
};

GMemChunk* g_mem_chunk_new     (const gchar *name,
				gint         atom_size,
				gulong       area_size,
				gint         type)
{
  GMemChunk *mem_chunk = g_malloc(sizeof(GMemChunk));
  mem_chunk->size = atom_size;
  return mem_chunk;
}

void       g_mem_chunk_destroy (GMemChunk   *mem_chunk)
{
  g_free(mem_chunk);
}

gpointer   g_mem_chunk_alloc   (GMemChunk   *mem_chunk)
{
  return g_malloc(mem_chunk->size);
}

gpointer   g_mem_chunk_alloc0  (GMemChunk   *mem_chunk)
{
  return g_malloc0(mem_chunk->size);
}

void       g_mem_chunk_free    (GMemChunk   *mem_chunk,
				gpointer     mem)
{
  g_free(mem);
}

