/* Vivified
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

Commands = new Object ();
Commands.print = Native.print;
Commands.r = Native.run;
Commands.run = Native.run;
Commands.halt = Native.stop;
Commands.stop = Native.stop;
Commands.s = Native.step;
Commands.step = Native.step;
Commands.reset = Native.reset;
Commands.restart = function () {
  Commands.reset ();
  Commands.run ();
};
Commands.quit = Native.quit;

Breakpoint = Native.Breakpoint;

Wrap = function () {};
Wrap.prototype = {};
Wrap.prototype.toString = Native.wrap_toString;

Frame = function () extends Wrap {};
Frame.prototype = new Wrap ();
Frame.prototype.addProperty ("name", Native.frame_name_get, null);

