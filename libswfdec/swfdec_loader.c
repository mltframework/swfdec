/* Swfdec
 * Copyright (C) 2006-2007 Benjamin Otte <otte@gnome.org>
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
#include "swfdec_loader_internal.h"
#include "swfdec_buffer.h"
#include "swfdec_debug.h"
#include "swfdec_loadertarget.h"
#include "swfdec_player_internal.h"

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

/**
 * SwfdecLoaderDataType:
 * @SWFDEC_LOADER_DATA_UNKNOWN: Unidentified data or data that cannot be 
 *                              identified.
 * @SWFDEC_LOADER_DATA_SWF: Data describing a normal Flash file.
 * @SWFDEC_LOADER_DATA_FLV: Data describing a Flash video stream.
 * @SWFDEC_LOADER_DATA_XML: Data in XML format.
 * @SWFDEC_LOADER_DATA_TEXT: Textual data.
 * @SWFDEC_LOADER_DATA_JPEG: a JPEG image
 * @SWFDEC_LOADER_DATA_PNG: a PNG image
 *
 * This type describes the different types of data that can be loaded inside 
 * Swfdec. Swfdec identifies its data streams and you can use the 
 * swfdec_loader_get_data_type() to acquire more information about the data
 * inside a #SwfdecLoader.
 */

/**
 * SwfdecLoaderRequest:
 * @SWFDEC_LOADER_REQUEST_DEFAULT: Use the default method (this most likely is 
 *                                 equal to HTTPget)
 * @SWFDEC_LOADER_REQUEST_GET: Use HTTP get
 * @SWFDEC_LOADER_REQUEST_POST: Use HTTP post
 *
 * Describes the moethod to use for requesting a given URL. These methods map
 * naturally to HTTP methods, since HTTP is the common method for requesting 
 * Flash content.
 */

/*** SwfdecLoader ***/

enum {
  PROP_0,
  PROP_ERROR,
  PROP_EOF,
  PROP_DATA_TYPE,
  PROP_SIZE,
  PROP_LOADED,
  PROP_URL
};

G_DEFINE_ABSTRACT_TYPE (SwfdecLoader, swfdec_loader, G_TYPE_OBJECT)

