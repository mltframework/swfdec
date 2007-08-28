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

/*** general objects ***/

Wrap = function () {};
Wrap.prototype = {};
Wrap.prototype.get = Native.wrap_get;
Wrap.prototype.toString = Native.wrap_toString;

Frame = function () extends Wrap {};
Frame.prototype = new Wrap ();
Frame.prototype.addProperty ("code", Native.frame_code_get, null);
Frame.prototype.addProperty ("name", Native.frame_name_get, null);
Frame.prototype.addProperty ("next", Native.frame_next_get, null);
Frame.prototype.addProperty ("this", Native.frame_this_get, null);

/*** breakpoints ***/

Breakpoint = function () extends Native.Breakpoint {
  super ();
  Breakpoint.list.push (this);
};
Breakpoint.list = new Array ();
Breakpoint.prototype.addProperty ("active", Native.breakpoint_active_get, Native.breakpoint_active_set);
Breakpoint.prototype.toString = function () {
  return "user-defined breakpoint";
};

/*** information about the player ***/

Player = {};
Player.addProperty ("filename", Native.player_filename_get, Native.player_filename_set);
Player.addProperty ("frame", Native.player_frame_get, null);
Player.addProperty ("global", Native.player_global_get, null);
Player.addProperty ("sound", Native.player_sound_get, Native.player_sound_set);
Player.addProperty ("variables", Native.player_variables_get, Native.player_variables_set);

/*** commands available for debugging ***/

Commands = new Object ();
Commands.print = Native.print;
Commands.error = Native.error;
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
/* can't use "break" as a function name, it's a keyword in JS */
Commands.add = function (name) {
  if (name == undefined) {
    Commands.error ("add command requires a function name");
    return undefined;
  }
  var ret = new Breakpoint ();
  ret.onEnterFrame = function (frame) {
    if (frame.name != name)
      return false;

    Commands.print ("Breakpoint: function " + name + " called");
    Commands.print ("  " + frame);
    return true;
  };
  ret.toString = function () {
    return "function call " + name;
  };
  return ret;
};
Commands.list = function () {
  var a = Breakpoint.list;
  var i;
  for (i = 0; i < a.length; i++) {
    Commands.print (i + ": " + a[i]);
  }
};
Commands.del = function (id) {
  var a = Breakpoint.list;
  if (id == undefined) {
    while (a[0])
      Commands.del (0);
  }
  var b = a[id];
  a.splice (id, 1);
  b.active = false;
};
Commands.delete = Commands.del;
Commands.where = function () {
  var frame = Player.frame;
  if (frame == undefined) {
    Commands.print ("---");
    return;
  }
  var i = 0;
  while (frame) {
    Commands.print (i++ + ": " + frame);
    frame = frame.next;
  }
};
Commands.backtrace = Commands.where;
Commands.bt = Commands.where;
Commands.watch = function () {
  var object;
  var name;
  if (arguments.length == 1) {
    name = arguments[0];
  } else if (arguments.length == 2) {
    object = arguments[0];
    name = arguments[1];
  } else {
    Commands.error ("usage: watch [object] name");
    return;
  }
  var ret = new Breakpoint ();
  ret.onSetVariable = function (o, variable, value) {
    if (object && o != object)
      return false;
    if (variable != name)
      return;

    if (object) {
      Commands.print ("Breakpoint: variable " + name + " on " + object + " set to: " + value);
    } else {
      Commands.print ("Breakpoint: variable " + name + " set to: " + value);
    }
    Commands.print ("  " + Player.frame);
    return true;
  };
  ret.toString = function () {
    var s = "watch " + name;
    if (object)
      s += " on " + object;
    return s;
  };
  return ret;
};
