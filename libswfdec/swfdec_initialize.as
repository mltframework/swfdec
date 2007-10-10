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

/*** GENERAL ***/

ASSetNative = ASnative (4, 0);
ASSetNativeAccessor = ASnative (4, 1);

/*** Object ***/
/* Only Flash extensions here, rest to swfdec_as_initialize.as */

Object.registerClass = ASnative(101, 8);
ASSetPropFlags (Object, null, 7);

/*** ASBROADCASTER ***/

function AsBroadcaster () { };

AsBroadcaster.broadcastMessage = ASnative(101, 12);

AsBroadcaster.addListener = function (x) {
  this.removeListener (x);
  this._listeners.push (x);
  return true;
};

AsBroadcaster.removeListener = function (x) {
  var l = this._listeners;
  var i;

  for (var i = 0; i < l.length; i++) {
    if (l[i] == x) {
      l.splice (i, 1);
      return true;
    }
  }
  return false;
};

AsBroadcaster.initialize = function (o) {
  o.broadcastMessage = ASnative(101, 12);
  o.addListener = AsBroadcaster.addListener;
  o.removeListener = AsBroadcaster.removeListener;
  o._listeners = [];
  ASSetPropFlags(o, "broadcastMessage,addListener,removeListener,_listeners", 131);
};
ASSetPropFlags(AsBroadcaster, null, 131);

Key = { ALT: 18, BACKSPACE: 8, CAPSLOCK: 20, CONTROL: 17, DELETEKEY: 46, 
    DOWN: 40, END: 35, ENTER: 13, ESCAPE: 27, HOME: 36, INSERT: 45, LEFT: 37, 
    PGDN: 34, PGUP: 33, RIGHT: 39, SHIFT: 16, SPACE: 32, TAB: 9, UP: 38 };
ASSetNative(Key, 800, "getAscii,getCode,isDown");
AsBroadcaster.initialize(Key);
ASSetPropFlags(Key, null, 7);

/*** MOUSE ***/

Mouse = { };
Mouse.show = ASnative (5, 0);
Mouse.hide = ASnative (5, 1);
AsBroadcaster.initialize (Mouse);
ASSetPropFlags(Mouse, null, 7);

/*** STAGE ***/

Stage = { };
AsBroadcaster.initialize (Stage);
ASSetNativeAccessor (Stage, 666, "scaleMode,align,width,height", 1);

/*** LOADVARS ***/

function LoadVars () { };

LoadVars.prototype.contentType = "application/x-www-form-urlencoded";

LoadVars.prototype.load = ASnative (301, 0);
//LoadVars.prototype.send = ASnative (301, 1);
//LoadVars.prototype.sendAndLoad = ASnative (301, 2);
LoadVars.prototype.decode = ASnative (301, 3);

LoadVars.prototype.onLoad = function () {
};

LoadVars.prototype.onData = function (src) {
  this.loaded = true;
  if (src != null) {
    this.decode (src);
    this.onLoad (true);
  } else {
    this.onLoad (false);
  }
};

LoadVars.prototype.toString = function () {
  var str = null;
  for (var x in this) {
    if (str == null) {
      str = escape(x) + "=" + escape(this[x]);
    } else {
      str += "&" + escape(x) + "=" + escape(this[x]);
    }
  }
  return str;
};

LoadVars.prototype.getBytesLoaded = function () {
  return this._bytesLoaded;
};

LoadVars.prototype.getBytesTotal = function () {
  return this._bytesTotal;
};

ASSetPropFlags(LoadVars.prototype, null, 131);

/*** Sound ***/

Sound = ASconstructor (500, 16);
ASSetNative (Sound.prototype, 500, "stop,attachSound,start", 6);

/*** XMLNode ***/

XMLNode = ASconstructor (253, 0);

XMLNode.prototype.cloneNode = ASnative (253, 1);
XMLNode.prototype.removeNode = ASnative (253, 2);
XMLNode.prototype.insertBefore = ASnative (253, 3);
XMLNode.prototype.appendChild = ASnative (253, 4);
XMLNode.prototype.hasChildNodes = ASnative (253, 5);
XMLNode.prototype.toString = ASnative (253, 6);
XMLNode.prototype.getNamespaceForPrefix = ASnative (253, 7);
XMLNode.prototype.getPrefixForNamespace = ASnative (253, 8);

/*** XML ***/

XML = ASconstructor (253, 9);

XML.prototype = new XMLNode (1, "");
ASSetPropFlags (XML, "prototype", 3);

