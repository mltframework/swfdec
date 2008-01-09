/* Swfdec
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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
#include <libsoup/soup-address.h>
#include <libswfdec/swfdec.h>
#include <glib/gprintf.h>

static void
socket_print (SoupSocket *sock, const char *format, ...)
{
  va_list varargs;

  g_print ("%p ", sock);
  va_start (varargs, format);
  g_vprintf (format, varargs);
  va_end (varargs);
}

static void
hexdump (SwfdecBuffer *buffer)
{
  guint i;

  for (i = 0; i < buffer->length; i++) {
    g_print ("%02X%s", buffer->data[i], i % 8 == 7 ? "\n" : (i % 4 == 3 ? "  " : " "));
  }
  if (i % 8)
    g_print ("\n");
}

static void
do_read (SoupSocket *conn, gpointer unused)
{
  SwfdecBuffer *buf;
  SoupSocketIOStatus status;

  buf = swfdec_buffer_new_and_alloc (16);
  while ((status = soup_socket_read (conn, buf->data, buf->length, &buf->length)) == SOUP_SOCKET_OK) {
    socket_print (conn, "\n");
    hexdump (buf);
    soup_socket_write (conn, buf->data, buf->length, &buf->length);
    buf->length = 16;
  }
  swfdec_buffer_unref (buf);
  socket_print (conn, "status: %u\n", (guint) status);
}

static void
new_connection (SoupSocket *server, SoupSocket *conn, gpointer unused)
{
  g_object_ref (conn);
  socket_print (conn, "new connection\n");
  g_signal_connect (conn, "readable", G_CALLBACK (do_read), NULL);
  do_read (conn, NULL);
}

int
main (int argc, char **argv)
{
  GMainLoop *loop;
  SoupSocket *server;
  SoupAddress *address;

  swfdec_init ();

  address = soup_address_new_any (SOUP_ADDRESS_FAMILY_IPV4, 1935);
  server = soup_socket_new ("non-blocking", TRUE, NULL);
  soup_socket_listen (server, address);
  g_signal_connect (server, "new-connection", G_CALLBACK (new_connection), NULL);
  g_object_unref (address);

  loop = g_main_loop_new (NULL, FALSE);
  g_main_loop_run (loop);
  g_main_loop_unref (loop);

  return 0;
}

