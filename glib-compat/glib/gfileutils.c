/* gfileutils.c - File utility functions
 *
 *  Copyright 2000 Red Hat, Inc.
 *
 * GLib is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * GLib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GLib; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 *   Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "glib.h"

#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#ifdef G_OS_WIN32
#include <io.h>
#ifndef F_OK
#define	F_OK 0
#define	W_OK 2
#define	R_OK 4
#endif /* !F_OK */

#ifndef S_ISREG
#define S_ISREG(mode) ((mode)&_S_IFREG)
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) ((mode)&_S_IFDIR)
#endif

#endif /* G_OS_WIN32 */

#ifndef S_ISLNK
#define S_ISLNK(x) 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

//#include "glibintl.h"

#ifdef G_OS_WIN32
#define G_IS_DIR_SEPARATOR(c) (c == G_DIR_SEPARATOR || c == '/')
#else
#define G_IS_DIR_SEPARATOR(c) (c == G_DIR_SEPARATOR)
#endif

static gboolean
get_contents_stdio (const gchar *filename,
                    FILE        *f,
                    gchar      **contents,
                    gsize       *length, 
                    GError     **error)
{
  gchar buf[2048];
  size_t bytes;
  char *str;
  size_t total_bytes;
  size_t total_allocated;
  
  g_assert (f != NULL);

#define STARTING_ALLOC 64
  
  total_bytes = 0;
  total_allocated = STARTING_ALLOC;
  str = g_malloc (STARTING_ALLOC);
  
  while (!feof (f))
    {
      bytes = fread (buf, 1, 2048, f);

      while ((total_bytes + bytes + 1) > total_allocated)
        {
          total_allocated *= 2;
          str = g_try_realloc (str, total_allocated);

          if (str == NULL)
            {
printf("error %s:%d\n",__FILE__,__LINE__);
              goto error;
            }
        }
      
      if (ferror (f))
        {
printf("error %s:%d\n",__FILE__,__LINE__);
          goto error;
        }

      memcpy (str + total_bytes, buf, bytes);
      total_bytes += bytes;
    }

  fclose (f);

  str[total_bytes] = '\0';
  
  if (length)
    *length = total_bytes;
  
  *contents = str;
  
  return TRUE;

 error:

  g_free (str);
  fclose (f);
  
  return FALSE;  
}

static gboolean
get_contents_win32 (const gchar *filename,
                    gchar      **contents,
                    gsize       *length,
                    GError     **error)
{
  FILE *f;

  /* I guess you want binary mode; maybe you want text sometimes? */
  f = fopen (filename, "rb");

  if (f == NULL)
    {
printf("error %s:%d\n",__FILE__,__LINE__);
      return FALSE;
    }
  
  return get_contents_stdio (filename, f, contents, length, error);
}

/**
 * g_file_get_contents:
 * @filename: a file to read contents from
 * @contents: location to store an allocated string
 * @length: location to store length in bytes of the contents
 * @error: return location for a #GError
 * 
 * Reads an entire file into allocated memory, with good error
 * checking. If @error is set, %FALSE is returned, and @contents is set
 * to %NULL. If %TRUE is returned, @error will not be set, and @contents
 * will be set to the file contents.  The string stored in @contents
 * will be nul-terminated, so for text files you can pass %NULL for the
 * @length argument.  The error domain is #G_FILE_ERROR. Possible
 * error codes are those in the #GFileError enumeration.
 *
 * Return value: %TRUE on success, %FALSE if error is set
 **/
gboolean
g_file_get_contents (const gchar *filename,
                     gchar      **contents,
                     gsize       *length,
                     GError     **error)
{  
  g_return_val_if_fail (filename != NULL, FALSE);
  g_return_val_if_fail (contents != NULL, FALSE);

  *contents = NULL;
  if (length)
    *length = 0;

#ifdef G_OS_WIN32
  return get_contents_win32 (filename, contents, length, error);
#else
  return get_contents_posix (filename, contents, length, error);
#endif
}

