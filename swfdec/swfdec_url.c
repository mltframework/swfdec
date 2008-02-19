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

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "swfdec_url.h"
#include "swfdec_debug.h"

/**
 * SECTION:SwfdecURL
 * @title: SwfdecURL
 * @short_description: URL handling in Swfdec
 *
 * SwfdecURL is Swfdec's way of handling URLs. You probably don't need to mess 
 * with this type unless you want to write a #SwfdecLoader. In that case you 
 * will want to use swfdec_loader_get_url() to get its url and then use the 
 * functions in this section to access it.
 *
 * @see_also: #SwfdecLoader
 */

/**
 * SwfdecURL:
 *
 * this is the structure used for URLs. It is a boxed type to glib's type system
 * and it is not reference counted. It is also a static struct in that it cannot
 * be modified after creation.
 */

struct _SwfdecURL {
  char *	url;			/* the complete url */
  char *	protocol;		/* lowercase, http, file, rtmp, ... */
  char *	host;			/* lowercase, can be NULL for files */
  guint		port;			/* can be 0 */
  char *	path;	  		/* can be NULL for root path */
  char *	query;			/* can be NULL */
};

GType
swfdec_url_get_type (void)
{
  static GType type = 0;

  if (!type)
    type = g_boxed_type_register_static ("SwfdecURL", 
       (GBoxedCopyFunc) swfdec_url_copy, (GBoxedFreeFunc) swfdec_url_free);

  return type;
}

/**
 * swfdec_url_new:
 * @string: a valid utf-8 string possibly containing an URL
 *
 * Parses the given string into a URL for use in swfdec.
 *
 * Returns: a new #SwfdecURL or %NULL if the URL was invalid
 **/
SwfdecURL *
swfdec_url_new (const char *string)
{
  SwfdecURL *url;
  char *s;

  g_return_val_if_fail (string != NULL, NULL);

  /* FIXME: error checking? */
  SWFDEC_DEBUG ("new url: %s", string);
  url = g_slice_new0 (SwfdecURL);
  url->url = g_strdup (string);
  s = strstr (string, "://");
  if (s == NULL) {
    SWFDEC_INFO ("URL %s has no protocol", string);
    goto error;
  }
  url->protocol = g_utf8_strdown (string, s - string);
  string = s + 3;
  s = strchr (string, '/');
  if (s != string) {
    char *colon;
    url->host = g_ascii_strdown (string, s ? s - string : -1);
    colon = strrchr (url->host, ':');
    if (colon) {
      *colon = 0;
      errno = 0;
      url->port = strtoul (colon + 1, &colon, 10);
      if (errno || *colon != 0) {
	SWFDEC_INFO ("%s: invalid port number", string);
	goto error;
      }
    }
    if (s == NULL)
      return url;
  }
  string = s + 1;
  s = strchr (string, '?');
  if (s == NULL) {
    url->path = *string ? g_strdup (string) : NULL;
    return url;
  }
  url->path = g_strndup (string, s - string);
  s++;
  if (*s)
    url->query = g_strdup (s);
  return url;

error:
  swfdec_url_free (url);
  return NULL;
}

/**
 * swfdec_url_new_components:
 * @protocol: protocol to use
 * @hostname: hostname or IP address or %NULL
 * @port: port number or 0. Must be 0 if no hostname is given
 * @path: a path or %NULL
 * @query: the query string or %NULL
 *
 * Creates a new URL from the given components.
 *
 * Returns: a new url pointing to the url from the given components
 **/
SwfdecURL *
swfdec_url_new_components (const char *protocol, const char *hostname, 
    guint port, const char *path, const char *query)
{
  GString *str;
  SwfdecURL *url;

  g_return_val_if_fail (protocol != NULL, NULL);
  g_return_val_if_fail (hostname != NULL || port == 0, NULL);
  g_return_val_if_fail (port < 65536, NULL);

  url = g_slice_new0 (SwfdecURL);
  str = g_string_new ("");
  
  /* protocol */
  url->protocol = g_ascii_strdown (protocol, -1);
  g_string_append (str, url->protocol);
  g_string_append (str, "://");

  /* hostname + port */
  if (hostname) {
    url->host = g_ascii_strdown (hostname, -1);
    url->port = port;
    g_string_append (str, url->host);
    if (port) {
      g_string_append_printf (str, ":%u", port);
    }
  }
  g_string_append (str, "/");

  /* path */
  if (path) {
    url->path = g_strdup (path);
    g_string_append (str, path);
  }

  /* query string */
  if (query) {
    url->query = g_strdup (query);
    g_string_append (str, "?");
    g_string_append (str, query);
  }

  url->url = g_string_free (str, FALSE);
  return url;
}