static void
swfdec_loader_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecLoader *loader = SWFDEC_LOADER (object);
  
  switch (param_id) {
    case PROP_ERROR:
      g_value_set_string (value, loader->error);
      break;
    case PROP_EOF:
      g_value_set_boolean (value, loader->state == SWFDEC_LOADER_STATE_EOF);
      break;
    case PROP_DATA_TYPE:
      g_value_set_enum (value, loader->data_type);
      break;
    case PROP_SIZE:
      g_value_set_long (value, loader->size);
      break;
    case PROP_LOADED:
      g_value_set_ulong (value, swfdec_loader_get_loaded (loader));
      break;
    case PROP_URL:
      g_value_set_boxed (value, loader->url);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_loader_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  SwfdecLoader *loader = SWFDEC_LOADER (object);

  switch (param_id) {
    case PROP_ERROR:
      swfdec_loader_error (loader, g_value_get_string (value));
      break;
    case PROP_SIZE:
      if (loader->size == -1 && g_value_get_long (value) >= 0)
	swfdec_loader_set_size (loader, g_value_get_long (value));
      break;
    case PROP_URL:
      loader->url = g_value_dup_boxed (value);
      if (loader->url == NULL) {
	g_warning ("must set a valid URL");
	loader->url = swfdec_url_new ("");
      }
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_loader_dispose (GObject *object)
{
  SwfdecLoader *loader = SWFDEC_LOADER (object);

  /* targets are supposed to keep a reference around */
  g_assert (loader->target == NULL);
  swfdec_buffer_queue_unref (loader->queue);
  swfdec_url_free (loader->url);
  g_free (loader->error);

  G_OBJECT_CLASS (swfdec_loader_parent_class)->dispose (object);
}

static void
swfdec_loader_class_init (SwfdecLoaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_loader_dispose;
  object_class->get_property = swfdec_loader_get_property;
  object_class->set_property = swfdec_loader_set_property;

  g_object_class_install_property (object_class, PROP_ERROR,
      g_param_spec_string ("error", "error", "NULL when no error or string describing error",
	  NULL, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_EOF,
      g_param_spec_boolean ("eof", "eof", "TRUE when all data has been handed to the loader",
	  FALSE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_DATA_TYPE,
      g_param_spec_enum ("data-type", "data type", "the data's type as identified by Swfdec",
	  SWFDEC_TYPE_LOADER_DATA_TYPE, SWFDEC_LOADER_DATA_UNKNOWN, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_SIZE,
      g_param_spec_long ("size", "size", "amount of bytes in loader",
	  -1, G_MAXLONG, -1, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_LOADED,
      g_param_spec_ulong ("loaded", "loaded", "bytes already loaded",
	  0, G_MAXULONG, 0, G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_URL,
      g_param_spec_boxed ("url", "url", "URL for this file",
	  SWFDEC_TYPE_URL, G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY));
}

static void
swfdec_loader_init (SwfdecLoader *loader)
{
  loader->queue = swfdec_buffer_queue_new ();
  loader->data_type = SWFDEC_LOADER_DATA_UNKNOWN;

  loader->size = -1;
}

/*** INTERNAL API ***/

static void
swfdec_loader_process (gpointer loaderp, gpointer unused)
{
  SwfdecLoader *loader = loaderp;

  g_assert (loader->target != NULL);

  loader->queued = FALSE;
  if (loader->state == loader->processed_state)
    return;
  g_assert (loader->state != SWFDEC_LOADER_STATE_CLOSED);
  if (loader->state == SWFDEC_LOADER_STATE_ERROR) {
    swfdec_loader_target_error (loader->target, loader);
    return;
  }
  g_object_ref (loader);
  while (loader->state != loader->processed_state) {
    if (loader->processed_state == SWFDEC_LOADER_STATE_NEW) {
      loader->processed_state = SWFDEC_LOADER_STATE_OPEN;
      swfdec_loader_target_open (loader->target, loader);
    } else if (loader->processed_state == SWFDEC_LOADER_STATE_OPEN) {
      loader->processed_state = SWFDEC_LOADER_STATE_READING;
      swfdec_loader_target_parse (loader->target, loader);
    } else if (loader->processed_state == SWFDEC_LOADER_STATE_READING) {
      loader->processed_state = SWFDEC_LOADER_STATE_EOF;
      swfdec_loader_target_eof (loader->target, loader);
    }
    /* stupid reentrancy */
    if (loader->processed_state == SWFDEC_LOADER_STATE_NEW)
      break;
  }
  g_object_unref (loader);
}

static void
swfdec_loader_queue_processing (SwfdecLoader *loader)
{
  if (loader->queued)
    return;
  loader->queued = TRUE;
  if (loader->target) {
    g_assert (loader->player);
    swfdec_player_add_external_action (loader->player, loader,
	swfdec_loader_process, NULL);
  }
}

SwfdecLoader *
swfdec_loader_load (SwfdecLoader *loader, const SwfdecURL *url,
    SwfdecLoaderRequest request, const char *data, gsize data_len)
{
  SwfdecLoader *ret;
  SwfdecLoaderClass *klass;

  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);
  g_return_val_if_fail (url != NULL, NULL);
  g_return_val_if_fail (data != NULL || data_len == 0, NULL);

  klass = SWFDEC_LOADER_GET_CLASS (loader);
  g_return_val_if_fail (klass->load != NULL, NULL);
  ret = g_object_new (G_OBJECT_CLASS_TYPE (klass), "url", url, NULL);
  klass->load (ret, loader, request, data, data_len);
  return ret;
}

void
swfdec_loader_close (SwfdecLoader *loader)
{
  SwfdecLoaderClass *klass;

  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  klass = SWFDEC_LOADER_GET_CLASS (loader);
  
  if (klass->close)
    klass->close (loader);
  if (loader->state != SWFDEC_LOADER_STATE_ERROR) {
    loader->state = SWFDEC_LOADER_STATE_CLOSED;
    loader->processed_state = SWFDEC_LOADER_STATE_CLOSED;
  }
}

void
swfdec_loader_set_target (SwfdecLoader *loader, SwfdecLoaderTarget *target)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (target == NULL || SWFDEC_IS_LOADER_TARGET (target));

  if (loader->target) {
    swfdec_player_remove_all_external_actions (loader->player, loader);
  }
  loader->queued = FALSE;
  loader->target = target;
  loader->processed_state = SWFDEC_LOADER_STATE_NEW;
  if (target) {
    loader->player = swfdec_loader_target_get_player (target);
    swfdec_loader_queue_processing (loader);
  } else {
    loader->player = NULL;
  }
}

/** PUBLIC API ***/

/**
 * swfdec_loader_error:
 * @loader: a #SwfdecLoader
 * @error: a string describing the error
 *
 * Moves the loader in the error state if it wasn't before. A loader that is in
 * the error state will not process any more data. Also, internal error 
 * handling scripts may be executed.
 **/
void
swfdec_loader_error (SwfdecLoader *loader, const char *error)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (error != NULL);

  if (loader->error) {
    SWFDEC_ERROR ("another error in loader %p: %s", loader, error);
    return;
  }

  SWFDEC_ERROR ("error in loader %p: %s", loader, error);
  loader->state = SWFDEC_LOADER_STATE_ERROR;
  loader->error = g_strdup (error);
  swfdec_loader_queue_processing (loader);
}

/**
 * swfdec_loader_open:
 * @loader: a #SwfdecLoader
 * @url: the real URL used for this loader if it has changed (e.g. after HTTP 
 *       redirects) or %NULL if it hasn't changed
 *
 * Call this function when your loader opened the resulting file. For HTTP this
 * is when having received the headers. You must call this function before 
 * swfdec_laoder_push() can be called.
 **/
void
swfdec_loader_open (SwfdecLoader *loader, const char *url)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (loader->state == SWFDEC_LOADER_STATE_NEW);

  loader->state = SWFDEC_LOADER_STATE_OPEN;
  if (url) {
    swfdec_url_free (loader->url);
    loader->url = swfdec_url_new (url);
    g_object_notify (G_OBJECT (loader), "url");
  }
  swfdec_loader_queue_processing (loader);
}

/**
 * swfdec_loader_push:
 * @loader: a #SwfdecLoader
 * @buffer: new data to make available. The loader takes the reference
 *          to the buffer.
 *
 * Makes the data in @buffer available to @loader and processes it. The @loader
 * must be open.
 **/
void
swfdec_loader_push (SwfdecLoader *loader, SwfdecBuffer *buffer)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (loader->state == SWFDEC_LOADER_STATE_OPEN || loader->state == SWFDEC_LOADER_STATE_READING);
  g_return_if_fail (buffer != NULL);

  swfdec_buffer_queue_push (loader->queue, buffer);
  g_object_notify (G_OBJECT (loader), "loaded");
  loader->state = SWFDEC_LOADER_STATE_READING;
  if (loader->processed_state == SWFDEC_LOADER_STATE_READING)
    loader->processed_state = SWFDEC_LOADER_STATE_OPEN;
  swfdec_loader_queue_processing (loader);
}

/**
 * swfdec_loader_eof:
 * @loader: a #SwfdecLoader
 *
 * Indicates to @loader that no more data will follow. The loader must be open.
 **/
void
swfdec_loader_eof (SwfdecLoader *loader)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (loader->state == SWFDEC_LOADER_STATE_OPEN || loader->state == SWFDEC_LOADER_STATE_READING);

  if (loader->size == 0) {
    gulong bytes = swfdec_loader_get_loaded (loader);
    if (bytes)
      swfdec_loader_set_size (loader, bytes);
  }
  g_object_notify (G_OBJECT (loader), "eof");
  loader->state = SWFDEC_LOADER_STATE_EOF;
  swfdec_loader_queue_processing (loader);
}

/**
 * swfdec_loader_get_filename:
 * @loader: a #SwfdecLoader
 *
 * Gets the suggested filename to use for this loader. This may be of interest
 * when displaying information about the file that is played back.
 *
 * Returns: A string in the glib filename encoding that contains the filename
 *          for this loader. g_free() after use.
 **/
char *
swfdec_loader_get_filename (SwfdecLoader *loader)
{
  const SwfdecURL *url;
  const char *path, *ext;
  char *ret = NULL;

  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);

  url = swfdec_loader_get_url (loader);
  path = swfdec_url_get_path (url);
  if (path) {
    char *s = strrchr (path, '/');
    if (s)
      path = s + 1;
    if (path[0] == 0)
      path = NULL;
  }
  if (path)
    ret = g_filename_from_utf8 (path, -1, NULL, NULL, NULL);
  if (ret == NULL)
    ret = g_strdup ("unknown");

  ext = swfdec_loader_data_type_get_extension (loader->data_type);
  if (*ext) {
    char *dot = strrchr (ret, '.');
    char *real;
    guint len = dot ? strlen (dot) : G_MAXUINT;
    if (len <= 5)
      *dot = '\0';
    real = g_strdup_printf ("%s.%s", ret, ext);
    g_free (ret);
    ret = real;
  }

  return ret;
}

