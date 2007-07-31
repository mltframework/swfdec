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

#include "swfdec_gtk_loader.h"

/*** GTK-DOC ***/

/**
 * SECTION:SwfdecGtkLoader
 * @title: SwfdecGtkLoader
 * @short_description: advanced loader able to load network ressources
 * @see_also: #SwfdecLoader
 *
 * #SwfdecGtkLoader is a #SwfdecLoader that is intended as an easy way to be 
 * access ressources that are not stored in files, such as http. It can 
 * however be compiled with varying support for different protocols, so don't
 * rely on support for a particular protocol being available. If you need this,
 * code your own SwfdecLoader subclass.
 */

/**
 * SwfdecGtkLoader:
 *
 * This is the object used to represent a loader. Since it may use varying 
 * backends, it is completely private.
 */

#ifndef HAVE_GNOMEVFS

#include <libswfdec/swfdec_loader_internal.h>

GType
swfdec_gtk_loader_get_type (void)
{
  return SWFDEC_TYPE_FILE_LOADER;
}

SwfdecLoader *
swfdec_gtk_loader_new (const char *uri)
{
  g_return_val_if_fail (uri != NULL, NULL);

  return swfdec_loader_new_from_file (uri);
}


#else /* HAVE_GNOMEVFS */

/* size of buffer we read */
#define BUFFER_SIZE 4096

#include <libgnomevfs/gnome-vfs.h>

struct _SwfdecGtkLoader
{
  SwfdecLoader		loader;

  GnomeVFSURI *		guri;		/* GnomeVFS URI used for resolving */
  GnomeVFSAsyncHandle *	handle;		/* handle to file or NULL when done */
  SwfdecBuffer *	current_buffer;	/* current buffer we're reading into */
};

struct _SwfdecGtkLoaderClass {
  SwfdecLoaderClass	loader_class;
};

/*** SwfdecGtkLoader ***/

G_DEFINE_TYPE (SwfdecGtkLoader, swfdec_gtk_loader, SWFDEC_TYPE_LOADER)

static void swfdec_gtk_loader_start_read (SwfdecGtkLoader *gtk);
static void
swfdec_gtk_loader_read_cb (GnomeVFSAsyncHandle *handle, GnomeVFSResult result,
    gpointer buffer, GnomeVFSFileSize bytes_requested, GnomeVFSFileSize bytes_read, 
    gpointer loaderp)
{
  SwfdecGtkLoader *gtk = loaderp;
  SwfdecLoader *loader = loaderp;

  if (result == GNOME_VFS_ERROR_EOF) {
    swfdec_loader_eof (loader);
    swfdec_buffer_unref (gtk->current_buffer);
    gtk->current_buffer = NULL;
    gnome_vfs_async_cancel (gtk->handle);
    gtk->handle = NULL;
    return;
  } else if (result != GNOME_VFS_OK) {
    char *err = g_strdup_printf ("%s: %s", 
	swfdec_url_get_url (swfdec_loader_get_url (loader)),
	gnome_vfs_result_to_string (result));
    swfdec_loader_error (loader, err);
    g_free (err);
    swfdec_buffer_unref (gtk->current_buffer);
    gtk->current_buffer = NULL;
    gnome_vfs_async_cancel (gtk->handle);
    gtk->handle = NULL;
    return;
  }
  if (bytes_read) {
    gtk->current_buffer->length = bytes_read;
    swfdec_loader_push (loader, gtk->current_buffer);
  } else {
    swfdec_buffer_unref (gtk->current_buffer);
  }
  gtk->current_buffer = NULL;
  swfdec_gtk_loader_start_read (gtk);
}

static void
swfdec_gtk_loader_start_read (SwfdecGtkLoader *gtk)
{
  g_assert (gtk->current_buffer == NULL);
  g_assert (gtk->handle != NULL);

  gtk->current_buffer = swfdec_buffer_new_and_alloc (BUFFER_SIZE);
  gnome_vfs_async_read (gtk->handle, gtk->current_buffer->data,
      gtk->current_buffer->length, swfdec_gtk_loader_read_cb, gtk);
}

static void
swfdec_gtk_loader_open_cb (GnomeVFSAsyncHandle *handle, GnomeVFSResult result, 
    gpointer loaderp)
{
  SwfdecGtkLoader *gtk = loaderp;
  SwfdecLoader *loader = loaderp;

  if (result != GNOME_VFS_OK) {
    char *err = g_strdup_printf ("%s: %s",
	swfdec_url_get_url (swfdec_loader_get_url (loader)),
	gnome_vfs_result_to_string (result));
    swfdec_loader_error (loader, err);
    g_free (err);
    gnome_vfs_async_cancel (gtk->handle);
    gtk->handle = NULL;
    return;
  }
  swfdec_gtk_loader_start_read (gtk);
}

static void
swfdec_gtk_loader_dispose (GObject *object)
{
  SwfdecGtkLoader *gtk = SWFDEC_GTK_LOADER (object);

  if (gtk->current_buffer) {
    swfdec_buffer_unref (gtk->current_buffer);
    gtk->current_buffer = NULL;
  }
  if (gtk->handle) {
    gnome_vfs_async_cancel (gtk->handle);
    gtk->handle = NULL;
  }
  if (gtk->guri) {
    gnome_vfs_uri_unref (gtk->guri);
    gtk->guri = NULL;
  }

  G_OBJECT_CLASS (swfdec_gtk_loader_parent_class)->dispose (object);
}

static void
swfdec_gtk_loader_load (SwfdecLoader *loader,
    SwfdecLoaderRequest request, const char *data, gsize data_len)
{
  SwfdecGtkLoader *gtk = SWFDEC_GTK_LOADER (loader);

  gnome_vfs_async_open (&gtk->handle, swfdec_url_get_url (swfdec_loader_get_url (loader)), 
      GNOME_VFS_OPEN_READ, GNOME_VFS_PRIORITY_DEFAULT, swfdec_gtk_loader_open_cb, gtk);
}

static void
swfdec_gtk_loader_class_init (SwfdecGtkLoaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecLoaderClass *loader_class = SWFDEC_LOADER_CLASS (klass);

  gnome_vfs_init ();

  object_class->dispose = swfdec_gtk_loader_dispose;

  loader_class->load = swfdec_gtk_loader_load;
}

static void
swfdec_gtk_loader_init (SwfdecGtkLoader *gtk_loader)
{
}

/**
 * swfdec_gtk_loader_new:
 * @uri: The location of the file to open
 *
 * Creates a new loader for the given URI using gnome-vfs (or using the local
 * file backend, if compiled without gnome-vfs support). The uri must be valid
 * UTF-8. If using gnome-vfs, you might want to use 
 * gnome_vfs_make_uri_from_shell_arg() or gnome_vfs_make_uri_from_input() on
 * the @uri prior to calling this function.
 *
 * Returns: a new #SwfdecLoader using gnome-vfs.
 **/
SwfdecLoader *
swfdec_gtk_loader_new (const char *uri)
{
  SwfdecLoader *loader;

  g_return_val_if_fail (uri != NULL, NULL);

  loader = g_object_new (SWFDEC_TYPE_GTK_LOADER, NULL);
  loader->url = swfdec_url_new (uri);
  swfdec_gtk_loader_load (loader, SWFDEC_LOADER_REQUEST_DEFAULT, NULL, 0);
  return loader;
}

#endif /* HAVE_GNOMEVFS */
