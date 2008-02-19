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

Buffer = Native.Buffer;
Buffer.load = Native.Buffer_load;
Buffer.prototype = {};
Buffer.prototype.diff = Native.Buffer_diff;
Buffer.prototype.find = Native.Buffer_find;
Buffer.prototype.sub = Native.Buffer_sub;
Buffer.prototype.toString = Native.Buffer_toString;

Image = Native.Image;
Image.prototype = {};
Image.prototype.compare = Native.Image_compare;
Image.prototype.save = Native.Image_save;

HTTPServer = Native.HTTPServer;
HTTPServer.prototype = {};
HTTPServer.prototype.getRequest = Native.HTTPServer_getRequest;
HTTPServer.prototype.addProperty ("port", Native.HTTPServer_get_port, null);

HTTPRequest = new Object ();
HTTPRequest.prototype = {};
HTTPRequest.prototype.addProperty ("server", Native.HTTPRequest_get_server, null);
HTTPRequest.prototype.addProperty ("url", Native.HTTPRequest_get_url, null);
HTTPRequest.prototype.addProperty ("path", Native.HTTPRequest_get_path, null);
HTTPRequest.prototype.addProperty ("headers", Native.HTTPRequest_get_headers, null);
HTTPRequest.prototype.addProperty ("contentType", Native.HTTPRequest_get_contentType, Native.HTTPRequest_set_contentType);
HTTPRequest.prototype.addProperty ("statusCode", Native.HTTPRequest_get_statusCode, Native.HTTPRequest_set_statusCode);
HTTPRequest.prototype.toString = Native.HTTPRequest_toString;
HTTPRequest.prototype.send = Native.HTTPRequest_send;
HTTPRequest.prototype.close = Native.HTTPRequest_close;

Socket = function () {};
Socket.prototype = {};
Socket.prototype.close = Native.Socket_close;
Socket.prototype.error = Native.Socket_error;
Socket.prototype.receive = Native.Socket_receive;
Socket.prototype.send = Native.Socket_send;
Socket.prototype.addProperty ("closed", Native.Socket_get_closed, null);

Test = Native.Test;
Test.prototype = {};
Test.prototype.advance = Native.Test_advance;
Test.prototype.getSocket = Native.Test_getSocket;
Test.prototype.mouse_move = Native.Test_mouse_move;
Test.prototype.mouse_press = Native.Test_mouse_press;
Test.prototype.mouse_release = Native.Test_mouse_release;
Test.prototype.render = Native.Test_render;
Test.prototype.reset = Native.Test_reset;
Test.prototype.addProperty ("rate", Native.Test_get_rate, null);
Test.prototype.addProperty ("quit", Native.Test_get_quit, null);
Test.prototype.addProperty ("trace", Native.Test_get_trace, null);
Test.prototype.addProperty ("launched", Native.Test_get_launched, null);

print = function (s) {
  if (s)
    Native.print ("INFO: " + s);
};
error = function (s) {
  if (s)
    Native.print ("ERROR: " + s);
};
