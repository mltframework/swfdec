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

/*** SwfdecLoader ***/

enum {
  PROP_0,
  PROP_ERROR,
  PROP_EOF,
  PROP_DATA_TYPE
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
      g_value_set_boolean (value, loader->eof);
      break;
    case PROP_DATA_TYPE:
      g_value_set_enum (value, loader->data_type);
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
    case PROP_EOF:
      if (g_value_get_boolean (value) && !loader->eof)
	swfdec_loader_eof (loader);
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

  swfdec_buffer_queue_free (loader->queue);
  g_free (loader->url);
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
}

static void
swfdec_loader_init (SwfdecLoader *loader)
{
  loader->queue = swfdec_buffer_queue_new ();
  loader->data_type = SWFDEC_LOADER_DATA_UNKNOWN;
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
    SWFDEC_ERROR ("\"%s\" is an absolute path - using relative instead", url);
    while (*url == G_DIR_SEPARATOR)
      url++;
  }

  /* FIXME: need to rework seperators on windows? */
  real_path = g_build_filename (SWFDEC_FILE_LOADER (loader)->dir, url, NULL);
  buffer = swfdec_buffer_new_from_file (real_path, &error);
  ret = g_object_new (SWFDEC_TYPE_FILE_LOADER, NULL);
  ret->url = real_path;
  SWFDEC_FILE_LOADER (ret)->dir = g_strdup (SWFDEC_FILE_LOADER (loader)->dir);
  if (buffer == NULL) {
    swfdec_loader_error (ret, error->message);
    g_error_free (error);
  } else {
    swfdec_loader_push (ret, buffer);
    swfdec_loader_eof (ret);
  }

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

/*** INTERNAL API ***/

SwfdecLoader *
swfdec_loader_load (SwfdecLoader *loader, const char *url)
{
  SwfdecLoader *ret;
  SwfdecLoaderClass *klass;

  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);
  g_return_val_if_fail (url != NULL, NULL);

  klass = SWFDEC_LOADER_GET_CLASS (loader);
  g_return_val_if_fail (klass->load != NULL, NULL);
  ret = klass->load (loader, url);
  g_assert (ret != NULL);
  return ret;
}

void
swfdec_loader_set_target (SwfdecLoader *loader, SwfdecLoaderTarget *target)
{
  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (target == NULL || SWFDEC_IS_LOADER_TARGET (target));

  loader->target = target;
}

static void
swfdec_loader_do_parse (gpointer empty, gpointer loaderp)
{
  SwfdecLoader *loader = SWFDEC_LOADER (loaderp);

  swfdec_loader_target_parse (loader->target, loader);
}

void
swfdec_loader_queue_parse (SwfdecLoader *loader)
{
  SwfdecPlayer *player;

  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (loader->target != NULL);

  player = swfdec_loader_target_get_player (loader->target);
  /* HACK: using player as action object makes them get auto-removed */
  swfdec_player_add_action (player, player, swfdec_loader_do_parse, loader);
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
  SwfdecPlayer *player;

  g_return_if_fail (SWFDEC_IS_LOADER (loader));
  g_return_if_fail (error != NULL);

  if (loader->error) {
    SWFDEC_ERROR ("another error in loader %p: %s", loader, error);
    return;
  }

  if (loader->target) {
    player = swfdec_loader_target_get_player (loader->target);
    swfdec_player_lock (player);
    swfdec_loader_error_locked (loader, error);
    swfdec_player_perform_actions (player);
    swfdec_player_unlock (player);
  } else {
    swfdec_loader_error_locked (loader, error);
  }
}

void
swfdec_loader_error_locked (SwfdecLoader *loader, const char *error)
{
  if (loader->error)
    return;

  SWFDEC_WARNING ("error in %s %p: %s", G_OBJECT_TYPE_NAME (loader), loader, error);
  loader->error = g_strdup (error);
  g_object_notify (G_OBJECT (loader), "error");
  if (loader->target)
    swfdec_loader_target_parse (loader->target, loader);
}

