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

#include "swfdec_loader_internal.h"
#include "swfdec_buffer.h"
#include "swfdec_debug.h"
#include "swfdec_root_movie.h"

/*** gtk-doc ***/

/**
 * SECTION:SwfdecLoader
 * @title: SwfdecLoader
 * @short_description: object used for input
 *
 * SwfdecLoader is the base class used for input. Since developers normally 
 * need to adapt input to the needs of their application, this class is 
 * provided to be adapted to their needs.
 *
 * Since Flash files can load new resources while operating, a #SwfdecLoader
 * can be instructed to load another resource. It's the loader's responsibility
 * to make sure the player is allowed to access the resource and provide its
 * data.
 *
 * For convenience, a #SwfdecLoader for file access is provided by Swfdec.
 */

/**
 * SwfdecLoader:
 *
 * This is the base class used for providing input. It is abstract, use a 
 * subclass to provide your input.
 */

/*** SwfdecLoader ***/

G_DEFINE_ABSTRACT_TYPE (SwfdecLoader, swfdec_loader, G_TYPE_OBJECT)

static void
swfdec_loader_dispose (GObject *object)
{
  SwfdecLoader *loader = SWFDEC_LOADER (object);

  swfdec_buffer_queue_free (loader->queue);
  g_free (loader->url);

  G_OBJECT_CLASS (swfdec_loader_parent_class)->dispose (object);
}

static void
swfdec_loader_do_error (SwfdecLoader *loader, const char *error)
{
  SWFDEC_ERROR ("Error from loader %p: %s", loader, error);
}

static void
swfdec_loader_class_init (SwfdecLoaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_loader_dispose;

  klass->error = swfdec_loader_do_error;
}

static void
swfdec_loader_init (SwfdecLoader *loader)
{
  loader->queue = swfdec_buffer_queue_new ();
}

/*** SwfdecFileLoader ***/

typedef struct _SwfdecFileLoader SwfdecFileLoader;
typedef struct _SwfdecFileLoaderClass SwfdecFileLoaderClass;

#define SWFDEC_TYPE_FILE_LOADER                    (swfdec_file_loader_get_type())
#define SWFDEC_IS_FILE_LOADER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_FILE_LOADER))
#define SWFDEC_IS_FILE_LOADER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_FILE_LOADER))
#define SWFDEC_FILE_LOADER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_FILE_LOADER, SwfdecFileLoader))
#define SWFDEC_FILE_LOADER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_FILE_LOADER, SwfdecFileLoaderClass))
#define SWFDEC_FILE_LOADER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_FILE_LOADER, SwfdecFileLoaderClass))

struct _SwfdecFileLoader
{
  SwfdecLoader		loader;

  char *		dir;		/* base directory for load operations */
};

struct _SwfdecFileLoaderClass
{
  SwfdecLoaderClass   	loader_class;
};

G_DEFINE_TYPE (SwfdecFileLoader, swfdec_file_loader, SWFDEC_TYPE_LOADER)

static void
swfdec_file_loader_dispose (GObject *object)
{
  SwfdecFileLoader *file_loader = SWFDEC_FILE_LOADER (object);

  g_free (file_loader->dir);

  G_OBJECT_CLASS (swfdec_file_loader_parent_class)->dispose (object);
}

static SwfdecLoader *
swfdec_file_loader_load (SwfdecLoader *loader, const char *url)
{
  SwfdecBuffer *buffer;
  char *real_path;
  SwfdecLoader *ret;
  GError *error = NULL;

  if (g_path_is_absolute (url)) {
    SWFDEC_ERROR ("absolute paths are not allowed");
    return NULL;
  }

  /* FIXME: need to rework seperators on windows? */
  real_path = g_build_filename (SWFDEC_FILE_LOADER (loader)->dir, url, NULL);
  buffer = swfdec_buffer_new_from_file (real_path, &error);
  if (buffer == NULL) {
    SWFDEC_ERROR ("Couldn't load \"%s\": %s", real_path, error->message);
    g_free (real_path);
    g_error_free (error);
    return NULL;
  }
  ret = g_object_new (SWFDEC_TYPE_FILE_LOADER, NULL);
  ret->url = real_path;
  SWFDEC_FILE_LOADER (ret)->dir = g_strdup (SWFDEC_FILE_LOADER (loader)->dir);
  swfdec_loader_push (ret, buffer);
  swfdec_loader_eof (ret);

  return ret;
}

static void
swfdec_file_loader_class_init (SwfdecFileLoaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecLoaderClass *loader_class = SWFDEC_LOADER_CLASS (klass);

  object_class->dispose = swfdec_file_loader_dispose;

  loader_class->load = swfdec_file_loader_load;
}

static void
swfdec_file_loader_init (SwfdecFileLoader *loader)
{
}

/** PUBLIC API ***/

/**
 * swfdec_loader_new_from_file:
 * @filename: name of the file to load
 * @error: return loacation for an error or NULL
 *
 * Creates a new loader for local files.
 *
 * Returns: a new loader on success or NULL on failure
 **/
SwfdecLoader *
swfdec_loader_new_from_file (const char *filename, GError ** error)
{
  SwfdecBuffer *buf;
  SwfdecLoader *loader;

  g_return_val_if_fail (filename != NULL, NULL);

  buf = swfdec_buffer_new_from_file (filename, error);
  if (buf == NULL)
    return NULL;

  loader = g_object_new (SWFDEC_TYPE_FILE_LOADER, NULL);
  if (g_path_is_absolute (filename)) {
    loader->url = g_strdup (filename);
  } else {
    char *cur = g_get_current_dir ();
    loader->url = g_build_filename (cur, filename, NULL);
    g_free (cur);
  }
  SWFDEC_FILE_LOADER (loader)->dir = g_path_get_dirname (loader->url);
  swfdec_loader_push (loader, buf);
  swfdec_loader_eof (loader);
  return loader;
}

/**
 * swfdec_loader_push:
 * @loader: a #SwfdecLoader
 * @buffer: new data to make available. The loader takes the reference
 *          to the buffer.
 *
 * Makes the data in @buffer available to @loader and processes it.
 **/
void
swfdec_loader_push (SwfdecLoader *loader, SwfdecBuffer *buffer)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (loader->eof == FALSE);
  g_return_if_fail (buffer != NULL);

  swfdec_buffer_queue_push (loader->queue, buffer);
  if (loader->target)
    swfdec_root_movie_parse (loader->target);
}

/**
 * swfdec_loader_eof:
 * @loader: a #SwfdecLoader
 *
 * Indicates to @loader that no more data will follow.
 **/
void
swfdec_loader_eof (SwfdecLoader *loader)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (loader->eof == FALSE);

  loader->eof = TRUE;
  if (loader->target)
    swfdec_root_movie_parse (loader->target);
}

SwfdecLoader *
swfdec_loader_load (SwfdecLoader *loader, const char *url)
{
  SwfdecLoaderClass *klass;

  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  klass = SWFDEC_LOADER_GET_CLASS (loader);
  g_return_val_if_fail (klass->load != NULL, NULL);
  return klass->load (loader, url);
}

