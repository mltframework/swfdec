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
#include <string.h>
#include "swfdec_url.h"
#include "swfdec_debug.h"

/**
 * SECTION:SwfdecURL
 * @title: SwfdecURL
 * @short_description: URL handling in Swfdec
 * @see_also #SwfdecLoader
 *
 * SwfdecURL is Swfdec's way of handling URLs. You probably don't need to mess 
 * with this type unless you want to write a #SwfdecLoader. In that case you 
 * will want to use @swfdec_loader_get_url() to get its url and then use the 
 * functions in this section to access it.
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
  char *	protocol;		/* http, file, rtmp, ... */
  char *	host;			/* can be NULL for files */
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
 * @string: a full-qualified URL encoded in UTF-8
 *
 * Parses the given string into a URL for use in swfdec.
 *
 * Returns: a new #SwfdecURL
 **/
SwfdecURL *
swfdec_url_new (const char *string)
{
  SwfdecURL *url;
  char *s;

  g_return_val_if_fail (string != NULL, NULL);

  g_print ("%s\n", string);
  url = g_slice_new0 (SwfdecURL);
  url->url = g_strdup (string);
  s = strstr (string, "://");
  if (s == NULL) {
    SWFDEC_ERROR ("URL %s has no protocol", string);
    return url;
  }
  url->protocol = g_strndup (string, s - string);
  string = s + 3;
  s = strchr (string, '/');
  if (s == NULL) {
    url->host = g_strdup (string);
    return url;
  }
  if (s != string)
    url->host = g_strndup (string, s - string);
  string = s + 1;
  s = strchr (string, '?');
  if (s == NULL) {
    url->path = *string ? g_strdup (string) : NULL;
    return url;
  }
  url->path = g_strndup (string, s - string);
  s++;
  if (*s)
    url->query = g_strdup (s + 1);
  return url;
}

/**
 * swfdec_url_new_relative:
 * @url: a #SwfdecURL
 * @string: a relative or absolute URL path
 *
 * Parses @string into a new URL. If the given @string is a relative URL, it 
 * uses @url to resolve it to an absolute url.
 *
 * Returns: a new #SwfdecURL or %NULL if an error was detected.
 **/
SwfdecURL *
swfdec_url_new_relative (const SwfdecURL *url, const char *string)
{
  SwfdecURL *ret;
  GString *str;

  g_return_val_if_fail (url != NULL, NULL);
  g_return_val_if_fail (string != NULL, NULL);

  if (strstr (string, "://")) {
    /* full-qualified URL */
    return swfdec_url_new (string);
  }
  str = g_string_new (url->protocol);
  g_string_append (str, "://");
  if (url->host)
    g_string_append (str, url->host);
  if (string[0] == '/') {
    /* absolute URL */
    g_string_append (str, string);
  } else {
    /* relative URL */
    g_string_append (str, "/");
    if (url->path == NULL) {
      g_string_append (str, string);
    } else {
      char *slash;
      slash = strrchr (url->path, '/');
      if (slash == NULL) {
	g_string_append (str, string);
      } else {
	g_string_append_len (str, url->path, slash - url->path + 1); /* append '/', too */
	g_string_append (str, string);
      }
    }
  }
  ret = swfdec_url_new (str->str);
  g_string_free (str, TRUE);
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
 * swfdec_url_get_host:
 * @url: a #SwfdecURL
 *
 * Gets the host for @url. If the host includes a portnumber, it will be present
 * in the returned string.
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