static gboolean
swfdec_url_path_to_parent_path (char *path)
{
  char *last = strrchr (path, '/');
  
  if (last == NULL)
    return FALSE;

  if (last[1] == '\0') {
    last[0] = '\0';
    return swfdec_url_path_to_parent_path (path);
  }

  *last = '\0';
  return TRUE;
}

/**
 * swfdec_url_new_parent:
 * @url: a #SwfdecURL
 *
 * Creates a new url that is the parent of @url. If the given @url has no 
 * parent, a copy of itself is returned.
 *
 * Returns: a new url pointing to the parent of @url or %NULL on failure.
 **/
SwfdecURL *
swfdec_url_new_parent (const SwfdecURL *url)
{
  char *path;
  SwfdecURL *ret;
  
  path = g_strdup (url->path);
  if (path)
    swfdec_url_path_to_parent_path (path);
  ret = swfdec_url_new_components (url->protocol, url->host, url->port,
      path, NULL);
  g_free (path);
  return ret;
}

/**
 * swfdec_url_new_relative:
 * @url: a #SwfdecURL
 * @string: a relative or absolute URL path
 *
 * Parses @string into a new URL. If the given @string is a relative URL, it 
 * uses @url to resolve it to an absolute url; @url must already contain a
 * directory path.
 *
 * Returns: a new #SwfdecURL or %NULL if an error was detected.
 **/
SwfdecURL *
swfdec_url_new_relative (const SwfdecURL *url, const char *string)
{
  SwfdecURL *ret;
  char *path, *query;

  g_return_val_if_fail (url != NULL, NULL);
  g_return_val_if_fail (string != NULL, NULL);

  /* check for full-qualified URL */
  if (strstr (string, "://"))
    return swfdec_url_new (string);

  if (string[0] == '/') {
    /* absolute URL */
    string++;
    query = strchr (string, '?');
    if (query == NULL) {
      path = *string ? g_strdup (string) : NULL;
    } else {
      path = g_strndup (string, query - string);
      query = g_strdup (query + 1);
    }
  } else {
    /* relative URL */
    char *cur = g_strdup (url->path);
    while (g_str_has_prefix (string, "../")) {
      if (!swfdec_url_path_to_parent_path (cur)) {
	g_free (cur);
	return NULL;
      }
      string += 3;
    }
    if (strstr (string, "/../")) {
      g_free (cur);
      return NULL;
    }
    path = g_strconcat (cur, "/", string, NULL);
    g_free (cur);
    cur = path;
    query = strchr (cur, '?');
    if (query == NULL) {
      path = *string ? g_strdup (cur) : NULL;
    } else {
      path = g_strndup (cur, query - cur);
      query = g_strdup (query + 1);
    }
    g_free (cur);
  }
  ret = swfdec_url_new_components (url->protocol, url->host, url->port,
      path, query);
  g_free (path);
  g_free (query);
  return ret;
}

/**
 * swfdec_url_copy:
 * @url: a #SwfdecURL
 *
 * copies the given url.
 *
 * Returns: a new #SwfdecURL
 **/
SwfdecURL *
swfdec_url_copy (const SwfdecURL *url)
{
  SwfdecURL *copy;

  g_return_val_if_fail (url != NULL, NULL);

  copy = g_slice_new0 (SwfdecURL);
  copy->url = g_strdup (url->url);
  copy->protocol = g_strdup (url->protocol);
  copy->host = g_strdup (url->host);
  copy->port = url->port;
  copy->path = g_strdup (url->path);
  copy->query = g_strdup (url->query);

  return copy;
}

/**
 * swfdec_url_free:
 * @url: a #SwfdecURL
 *
 * Frees the URL and its associated ressources.
 **/
void
swfdec_url_free (SwfdecURL *url)
{
  g_return_if_fail (url != NULL);

  g_free (url->url);
  g_free (url->protocol);
  g_free (url->host);
  g_free (url->path);
  g_free (url->query);
  g_slice_free (SwfdecURL, url);
}

/**
 * swfdec_url_get_url:
 * @url: a #SwfdecURL
 *
 * Gets the whole URL.
 *
 * Returns: the complete URL as string
 **/
const char *
swfdec_url_get_url (const SwfdecURL *url)
{
  g_return_val_if_fail (url != NULL, NULL);

  return url->url;
}

/**
 * swfdec_url_get_protocol:
 * @url: a #SwfdecURL
 *
 * Gets the protocol used by this URL, such as "http" or "file".
 *
 * Returns: the protocol used or "error" if the URL is broken
 **/