void
swfdec_loader_parse (SwfdecLoader *loader)
{
  SwfdecPlayer *player;

  if (loader->target == NULL ||
      loader->error)
    return;

  player = swfdec_loader_target_get_player (loader->target);
  swfdec_player_lock (player);
  swfdec_loader_target_parse (loader->target, loader);
  swfdec_player_perform_actions (player);
  swfdec_player_unlock (player);
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
  swfdec_loader_parse (loader);
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
  g_object_notify (G_OBJECT (loader), "eof");
  swfdec_loader_parse (loader);
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
  char *start, *end, *ret;

  g_return_val_if_fail (SWFDEC_IS_LOADER (loader), NULL);
  /* every loader must set this */
  g_return_val_if_fail (loader->url != NULL, NULL);

  end = strchr (loader->url, '?');
  if (end) {
    char *next = NULL;
    do {
      start = next ? next + 1 : loader->url;
      next = strchr (start, '/');
    } while (next != NULL && next < end);
  } else {
    start = strrchr (loader->url, '/');
    if (start == NULL) {
      start = loader->url;
    } else {
      start++;
    }
  }
  ret = g_filename_from_utf8 (start, end ? end - start : -1, NULL, NULL, NULL);
  if (ret) {
    const char *ext;
    
    ext = swfdec_loader_data_type_get_extension (loader->data_type);
    if (*ext) {
      char *dot = strrchr (ret, '.');
      char *real;
      guint len = dot ? strlen (dot) : G_MAXUINT;
      g_print ("ret: %s, dot: %s, ext: %s\n", ret, dot, ext);
      if (len <= 5)
	*dot = '\0';
      real = g_strdup_printf ("%s.%s", ret, ext);
      g_free (ret);
      ret = real;
    }
  } else {
    ret = g_strdup ("unknown file");
  }

  return ret;
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
    default:
      g_warning ("unknown data type %u", type);
      return "";
  }
}

/*** X-WWW-FORM-URLENCODED ***/

/* if speed ever gets an issue, use a 256 byte array instead of strchr */
static const char *urlencode_unescaped="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.:/";
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

static char *
swfdec_urldecode_one_string (const char *s, const char **out)
{
  GString *ret = g_string_new ("");

  while (*s) {
    if (strchr (urlencode_unescaped, *s)) {
      g_string_append_c (ret, *s);
    } else if (*s == '+') {
      g_string_append_c (ret, ' ');
    } else if (*s == '%') {
      guint byte;
      s++;
      if (*s >= '0' && *s <= '9') {
	byte = *s - '0';
      } else if (*s >= 'A' && *s <= 'F') {
	byte = *s - 'A' + 10;
      } else if (*s >= 'a' && *s <= 'f') {
	byte = *s - 'a' + 10;
      } else {
	g_string_free (ret, TRUE);
	*out = s;
	return NULL;
      }
      byte *= 16;
      s++;
      if (*s >= '0' && *s <= '9') {
	byte += *s - '0';
      } else if (*s >= 'A' && *s <= 'F') {
	byte += *s - 'A' + 10;
      } else if (*s >= 'a' && *s <= 'f') {
	byte += *s - 'a' + 10;
      } else {
	g_string_free (ret, TRUE);
	*out = s;
	return NULL;
      }
      g_assert (byte < 256);
      g_string_append_c (ret, byte);
    } else {
      break;
    }
    s++;
  }
  *out = s;
  return g_string_free (ret, FALSE);
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
swfdec_string_append_urlencoded (GString *str, char *name, char *value)
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

/**
 * swfdec_urldecode_one:
 * @string: string in 'application/x-www-form-urlencoded' form
 * @name: pointer that will hold a newly allocated string for the name of the 
 *        parsed property or NULL
 * @value: pointer that will hold a newly allocated string containing the 
 *         value of the parsed property or NULL
 * @end: If not %NULL, on success, pointer to the first byte in @s that was 
 *       not parsed. On failure it will point to the byte causing the problem
 *
 * Tries to parse the given @string into a name/value pair, assuming the string 
 * is in the application/x-www-form-urlencoded format. If the parsing succeeds,
 * @name and @value will contain the parsed values and %TRUE will be returned.
 *
 * Returns: %TRUE if parsing the property succeeded, %FALSE otherwise
 */
gboolean
swfdec_urldecode_one (const char *string, char **name, char **value, const char **end)
{
  char *name_str, *value_str;

  g_return_val_if_fail (string != NULL, FALSE);

  name_str = swfdec_urldecode_one_string (string, &string);
  if (name_str == NULL)
    goto fail;
  if (*string != '=') {
    g_free (name_str);
    goto fail;
  }
  string++;
  value_str = swfdec_urldecode_one_string (string, &string);
  if (value_str == NULL) {
    g_free (name_str);
    goto fail;
  }

  if (name)
    *name = name_str;
  else
    g_free (name_str);
  if (value)
    *value = value_str;
  else
    g_free (value_str);
  if (end)
    *end = string;
  return TRUE;

fail:
  if (name)
    *name = NULL;
  if (value)
    *value = NULL;
  if (end)
    *end = string;
  return FALSE;
}