XML.prototype.load = ASnative (301, 0);
//XML.prototype.send = ASnative (301, 1);
//XML.prototype.sendAndLoad = ASnative (301, 2);
XML.prototype.createElement = ASnative (253, 10);
XML.prototype.createTextNode = ASnative (253, 11);
XML.prototype.parseXML = ASnative (253, 12);

XML.prototype.onLoad = function () {
};

// Note: handling of loaded is different here than in LoadVars
XML.prototype.onData = function (src) {
  if (src != null) {
    this.loaded = true;
    this.parseXML (src);
    this.onLoad (true);
  } else {
    this.loaded = false;
    this.onLoad (false);
  }
};

XML.prototype.getBytesLoaded = function () {
  return this._bytesLoaded;
};

XML.prototype.getBytesTotal = function () {
  return this._bytesTotal;
};

/*** System ***/

System = {};
System.capabilities = {};
System.capabilities.Query = ASnative (11, 0);
System.capabilities.Query ();
delete System.capabilities.Query;

/*** Color ***/

Color = function (target) {
  this.target = target;
  ASSetPropFlags (this, null, 7);
};
ASSetNative (Color.prototype, 700, "setRGB,setTransform,getRGB,getTransform");
ASSetPropFlags (Color.prototype, null, 7);

/* MovieClip */

MovieClip.prototype.meth = function (method) {
  var lower = method.toLowerCase ();
  if (lower == "post") {
    return 2;
  } else if (lower == "get") {
    return 1;
  } else {
    return 0;
  }
};

MovieClip.prototype.getURL = function (url, target, method) {
  if (typeof (target) == "undefined")
    target = ""; // undefined to empty string, even in version >= 7

  var type = this.meth (method);
  if (type == 0) {
    getURL (url, target);
  } else if (type == 1) {
    getURL (url, target, "GET");
  } else {
    getURL (url, target, "POST");
  }
};

// work around ming bug, causing loadVariables to be lower cased
MovieClip.prototype["loadVariables"] = function (url, method) {
  var type = this.meth (method);
  setTarget (this);
  if (type == 0) {
    loadVariables (url, this._target);
  } else if (type == 1) {
    loadVariables (url, this._target, "GET");
  } else {
    loadVariables (url, this._target, "POST");
  }
  setTarget (null);
};

// work around ming bug, causing loadMovie to be lower cased
MovieClip.prototype["loadMovie"] = function (url, method) {
  var type = this.meth (method);
  setTarget (this);
  if (type == 0) {
    loadMovie (url, this._target);
  } else if (type == 1) {
    loadMovie (url, this._target, "GET");
  } else {
    loadMovie (url, this._target, "POST");
  }
  setTarget (null);
};

MovieClip.prototype.attachMovie = ASnative (900, 0);
MovieClip.prototype.swapDepths = ASnative (900, 1);
MovieClip.prototype.hitTest = ASnative (900, 4);
MovieClip.prototype.getBounds = ASnative (900, 5);
MovieClip.prototype.getBytesTotal = ASnative (900, 6);
MovieClip.prototype.getBytesLoaded = ASnative (900, 7);
MovieClip.prototype.getDepth = ASnative (900, 10);
MovieClip.prototype.play = ASnative (900, 12);
MovieClip.prototype.stop = ASnative (900, 13);
MovieClip.prototype.nextFrame = ASnative (900, 14);
MovieClip.prototype.prevFrame = ASnative (900, 15);
MovieClip.prototype.gotoAndPlay = ASnative (900, 16);
MovieClip.prototype.gotoAndStop = ASnative (900, 17);
// work around ming bug, causing these two to be lower cased
MovieClip.prototype["duplicateMovieClip"] = ASnative (900, 18);
MovieClip.prototype["removeMovieClip"] = ASnative (900, 19);
MovieClip.prototype.startDrag = ASnative (900, 20);
MovieClip.prototype.stopDrag = ASnative (900, 21);
MovieClip.prototype.createEmptyMovieClip = ASnative (901, 0);
MovieClip.prototype.createTextField = ASnative (104, 200);
ASSetPropFlags (MovieClip.prototype, "getDepth,createEmptyMovieClip", 128);

ASSetNative (MovieClip.prototype, 901, "6createEmptyMovieClip,6beginFill,6beginGradientFill,6moveTo,6lineTo,6curveTo,6lineStyle,6endFill,6clear");
ASSetPropFlags (MovieClip.prototype, null, 3);

/* TextField */