const char *
swfdec_url_get_protocol (const SwfdecURL *url)
{
  g_return_val_if_fail (url != NULL, NULL);

  if (url->protocol)
    return url->protocol;
  else
    return "error";
}

/**
 * swfdec_url_has_protocol:
 * @url: a url
 * @protocol: protocol name to check for
 *
 * Checks if the given @url references the given @protocol
 *
 * Returns: %TRUE if both protocols match, %FALSE otherwise
 **/
gboolean
swfdec_url_has_protocol (const SwfdecURL *url, const char *protocol)
{
  g_return_val_if_fail (url != NULL, FALSE);
  g_return_val_if_fail (protocol != NULL, FALSE);

  return g_str_equal (url->protocol, protocol);
}

/**
 * swfdec_url_get_host:
 * @url: a #SwfdecURL
 *
 * Gets the host for @url as a lower case string.
 *
 * Returns: the host or %NULL if none (typically for file URLs).
 **/
const char *
swfdec_url_get_host (const SwfdecURL *url)
{
  g_return_val_if_fail (url != NULL, NULL);

  return url->host;
}

/**
 * swfdec_url_get_port:
 * @url: a #SwfdecURL
 *
 * Gets the port number specified by the given @url. If the @url does not 
 * specify a port number, 0 will be returned.
 *
 * Returns: the specified port or 0 if none was given.
 **/
guint
swfdec_url_get_port (const SwfdecURL *url)
{
  g_return_val_if_fail (url != NULL, 0);

  return url->port;
}

/**
 * swfdec_url_get_path:
 * @url: a #SwfdecURL
 *
 * Gets the path associated with @url. If it contains no path, %NULL is 
 * returned.
 * <note>The returned path does not start with a slash. So in particular for 
 * files, you want to prepend the slash yourself.</note>
 *
 * Returns: the path or %NULL if none
 **/
const char *
swfdec_url_get_path (const SwfdecURL *url)
{
  g_return_val_if_fail (url != NULL, NULL);

  return url->path;
}

/**
 * swfdec_url_get_query:
 * @url: a #SwfdecURL
 *
 * Gets the query string associated with @url. If the URL does not have a query
 * string, %NULL is returned.
 *
 * Returns: Query string or %NULL
 **/
const char *
swfdec_url_get_query (const SwfdecURL *url)
{
  g_return_val_if_fail (url != NULL, NULL);

  return url->query;
}

/**
 * swfdec_url_is_parent:
 * @parent: the supposed parent url
 * @child: the supposed child url
 *
 * Checks if the given @parent url is a parent url of the given @child url. The
 * algorithm used is the same as checking policy files if hey apply. If @parent
 * equals @child, %TRUE is returned. This function does not compare query 
 * strings.
 *
 * Returns: %TRUE if @parent is a parent of @child, %FALSE otherwise.
 **/
gboolean
swfdec_url_is_parent (const SwfdecURL *parent, const SwfdecURL *child)
{
  gsize len;

  g_return_val_if_fail (parent != NULL, FALSE);
  g_return_val_if_fail (child != NULL, FALSE);

  if (!g_str_equal (parent->protocol, child->protocol))
    return FALSE;
  if (parent->host == NULL) {
    if (child->host != NULL)
      return FALSE;
  } else {
    if (child->host == NULL || !g_str_equal (parent->host, child->host))
      return FALSE;
  }
  if (parent->port != child->port)
    return FALSE;
  if (parent->path == NULL)
    return TRUE;
  if (child->path == NULL)
    return TRUE;
  len = strlen (parent->path);
  if (strncmp (parent->path, child->path, len) != 0)
    return FALSE;
  return child->path[len] == '\0' || child->path[len] == '/';
}

/**
 * swfdec_url_is_local:
 * @url: the url to check
 *
 * Checks if the given @url references a local resource. Local resources are
 * treated differently by Flash, since they get a higher degree of trust.
 *
 * Returns: %TRUE if the given url is local.
 **/
gboolean
swfdec_url_is_local (const SwfdecURL *url)
{
  g_return_val_if_fail (url != NULL, FALSE);

  /* FIXME: If we ever support gnome-vfs, this might become tricky */
  return swfdec_url_has_protocol (url, "file");
}

/**
 * swfdec_url_equal:
 * @a: a #SwfdecURL
 * @b: a #SwfdecURL
 *
 * Compares the 2 given URLs for equality. 2 URLs are considered equal, when
 * they point to the same resource. This function is intended to be 
 * used together with swfdec_url_hash() in a #GHashtable.
 *
 * Returns: %TRUE if the 2 given urls point to the same resource, %FALSE 
 *          otherwise.
 **/