/**
 * swfdec_loader_get_url:
 * @loader: a #SwfdecLoader
 *
 * Gets the url this loader is handling. This is mostly useful for writing 
 * subclasses of #SwfdecLoader.
 *
 * Returns: a #SwfdecURL describing @loader.
 **/
const SwfdecURL *
swfdec_loader_get_url (SwfdecLoader *loader)
{
  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);

  return loader->url;
}

/**
 * swfdec_loader_get_data_type:
 * @loader: a #SwfdecLoader
 *
 * Queries the type of data this loader provides. The type is determined 
 * automatically by Swfdec.
 *
 * Returns: the type this data was identified to be in or 
 *          #SWFDEC_LOADER_DATA_UNKNOWN if not identified
 **/
SwfdecLoaderDataType
swfdec_loader_get_data_type (SwfdecLoader *loader)
{
  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), SWFDEC_LOADER_DATA_UNKNOWN);

  return loader->data_type;
}

void
swfdec_loader_set_data_type (SwfdecLoader *loader, SwfdecLoaderDataType	type)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (loader->data_type == SWFDEC_LOADER_DATA_UNKNOWN);
  g_return_if_fail (type != SWFDEC_LOADER_DATA_UNKNOWN);

  loader->data_type = type;
  g_object_notify (G_OBJECT (loader), "data-type");
}