TextField = ASconstructor (104, 0);
TextField.prototype.setTextFormat = ASnative (104, 102);
TextField.prototype.getNewTextFormat = ASnative (104, 104);
TextField.prototype.setNewTextFormat = ASnative (104, 105);

/* TextFormat */

TextFormat = ASconstructor (110, 0);

/* TextField.Stylesheet */

TextField.StyleSheet = ASconstructor (113, 0);

TextField.StyleSheet.prototype._copy = function (o) {
  if (typeof (o) != "object")
    return null;

  var o_new = {};
  for (var prop in o) {
    o_new[prop] = o[prop];
  }
  return o_new;
};

TextField.StyleSheet.prototype.clear = function () {
  this._css = {};
  this._styles = {};
};

TextField.StyleSheet.prototype.getStyle = function (name) {
  return (this._copy (this._css[name]));
};

TextField.StyleSheet.prototype.setStyle = function (name, style) {
  if (!this._css)
    this._css = {};

  this._css[name] = this._copy (style);
  this.doTransform (name);
};

TextField.StyleSheet.prototype.getStyleNames = function () {
  var tmp = this._css; /* ming bug? */
  var names = [];
  for (var prop in tmp) {
    names.push (prop);
  }
  return names;
};

TextField.StyleSheet.prototype.doTransform = function (name) {
  if (!this._styles) {
    this._styles = {};
  }
  this._styles[name] = this.transform (this._css[name]);
};

TextField.StyleSheet.prototype.transform = function (style) {
  if (style == null)
    return null;

  var format = new TextFormat ();

  if (style.textAlign)
    format.align = style.textAlign;

  if (style.fontWeight == "bold") {
    format.bold = true;
  } else if (style.fontWeight == "normal") {
    format.bold = false;
  }

  if (style.color) {
    var tmp = this.parseColor (style.color);
    if (tmp != null)
      format.color = tmp;
  }

  format.display = style.display;

  if (style.fontFamily)
    format.font = this.parseCSSFontFamily (style.fontFamily);

  if (style.textIndent)
    format.indent = parseInt (style.textIndent);

  if (style.fontStyle == "italic") {
    format.italic = true;
  } else if (style.fontStyle == "normal") {
    format.italic = false;
  }

  if (style.kerning == "true") {
    format.kerning = true;
  } else if (style.kerning == "false") {
    format.kerning = false;
  } else {
    format.kerning = parseInt (style.kerning);
  }

  if (style.leading)
    format.leading = parseInt (style.leading);

  if (style.marginLeft)
    format.leftMargin = parseInt (style.marginLeft);

  if (style.letterSpacing)
    format.letterSpacing = parseInt (style.letterSpacing);

  if (style.marginRight)
    format.rightMargin = parseInt (style.marginRight);

  if (style.fontSize) {
    var tmp = parseInt (style.fontSize);
    if (tmp > 0)
      format.size = tmp;
  }

  if (style.textDecoration == "underline") {
    format.underline = true;
  } else if (style.textDecoration == "none") {
    format.underline = false;
  }

  return format;
};

TextField.StyleSheet.prototype.parseCSS = function (css) {
  var result = this.parseCSSInternal (css);
  if (typeof (result) == "null")
    return false;

  if (!this._css)
    this._css = {};

  for (var prop in result) {
    this._css[prop] = this._copy (result[prop]);
    this.doTransform (prop);
  }

  return true;
};

TextField.StyleSheet.prototype.parse = TextField.StyleSheet.prototype.parseCSS;

TextField.StyleSheet.prototype.load = ASnative (301, 0);

TextField.StyleSheet.prototype.onLoad = function () {
};

TextField.StyleSheet.prototype.onData = function (src) {
  if (src != null) {
    var result = this.parse (src);
    this.loaded = result;
    this.onLoad (result);
  } else {
    this.onLoad (false);
  }
};

TextField.StyleSheet.prototype.parseCSSInternal = ASnative (113, 101);
TextField.StyleSheet.prototype.parseCSSFontFamily = ASnative (113, 102);
TextField.StyleSheet.prototype.parseColor = ASnative (113, 103);
ASSetPropFlags (TextField.StyleSheet.prototype, null, 1027);
ASSetPropFlags (TextField, "StyleSheet", 1027);

/* Global Functions */

setInterval = ASnative (250, 0);
clearInterval = ASnative (250, 1);
setTimeout = ASnative(250, 2);
clearTimeout = clearInterval;

/*** OH THE HUMANITY ***/

o = null;

/*** GLOBAL PROPFLAGS */

ASSetPropFlags (this, null, 1, 6);
