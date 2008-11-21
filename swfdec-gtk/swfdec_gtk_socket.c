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

#include "swfdec_gtk_socket.h"
#include <libsoup/soup.h>
#include <libsoup/soup-address.h>

/*** GTK-DOC ***/

/**
 * SECTION:SwfdecGtkSocket
 * @title: SwfdecGtkSocket
 * @short_description: a socket implementation integrating into the Gtk main loop
 * @see_also: #SwfdecSocket, #SwfdecGtkPlayer
 *
 * #SwfdecGtkSocket is a #SwfdecSocket that is a socket implementation that 
 * uses libsoup and integrates with the Gtk main loop. It is used by default
 * by the #SwfecGtkPlayer.
 */

/**
 * SwfdecGtkSocket:
 *
 * This is the object used to represent a socket. It is completely private.
 */

struct _SwfdecGtkSocket
{
  SwfdecSocket		socket;

  SoupSocket *		sock;		/* libsoup socket we're using */
};

struct _SwfdecGtkSocketClass {
  SwfdecSocketClass	socket_class;
};

/*** SwfdecGtkSocket ***/

G_DEFINE_TYPE (SwfdecGtkSocket, swfdec_gtk_socket, SWFDEC_TYPE_SOCKET)

static void
swfdec_gtk_socket_close (SwfdecStream *stream)
{
  SwfdecGtkSocket *gtk = SWFDEC_GTK_SOCKET (stream);

  soup_socket_disconnect (gtk->sock);
}

static void
swfdec_gtk_socket_do_disconnect (SoupSocket *sock, SwfdecGtkSocket *gtk)
{
  swfdec_stream_close (SWFDEC_STREAM (gtk));
}

static void
swfdec_gtk_socket_do_read (SoupSocket *sock, SwfdecGtkSocket *gtk)
{
#define SWFDEC_GTK_SOCKET_BLOCK_SIZE 1024
  SwfdecBuffer *buffer;
  SoupSocketIOStatus status;
  gsize len;
  GError *error = NULL;

  do {
    buffer = swfdec_buffer_new (SWFDEC_GTK_SOCKET_BLOCK_SIZE);
    status = soup_socket_read (sock, buffer->data,
	SWFDEC_GTK_SOCKET_BLOCK_SIZE, &len, NULL, &error);
    buffer->length = len;
    switch (status) {
      case SOUP_SOCKET_OK:
	swfdec_stream_push (SWFDEC_STREAM (gtk), buffer);
	break;
      case SOUP_SOCKET_WOULD_BLOCK:
      case SOUP_SOCKET_EOF:
	swfdec_buffer_unref (buffer);
	break;
      case SOUP_SOCKET_ERROR:
	swfdec_buffer_unref (buffer);
	swfdec_stream_error (SWFDEC_STREAM (gtk), "%s", error->message);
	g_error_free (error);
	break;
      default:
	g_warning ("unhandled status code %u from soup_socket_read()", (guint) status);
	break;
    }
  } while (status == SOUP_SOCKET_OK);
}

static void
swfdec_gtk_socket_writable (SoupSocket *sock, SwfdecGtkSocket *gtk)
{
  swfdec_socket_signal_writable (SWFDEC_SOCKET (gtk));
}

static void
swfdec_gtk_socket_do_connect (SoupSocket *sock, guint status, gpointer gtk)
{
  if (SOUP_STATUS_IS_SUCCESSFUL (status)) {
    swfdec_stream_open (gtk);
    // need to read here, since readable signal will only be launched once the
    // socket has been read until it's empty once
    swfdec_gtk_socket_do_read (sock, gtk);
  } else {
    swfdec_stream_error (gtk, "error connecting");
  }
}

static void
swfdec_gtk_socket_connect (SwfdecSocket *sock_, SwfdecPlayer *player, 
    const char *hostname, guint port)
{
  SwfdecGtkSocket *sock = SWFDEC_GTK_SOCKET (sock_);
  SoupAddress *addr;

  addr = soup_address_new (hostname, port);
  sock->sock = soup_socket_new (
      SOUP_SOCKET_FLAG_NONBLOCKING, TRUE,
      SOUP_SOCKET_REMOTE_ADDRESS, addr, NULL);
  g_signal_connect (sock->sock, "disconnected", 
      G_CALLBACK (swfdec_gtk_socket_do_disconnect), sock);
  g_signal_connect (sock->sock, "readable", 
      G_CALLBACK (swfdec_gtk_socket_do_read), sock);
  g_signal_connect (sock->sock, "writable", 
      G_CALLBACK (swfdec_gtk_socket_writable), sock);
  soup_socket_connect_async (sock->sock, NULL, swfdec_gtk_socket_do_connect, sock);
}

static gsize
swfdec_gtk_socket_send (SwfdecSocket *sock, SwfdecBuffer *buffer)
{
  SwfdecGtkSocket *gtk = SWFDEC_GTK_SOCKET (sock);
  SoupSocketIOStatus status;
  GError *error = NULL;
  gsize len;

  status = soup_socket_write (gtk->sock, buffer->data, buffer->length, 
      &len, NULL, &error);
  switch (status) {
    case SOUP_SOCKET_OK:
    case SOUP_SOCKET_WOULD_BLOCK:
    case SOUP_SOCKET_EOF:
      break;
    case SOUP_SOCKET_ERROR:
      swfdec_stream_error (SWFDEC_STREAM (gtk), "%s", error->message);
      g_error_free (error);
      return 0;
    default:
      g_warning ("unhandled status code %u from soup_socket_read()", (guint) status);
      break;
  }
  return len;
}

static void
swfdec_gtk_socket_dispose (GObject *object)
{
  SwfdecGtkSocket *gtk = SWFDEC_GTK_SOCKET (object);

  if (gtk->sock) {
    g_signal_handlers_disconnect_matched (gtk->sock, G_SIGNAL_MATCH_DATA,
      0, 0, NULL, NULL, gtk);
    g_object_unref (gtk->sock);
    gtk->sock = NULL;
  }

  G_OBJECT_CLASS (swfdec_gtk_socket_parent_class)->dispose (object);
}

static void
swfdec_gtk_socket_class_init (SwfdecGtkSocketClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecStreamClass *stream_class = SWFDEC_STREAM_CLASS (klass);
  SwfdecSocketClass *socket_class = SWFDEC_SOCKET_CLASS (klass);

  object_class->dispose = swfdec_gtk_socket_dispose;

  stream_class->close = swfdec_gtk_socket_close;

  socket_class->connect = swfdec_gtk_socket_connect;
  socket_class->send = swfdec_gtk_socket_send;
}

static void
swfdec_gtk_socket_init (SwfdecGtkSocket *gtk)
{
}