/**
 * swfdec_loader_set_size:
 * @loader: a #SwfdecLoader
 * @size: the amount of bytes in this loader
 *
 * Sets the size of bytes in this loader. This function may only be called once.
 **/
void
swfdec_loader_set_size (SwfdecLoader *loader, gulong size)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (loader->size == -1);
  g_return_if_fail (size <= G_MAXLONG);

  loader->size = size;
  g_object_notify (G_OBJECT (loader), "size");
}

/**
 * swfdec_loader_get_size:
 * @loader: a #SwfdecLoader
 *
 * Queries the amount of bytes inside @loader. If the size is unknown, -1 is 
 * returned. Otherwise the number is greater or equal to 0.
 *
 * Returns: the total number of bytes for this loader or -1 if unknown
 **/
glong
swfdec_loader_get_size (SwfdecLoader *loader)
{
  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), -1);

  return loader->size;
}

/**
 * swfdec_loader_get_loaded:
 * @loader: a #SwfdecLoader
 *
 * Gets the amount of bytes that have already been pushed into @loader and are
 * available to Swfdec.
 *
 * Returns: Amount of bytes in @loader
 **/
gulong
swfdec_loader_get_loaded (SwfdecLoader *loader)
{
  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), 0);

  return swfdec_buffer_queue_get_depth (loader->queue) + 
    swfdec_buffer_queue_get_offset (loader->queue);
}

/**
 * swfdec_loader_data_type_get_extension:
 * @type: a #SwfdecLoaderDataType
 *
 * Queries the extension to be used for data of the given @type.
 *
 * Returns: the typical extension for this data type or the empty string
 *          if the type has no extension
 **/
const char *
swfdec_loader_data_type_get_extension (SwfdecLoaderDataType type)
{
  switch (type) {
    case SWFDEC_LOADER_DATA_UNKNOWN:
      return "";
    case SWFDEC_LOADER_DATA_SWF:
      return "swf";
    case SWFDEC_LOADER_DATA_FLV:
      return "flv";
    case SWFDEC_LOADER_DATA_XML:
      return "xml";
    case SWFDEC_LOADER_DATA_TEXT:
      return "txt";
    case SWFDEC_LOADER_DATA_JPEG:
      return "jpg";
    case SWFDEC_LOADER_DATA_PNG:
      return "png";
    default:
      g_warning ("unknown data type %u", type);
      return "";
  }
}

