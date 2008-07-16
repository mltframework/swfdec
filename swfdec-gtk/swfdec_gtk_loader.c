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

#include <libsoup/soup.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "swfdec_gtk_loader.h"

/*** GTK-DOC ***/

/**
 * SECTION:GtkExtensions
 * @title: Gtk extensions
 * @short_description: extension objects for @SwfdecGtkPlayer
 *
 * Swfdec-Gtk provides some objects that implement the various extensions
 * provided by the @SwfdecPlayer object. They are used by default in the 
 * #SwfdecGtkPlayer object, but you can use them outside of it, too. 
 * However, they require certain Gtk functionality, such as a running glib
 * main loop.
 */

/**
 * SwfdecGtkLoader:
 *
 * #SwfdecGtkLoader is a #SwfdecLoader that is intended as an easy way to be 
 * access ressources that are not stored in files, such as HTTP. It is a 
 * private object.
 */

struct _SwfdecGtkLoader
{
  SwfdecLoader		loader;

  SoupMessage *		message;	/* the message we're sending */
  gboolean		opened;		/* set after first bytes of data have arrived */
};

struct _SwfdecGtkLoaderClass {
  SwfdecLoaderClass	loader_class;

  SoupSession *		session;	/* the session used by the loader */
};

/*** SwfdecGtkLoader ***/

G_DEFINE_TYPE (SwfdecGtkLoader, swfdec_gtk_loader, SWFDEC_TYPE_FILE_LOADER)

static void
swfdec_gtk_loader_ensure_open (SwfdecGtkLoader *gtk)
{
  char *real_uri;

  if (gtk->opened)
    return;

  real_uri = soup_uri_to_string (soup_message_get_uri (gtk->message), FALSE);
  swfdec_loader_set_url (SWFDEC_LOADER (gtk), real_uri);
  g_free (real_uri);
  if (soup_message_headers_get_encoding (gtk->message->response_headers) == SOUP_ENCODING_CONTENT_LENGTH) {
    swfdec_loader_set_size (SWFDEC_LOADER (gtk), 
	soup_message_headers_get_content_length (gtk->message->response_headers));
  }
  swfdec_stream_open (SWFDEC_STREAM (gtk));
  gtk->opened = TRUE;
}

static void
swfdec_gtk_loader_push (SoupMessage *msg, SoupBuffer *chunk, gpointer loader)
{
  SwfdecGtkLoader *gtk = SWFDEC_GTK_LOADER (loader);
  SwfdecBuffer *buffer;

  chunk = soup_buffer_copy (chunk);

  swfdec_gtk_loader_ensure_open (gtk);
  buffer = swfdec_buffer_new_full ((unsigned char *) chunk->data, chunk->length,
      (SwfdecBufferFreeFunc) soup_buffer_free, chunk);
  swfdec_stream_push (loader, buffer);
}
  
static void
swfdec_gtk_loader_finished (SoupMessage *msg, gpointer loader)
{
  if (SOUP_STATUS_IS_SUCCESSFUL (msg->status_code)) {
    swfdec_gtk_loader_ensure_open (loader);
    swfdec_stream_close (loader);
  } else {
    if (!SWFDEC_GTK_LOADER (loader)->opened) {
      char *real_uri = soup_uri_to_string (soup_message_get_uri (msg), FALSE);
      swfdec_loader_set_url (loader, real_uri);
      g_free (real_uri);
    }
    swfdec_stream_error (loader, "%u %s", msg->status_code, msg->reason_phrase);
  }
}

static void
swfdec_gtk_loader_dispose (GObject *object)
{
  SwfdecGtkLoader *gtk = SWFDEC_GTK_LOADER (object);

  if (gtk->message) {
    g_signal_handlers_disconnect_by_func (gtk->message, swfdec_gtk_loader_push, gtk);
    g_signal_handlers_disconnect_by_func (gtk->message, swfdec_gtk_loader_finished, gtk);
    g_object_unref (gtk->message);
    gtk->message = NULL;
  }

  G_OBJECT_CLASS (swfdec_gtk_loader_parent_class)->dispose (object);
}

static void
swfdec_gtk_loader_load (SwfdecLoader *loader, SwfdecPlayer *player, 
    const char *url_string, SwfdecBuffer *buffer, guint n_headers,
    const char **header_names, const char **header_values)
{
  SwfdecURL *url;
  guint i;
  
  if (swfdec_url_path_is_relative (url_string)) {
    url = swfdec_url_new_relative (swfdec_player_get_base_url (player), url_string);
  } else {
    url = swfdec_url_new (url_string);
  }

  if (url == NULL) {
    swfdec_stream_error (SWFDEC_STREAM (loader), "invalid URL %s", url_string);
    return;
  };
  if (!swfdec_url_has_protocol (url, "http") &&
      !swfdec_url_has_protocol (url, "https")) {
    SWFDEC_LOADER_CLASS (swfdec_gtk_loader_parent_class)->load (loader, player,
	url_string, buffer, n_headers, header_names, header_values);
  } else {
    SwfdecGtkLoader *gtk = SWFDEC_GTK_LOADER (loader);
    SwfdecGtkLoaderClass *klass = SWFDEC_GTK_LOADER_GET_CLASS (gtk);

    gtk->message = soup_message_new (buffer != NULL ? "POST" : "GET",
	swfdec_url_get_url (url));
    soup_message_set_flags (gtk->message, SOUP_MESSAGE_OVERWRITE_CHUNKS);

    for (i = 0; i < n_headers; i++) {
      soup_message_headers_append (gtk->message->request_headers,
	  header_names[i], header_values[i]);
    }

    g_signal_connect (gtk->message, "got-chunk", G_CALLBACK (swfdec_gtk_loader_push), gtk);
    g_signal_connect (gtk->message, "finished", G_CALLBACK (swfdec_gtk_loader_finished), gtk);
    if (buffer)
      soup_message_set_request (gtk->message, "appliation/x-www-form-urlencoded",
	  SOUP_MEMORY_COPY, (char *) buffer->data, buffer->length);
    g_object_ref (gtk->message);
    soup_session_queue_message (klass->session, gtk->message, NULL, NULL);
  }
  swfdec_url_free (url);
}

static void
swfdec_gtk_loader_close (SwfdecStream *stream)
{
  SwfdecGtkLoader *gtk = SWFDEC_GTK_LOADER (stream);

  if (gtk->message && !swfdec_stream_is_complete (stream)) {
    SwfdecGtkLoaderClass *klass = SWFDEC_GTK_LOADER_GET_CLASS (gtk);

    soup_session_cancel_message (klass->session, gtk->message, SOUP_STATUS_CANCELLED);
    g_object_unref (gtk->message);
    gtk->message = NULL;
  }
}

static void
swfdec_gtk_loader_class_init (SwfdecGtkLoaderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecStreamClass *stream_class = SWFDEC_STREAM_CLASS (klass);
  SwfdecLoaderClass *loader_class = SWFDEC_LOADER_CLASS (klass);

  object_class->dispose = swfdec_gtk_loader_dispose;

  stream_class->close = swfdec_gtk_loader_close;

  loader_class->load = swfdec_gtk_loader_load;
  
  klass->session = soup_session_async_new ();
}

static void
swfdec_gtk_loader_init (SwfdecGtkLoader *gtk_loader)
{
}