gboolean
swfdec_url_equal (gconstpointer a, gconstpointer b)
{
  const SwfdecURL *urla = a;
  const SwfdecURL *urlb = b;

  if (!swfdec_url_has_protocol (urla, urlb->protocol))
    return FALSE;

  if (urla->host == NULL) {
    if (urlb->host != NULL)
      return FALSE;
  } else {
    if (urlb->host == NULL || 
	!g_str_equal (urla->host, urlb->host))
      return FALSE;
  }

  if (urla->port != urlb->port)
    return FALSE;

  if (urla->path == NULL) {
    if (urlb->path != NULL)
      return FALSE;
  } else {
    if (urlb->path == NULL || 
	!g_str_equal (urla->path, urlb->path))
      return FALSE;
  }

  /* FIXME: include query strings? */
  if (urla->query == NULL) {
    if (urlb->query != NULL)
      return FALSE;
  } else {
    if (urlb->query == NULL || 
	!g_str_equal (urla->query, urlb->query))
      return FALSE;
  }

  return TRUE;
}

/**
 * swfdec_url_hash:
 * @url: a #SwfdecURL
 *
 * Creates a hash value for the given @url. This function is intended to be 
 * used together with swfdec_url_equal() in a #GHashtable.
 *
 * Returns: a hash value
 **/
guint
swfdec_url_hash (gconstpointer url)
{
  const SwfdecURL *u = url;
  guint ret;

  /* random hash function, feel free to improve it */
  ret = g_str_hash (u->protocol);
  if (u->host)
    ret ^= g_str_hash (u->host);
  ret ^= u->port;
  if (u->path)
    ret ^= g_str_hash (u->path);
  /* queries aren't used often in hashed urls, so ignore them */
  return ret;
}

/**
 * swfdec_url_path_is_relative:
 * @path: a string used to specify a url
 *
 * Checks if the given URL is relative or absolute.
 *
 * Returns: %TRUE if the path is a relative path, %FALSE if it is absolute
 **/
gboolean
swfdec_url_path_is_relative (const char *path)
{
  g_return_val_if_fail (path != NULL, FALSE);

  return strstr (path, "://") == NULL;
}

/**
 * swfdec_url_new_from_input:
 * @input: the input povided
 *
 * Tries to guess the right URL from the given @input. This function is meant 
 * as a utility function helping to convert user input (like command line 
 * arguments) to a URL without requiring the full URL.
 *
 * Returns: a new url best matching the given @input.
 **/
SwfdecURL *
swfdec_url_new_from_input (const char *input)
{
  SwfdecURL *url;

  g_return_val_if_fail (input != NULL, NULL);

  /* if it's a full URL, return it */
  if (!swfdec_url_path_is_relative (input) &&
      (url = swfdec_url_new (input)))
    return url;

  /* FIXME: split at '?' for query? */
  if (g_path_is_absolute (input)) {
    url = swfdec_url_new_components ("file", NULL, 0,
	input[0] == '/' ? &input[1] : &input[0], NULL);
  } else {
    char *absolute, *cur;
    cur = g_get_current_dir ();
    absolute = g_build_filename (cur, input, NULL);
    g_free (cur);
    url = swfdec_url_new_components ("file", NULL, 0,
	absolute, NULL);
    g_free (absolute);
  }

  g_return_val_if_fail (url != NULL, NULL);
  return url;
}

/**
 * swfdec_url_format_for_display:
 * @url: the url to display
 *
 * Creates a string suitable to display the given @url. An example for using
 * this function is to identify a currently playing Flash URL. Use 
 * swfdec_player_get_url() to query the player's URL and then use this function
 * to get a displayable string.
 *
 * Returns: A new string containig a short description for this URL. g_free()
 *          after use.
 **/
char *
swfdec_url_format_for_display (const SwfdecURL *url)
{
  GString *str;

  g_return_val_if_fail (url != NULL, NULL);

  if (swfdec_url_is_local (url)) {
    const char *slash;
    
    if (url->path == NULL)
      return g_strdup ("/");
    slash = strrchr (url->path, '/');
    if (slash && slash[1] != '\0') {
      return g_strdup (slash + 1);
    } else {
      return g_strdup (url->path);
    }
  }
  str = g_string_new (url->protocol);
  g_string_append (str, "://");
  if (url->host)
    g_string_append (str, url->host);
  g_string_append (str, "/");
  if (url->path)
    g_string_append (str, url->path);

  return g_string_free (str, FALSE);
}