typedef struct {
  const char		*name;
  guint			length;
  guchar		data[4];
} ByteOrderMark;

static ByteOrderMark boms[] = {
  { "UTF-8", 3, {0xEF, 0xBB, 0xBF, 0} },
  { "UTF-16BE", 2, {0xFE, 0xFF, 0, 0} },
  { "UTF-16LE", 2, {0xFF, 0xFE, 0, 0} },
  { "UTF-8", 0, {0, 0, 0, 0} }
};

char *
swfdec_loader_get_text (SwfdecLoader *loader, guint version)
{
  SwfdecBuffer *buffer;
  char *raw, *text;
  guint size, i, j;

  /* get the text from the loader */
  size = swfdec_buffer_queue_get_depth (loader->queue);
  if (size == 0) {
    SWFDEC_LOG ("empty loader, returning empty string");
    return g_strdup ("");
  }
  raw = g_try_malloc (size + 1);
  if (!raw) {
    SWFDEC_ERROR ("not enough memory to copy %u bytes", size);
    swfdec_buffer_queue_clear (loader->queue);
    return NULL;
  }

  buffer = swfdec_buffer_queue_peek (loader->queue, size);
  if (!buffer)
    return NULL;

  if (version > 5) {
    for (i = 0; boms[i].length > 0; i++) {
      // FIXME: test what happens if we have BOM and nothing else
      if (size < boms[i].length)
	continue;

      for (j = 0; j < boms[i].length; j++) {
	if (buffer->data[j] != boms[i].data[j])
	  break;
      }
      if (j == boms[i].length)
	break;
    }

    if (!strcmp (boms[i].name, "UTF-8")) {
      if (!g_utf8_validate ((char *)buffer->data + boms[i].length,
	    size - boms[i].length, NULL)) {
	SWFDEC_ERROR ("downloaded data is not valid UTF-8");
	text = NULL;
      } else {
	text =
	  g_strndup ((char *)buffer->data + boms[i].length,
	      size - boms[i].length);
      }
    } else {
      text = g_convert ((char *)buffer->data + boms[i].length,
	  size - boms[i].length, "UTF-8", boms[i].name, NULL, NULL, NULL);
      if (text == NULL)
	SWFDEC_ERROR ("downloaded data is not valid %s", boms[i].name);
    }
  } else {
    text = g_convert ((char *)buffer->data, size, "UTF-8", "LATIN1", NULL,
	NULL, NULL);
    if (text == NULL)
      SWFDEC_ERROR ("downloaded data is not valid LATIN1");
  }

  swfdec_buffer_unref (buffer);

  return text;
}

/*** X-WWW-FORM-URLENCODED ***/

/* if speed ever gets an issue, use a 256 byte array instead of strchr */
static const char *urlencode_unescaped="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.,:/()'";
static void
swfdec_urlencode_append_string (GString *str, const char *s)
{
  g_assert (s != NULL);
  while (*s) {
    if (strchr (urlencode_unescaped, *s))
      g_string_append_c (str, *s);
    else if (*s == ' ')
      g_string_append_c (str, '+');
    else
      g_string_append_printf (str, "%%%02X", (guint) *s);
    s++;
  }
}

/**
 * swfdec_string_append_urlencoded:
 * @str: a #GString
 * @name: name of the property to append
 * @value: value of property to append or NULL for empty
 *
 * Appends a name/value pair in encoded as 'application/x-www-form-urlencoded' 
 * to the given @str
 **/
void
swfdec_string_append_urlencoded (GString *str, const char *name, const char *value)
{
  g_return_if_fail (str != NULL);
  g_return_if_fail (name != NULL);

  if (str->len > 0)
    g_string_append_c (str, '&');
  swfdec_urlencode_append_string (str, name);
  g_string_append_c (str, '=');
  if (value)
    swfdec_urlencode_append_string (str, value);
}

