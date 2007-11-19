/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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

flash = {};
ASSetPropFlags (this, "flash", 4096);

/*** Object ***/
/* Only Flash extensions here, rest to swfdec_as_initialize.as */

Object.registerClass = ASnative(101, 8);
ASSetPropFlags (Object, null, 7);

/*** Error ***/

function Error (msg) {
  if (typeof (msg) != "undefined")
    this.message = msg;
}
Error.prototype.name = Error.prototype.message = "Error";
Error.prototype.toString = function () {
      return this.message;
};

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
LoadVars.prototype.sendAndLoad = ASnative (301, 2);
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
XML.prototype.sendAndLoad = ASnative (301, 2);
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

System.setClipboard = ASnative (1066, 0);
System.showSettings = ASnative (2107, 0);
ASSetNativeAccessor (System, 2107, "exactSettings,useCodePage", 1);
ASSetPropFlags (System, "exactSettings,useCodePage", 128);

/*** System.security */

System.security = new Object();
ASSetNative (System.security, 12, "allowDomain,7allowInsecureDomain,loadPolicyFile,chooseLocalSwfPath,escapeDomain");
ASSetNativeAccessor (System.security, 12, "sandboxType", 5);

/*** System.Product ***/

System.Product = function () {
  SWFDEC_STUB ("System.Product");
};

System.Product.prototype.isRunning = function () {
  SWFDEC_STUB ("System.Product.isRunning");
};

System.Product.prototype.isInstalled = function () {
  SWFDEC_STUB ("System.Product.isInstalled");
};

System.Product.prototype.launch = function () {
  SWFDEC_STUB ("System.Product.launch");
};

System.Product.prototype.download = function () {
  SWFDEC_STUB ("System.Product.download");
};

System.Product.prototype.installedVersion = function () {
  SWFDEC_STUB ("System.Product.installedVersion");
};

ASSetPropFlags (System.Product.prototype, null, 3);

/*** Color ***/

Color = function (target) {
  this.target = target;
  ASSetPropFlags (this, null, 7);
};
ASSetNative (Color.prototype, 700, "setRGB,setTransform,getRGB,getTransform");
ASSetPropFlags (Color.prototype, null, 7);

/* TextSnapshot */

TextSnapshot = ASconstructor (1067, 0);
ASSetNative (TextSnapshot.prototype, 1067, "6getCount,6setSelected,6getSelected,6getText,6getSelectedText,6hitTestTextNearPos,6findText,6setSelectColor,6getTextRunInfo", 1);

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
  setTarget ("");
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
  setTarget ("");
};

MovieClip.prototype.unloadMovie = function () {
  setTarget (this);
  loadMovie ("", this._target);
  setTarget ("");
};

MovieClip.prototype.getTextSnapshot = function () {
    return new TextSnapshot(this);
};
ASSetPropFlags (MovieClip.prototype, "getTextSnapshot", 128);

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
MovieClip.prototype.createTextField = ASnative (104, 200);
ASSetPropFlags (MovieClip.prototype, "getDepth", 128);

ASSetNative (MovieClip.prototype, 901, "6createEmptyMovieClip,6beginFill,6beginGradientFill,6moveTo,6lineTo,6curveTo,6lineStyle,6endFill,6clear");
ASSetPropFlags (MovieClip.prototype, null, 3);

/* MovieClipLoader */

MovieClipLoader = ASconstructor (112, 0);
ASSetNative(MovieClipLoader.prototype, 112, "7loadClip,7getProgress,7unloadClip", 100);
AsBroadcaster.initialize(MovieClipLoader.prototype);
ASSetPropFlags(MovieClipLoader.prototype, null, 1027);

/* TextField */

TextField = ASconstructor (104, 0);
TextField.getFontList = ASnative (104, 201);
TextField.prototype.getTextFormat = ASnative (104, 101);
TextField.prototype.setTextFormat = ASnative (104, 102);
TextField.prototype.removeTextField = ASnative (104, 103);
TextField.prototype.getNewTextFormat = ASnative (104, 104);
TextField.prototype.setNewTextFormat = ASnative (104, 105);
TextField.prototype.getDepth = ASnative (104, 106);
TextField.prototype.replaceText = ASnative (104, 107);
ASSetPropFlags (TextField.prototype, "replaceText", 1024);

AsBroadcaster.initialize (TextField.prototype);

ASSetPropFlags (TextField.prototype, null, 131);
ASSetPropFlags (TextField, null, 131);

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
  this.update ();
};

TextField.StyleSheet.prototype.getStyle = function (name) {
  return (this._copy (this._css[name]));
};

TextField.StyleSheet.prototype.setStyle = function (name, style) {
  if (!this._css)
    this._css = {};

  this._css[name] = this._copy (style);
  this.doTransform (name);
  this.update ();
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
  this.update ();

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

ASSetNative (TextField.StyleSheet.prototype, 113, "7update,7parseCSSInternal,7parseCSSFontFamily,7parseColor", 100);
ASSetPropFlags (TextField.StyleSheet.prototype, null, 1027);
ASSetPropFlags (TextField, "StyleSheet", 1027);

/* Accessibility */

Accessibility = {};
ASSetNative (Accessibility, 1999, "6isActive,6sendEvent,6updateProperties");
ASSetPropFlags (Accessibility, null, 6);

/* Camera */

function Camera () {
}

Camera.get = function (index) {
  var get_func = ASnative (2102, 200);
  return get_func (index);
};
Camera.addProperty ("names", ASnative (2102, 201), null);
ASSetNative (Camera.prototype, 2102, "6setMode,6setQuality,6setKeyFrameInterval,6setMotionLevel,6setLoopback,6setCursor");
ASSetPropFlags (Camera.prototype, null, 3);

/* ContextMenu */

function ContextMenu (callback) {
  this.onSelect = callback;
  this.customItems = new Array ();
  this.builtInItems = {
    forward_back: true,
    loop: true,
    play: true,
    print: true,
    quality: true,
    rewind: true,
    save: true,
    zoom: true
  };
}

ContextMenu.prototype.copy = function () {
  var o = new ContextMenu ();

  o.onSelect = this.onSelect;

  o.customItems = new Array ();
  for (var i = 0; i < this.customItems.length; i++) {
    o.customItems.push (this.customItems[i].copy ());
  }

  o.builtInItems = this.builtInItems;

  return o;
};

ContextMenu.prototype.hideBuiltInItems = function () {
  this.builtInItems = {
    forward_back: false,
    loop: false,
    play: false,
    print: false,
    quality: false,
    rewind: false,
    save: false,
    zoom: false
  };
};

ASSetPropFlags (ContextMenu.prototype, null, 1027);

/* ContextMenuItem */

function ContextMenuItem (caption, callback, separatorBefore, enabled, visible)
{
  this.caption = caption;
  this.onSelect = callback;
  if (separatorBefore == undefined) {
    this.separatorBefore = false;
  } else {
    this.separatorBefore = separatorBefore;
  }
  if (enabled == undefined) {
    this.enabled = true;
  } else {
    this.enabled = enabled;
  }
  if (visible == undefined) {
    this.visible = true;
  } else {
    this.visible = visible;
  }
}

ContextMenuItem.prototype.copy = function () {
  var o = new ContextMenuItem ();

  o.caption = this.caption;
  o.onSelect = this.onSelect;
  o.separatorBefore = this.separatorBefore;
  o.enabled = this.enabled;
  o.visible = this.visible;

  return o;
};

ASSetPropFlags (ContextMenuItem.prototype, null, 1027);

/* FileReference */

flash.net = {};
flash.net.FileReference = function () {
  var c = ASnative (2204, 200);
  c (this);
  this._listeners = [];
};

AsBroadcaster.initialize(flash.net.FileReference.prototype);
ASSetNative(flash.net.FileReference.prototype, 2204, "8browse,8upload,8download,8cancel");
ASSetPropFlags(flash.net.FileReference.prototype, null, 3);

/* FileReferenceList */

flash.net.FileReferenceList = function () {
  this.fileList = new Array();
  this._listeners = [];
};

AsBroadcaster.initialize (flash.net.FileReferenceList.prototype);
ASSetNative (flash.net.FileReferenceList.prototype, 2205, "8browse");
ASSetPropFlags (flash.net.FileReferenceList.prototype, null, 3);

/* LocalConnection */

function LocalConnection () {
}

ASSetNative (LocalConnection.prototype, 2200, "6connect,6send,6close,6domain");
ASSetPropFlags (LocalConnection.prototype, null, 3);

/* Microphone */

function Microphone () {
}

Microphone.get = function (index) {
  var get_func = ASnative (2104, 200);
  return get_func (index);
};
Microphone.addProperty ("names", ASnative (2104, 201), null);
ASSetNative (Microphone.prototype, 2104, "6setSilenceLevel,6setRate,6setGain,6setUseEchoSuppression");
ASSetPropFlags (Microphone.prototype, null, 3);

/* PrintJob */

PrintJob = ASconstructor(111, 0);
ASSetNative (PrintJob.prototype, 111, "7start,7addPage,7send", 100);
ASSetPropFlags (PrintJob.prototype, null, 1027);

/* Selection */

Selection = {};
ASSetNative (Selection, 600, "getBeginIndex,getEndIndex,getCaretIndex,getFocus,setFocus,setSelection");
AsBroadcaster.initialize (Selection);
ASSetPropFlags (Selection, null, 7);

/* TextRenderer */

flash.text = {};
flash.text.TextRenderer = ASconstructor (2150, 0);

ASSetNative (flash.text.TextRenderer, 2150, "8setAdvancedAntialiasingTable", 1);
ASSetNativeAccessor (flash.text.TextRenderer, 2150, "8maxLevel", 4);
ASSetNativeAccessor (flash.text.TextRenderer, 2150, "8displayMode", 10);

textRenderer = flash.text.TextRenderer; // awesome

/* XMLSocket */

function XMLSocket () {
}

XMLSocket.prototype.onData = function (src) {
    this.onXML (new XML (src));
};

ASSetNative (XMLSocket.prototype, 400, "connect,send,close");
ASSetPropFlags (XMLSocket.prototype, null, 3);

/* Point */

flash.geom = {};

flash.geom.Point = function () {
  if (arguments.length == 0) {
    this.x = 0;
    this.y = 0;
  } else {
    // Note: we don't check for length == 1
    this.x = arguments[0];
    this.y = arguments[1];
  }
};

flash.geom.Point.distance = function (a, b) {
  return a.subtract (b).length;
};

flash.geom.Point.interpolate = function (a, b, value) {
  return new flash.geom.Point (b.x + value * (a.x - b.x),
      b.y + value * (a.y - b.y));
};

flash.geom.Point.polar = function (length, angle) {
  return new flash.geom.Point (length * Math.cos (angle),
      length * Math.sin (angle));
};

flash.geom.Point.prototype.addProperty ("length",
    function () { return Math.sqrt (this.x * this.x + this.y * this.y); },
    null);

flash.geom.Point.prototype.add = function (other) {
    return new flash.geom.Point (this.x + other.x, this.y + other.y);
};

flash.geom.Point.prototype.clone = function () {
  return new flash.geom.Point (this.x, this.y);
};

flash.geom.Point.prototype.equals = function (other) {
  if (!other instanceOf flash.geom.Point)
    return false;

  return (other.x == this.x && other.y == this.y);
};

flash.geom.Point.prototype.normalize = function (length) {
  if (this.length <= 0)
    return undefined;

  var factor = length / this.length;
  this.x = this.x * factor;
  this.y = this.y * factor;
};

flash.geom.Point.prototype.subtract = function (other) {
    return new flash.geom.Point (this.x - other.x, this.y - other.y);
};

flash.geom.Point.prototype.offset = function (x, y) {
  this.x += x;
  this.y += y;
};

flash.geom.Point.prototype.toString = function () {
  return "(x=" + this.x + ", y=" + this.y + ")";
};

/* Rectangle */

flash.geom.Rectangle = function () {
  SWFDEC_STUB ("Rectangle");
};

flash.geom.Rectangle.prototype.clone = function () {
  SWFDEC_STUB ("Rectangle.clone");
};

flash.geom.Rectangle.prototype.setEmpty = function () {
  SWFDEC_STUB ("Rectangle.setEmpty");
};

flash.geom.Rectangle.prototype.isEmpty = function () {
  SWFDEC_STUB ("Rectangle.isEmpty");
};

flash.geom.Rectangle.prototype.addProperty ("left",
    function () { SWFDEC_STUB ("Rectangle.left (get)"); },
    function () { SWFDEC_STUB ("Rectangle.left (set)"); });

flash.geom.Rectangle.prototype.addProperty ("right",
    function () { SWFDEC_STUB ("Rectangle.right (get)"); },
    function () { SWFDEC_STUB ("Rectangle.right (set)"); });

flash.geom.Rectangle.prototype.addProperty ("top",
    function () { SWFDEC_STUB ("Rectangle.top (get)"); },
    function () { SWFDEC_STUB ("Rectangle.top (set)"); });

flash.geom.Rectangle.prototype.addProperty ("bottom",
    function () { SWFDEC_STUB ("Rectangle.bottom (get)"); },
    function () { SWFDEC_STUB ("Rectangle.bottom (set)"); });

flash.geom.Rectangle.prototype.addProperty ("topLeft",
    function () { SWFDEC_STUB ("Rectangle.topLeft (get)"); },
    function () { SWFDEC_STUB ("Rectangle.topLeft (set)"); });

flash.geom.Rectangle.prototype.addProperty ("bottomRight",
    function () { SWFDEC_STUB ("Rectangle.bottomRight (get)"); },
    function () { SWFDEC_STUB ("Rectangle.bottomRight (set)"); });

flash.geom.Rectangle.prototype.addProperty ("size",
    function () { SWFDEC_STUB ("Rectangle.size (get)"); },
    function () { SWFDEC_STUB ("Rectangle.size (set)"); });

flash.geom.Rectangle.prototype.inflate = function () {
  SWFDEC_STUB ("Rectangle.inflate");
};

flash.geom.Rectangle.prototype.inflatePoint = function () {
  SWFDEC_STUB ("Rectangle.inflatePoint");
};

flash.geom.Rectangle.prototype.offset = function () {
  SWFDEC_STUB ("Rectangle.offset");
};

flash.geom.Rectangle.prototype.offsetPoint = function () {
  SWFDEC_STUB ("Rectangle.offsetPoint");
};

flash.geom.Rectangle.prototype.contains = function () {
  SWFDEC_STUB ("Rectangle.contains");
};

flash.geom.Rectangle.prototype.containsPoint = function () {
  SWFDEC_STUB ("Rectangle.containsPoint");
};

flash.geom.Rectangle.prototype.containsRectangle = function () {
  SWFDEC_STUB ("Rectangle.containsRectangle");
};

flash.geom.Rectangle.prototype.intersection = function () {
  SWFDEC_STUB ("Rectangle.intersection");
};

flash.geom.Rectangle.prototype.intersects = function () {
  SWFDEC_STUB ("Rectangle.intersects");
};

flash.geom.Rectangle.prototype.union = function () {
  SWFDEC_STUB ("Rectangle.union");
};

flash.geom.Rectangle.prototype.equals = function () {
  SWFDEC_STUB ("Rectangle.equals");
};

flash.geom.Rectangle.prototype.toString = function () {
  SWFDEC_STUB ("Rectangle.toString");
};

/* Matrix */

flash.geom.Matrix = function () {
  SWFDEC_STUB ("Matrix");
};

flash.geom.Matrix.prototype.clone = function () {
  SWFDEC_STUB ("Matrix.clone");
};

flash.geom.Matrix.prototype.concat = function () {
  SWFDEC_STUB ("Matrix.concat");
};

flash.geom.Matrix.prototype.createBox = function () {
  SWFDEC_STUB ("Matrix.createBox");
};

flash.geom.Matrix.prototype.createGradientBox = function () {
  SWFDEC_STUB ("Matrix.createGradientBox");
};

flash.geom.Matrix.prototype.deltaTransformPoint = function () {
  SWFDEC_STUB ("Matrix.deltaTransformPoint");
};

flash.geom.Matrix.prototype.identity = function () {
  SWFDEC_STUB ("Matrix.identity");
};

flash.geom.Matrix.prototype.invert = function () {
  SWFDEC_STUB ("Matrix.invert");
};

flash.geom.Matrix.prototype.rotate = function () {
  SWFDEC_STUB ("Matrix.rotate");
};

flash.geom.Matrix.prototype.scale = function () {
  SWFDEC_STUB ("Matrix.scale");
};

flash.geom.Matrix.prototype.transformPoint = function () {
  SWFDEC_STUB ("Matrix.transformPoint");
};

flash.geom.Matrix.prototype.translate = function () {
  SWFDEC_STUB ("Matrix.translate");
};

flash.geom.Matrix.prototype.toString = function () {
  SWFDEC_STUB ("Matrix.toString");
};

/* ColorTransform */

flash.geom.ColorTransform = ASconstructor (1105, 0);

flash.geom.ColorTransform.prototype.toString = function () {
  SWFDEC_STUB ("ColorTransform.toString");
};

ASSetNative (flash.geom.ColorTransform.prototype, 1105, "8concat", 1);
ASSetNativeAccessor (flash.geom.ColorTransform.prototype, 1105, "8alphaMultiplier,8redMultiplier,8greenMultiplier,8blueMultiplier,8alphaOffset,8redOffset,8greenOffset,8blueOffset,8rgb", 101);

/* Transform */

flash.geom.Transform = ASconstructor (1106, 0);

ASSetNativeAccessor (flash.geom.Transform.prototype, 1106, "8matrix,8concatenatedMatrix,8colorTransform,8concatenatedColorTransform,8pixelBounds", 101);

/* BitmapData */

flash.display = {};
flash.display.BitmapData = ASconstructor (1100, 0);

flash.display.BitmapData.RED_CHANNEL = 1;
flash.display.BitmapData.GREEN_CHANNEL = 2;
flash.display.BitmapData.BLUE_CHANNEL = 4;
flash.display.BitmapData.ALPHA_CHANNEL = 8;

ASSetNative (flash.display.BitmapData, 1100, "8loadBitmap", 40);

ASSetNative (flash.display.BitmapData.prototype, 1100, "8getPixel,8setPixel,8fillRect,8copyPixels,8applyFilter,8scroll,8threshold,8draw,8pixelDissolve,8getPixel32,8setPixel32,8floodFill,8getColorBoundsRect,8perlinNoise,8colorTransform,8hitTest,8paletteMap,8merge,8noise,8copyChannel,8clone,8dispose,8generateFilterRect,8compare", 1);
ASSetNativeAccessor(flash.display.BitmapData.prototype, 1100, "8width,8height,8rectangle,8transparent", 100);

/* ExternalInterface */

flash.external = {};

flash.external.ExternalInterface = function () {
};

flash.external.ExternalInterface.addCallback = function () {
  SWFDEC_STUB ("ExternalInterface.addCallback (static)");
};

flash.external.ExternalInterface.call = function () {
  SWFDEC_STUB ("ExternalInterface.call (static)");
};

flash.external.ExternalInterface._callIn = function () {
  SWFDEC_STUB ("ExternalInterface._callIn (static)");
};

flash.external.ExternalInterface._toXML = function () {
  SWFDEC_STUB ("ExternalInterface._toXML (static)");
};

flash.external.ExternalInterface._objectToXML = function () {
  SWFDEC_STUB ("ExternalInterface._objectToXML (static)");
};

flash.external.ExternalInterface._arrayToXML = function () {
  SWFDEC_STUB ("ExternalInterface._arrayToXML (static)");
};

flash.external.ExternalInterface._argumentsToXML = function () {
  SWFDEC_STUB ("ExternalInterface._argumentsToXML (static)");
};

flash.external.ExternalInterface._toAS = function () {
  SWFDEC_STUB ("ExternalInterface._toAS (static)");
};

flash.external.ExternalInterface._objectToAS = function () {
  SWFDEC_STUB ("ExternalInterface._objectToAS (static)");
};

flash.external.ExternalInterface._arrayToAS = function () {
  SWFDEC_STUB ("ExternalInterface._arrayToAS (static)");
};

flash.external.ExternalInterface._argumentsToAS = function () {
  SWFDEC_STUB ("ExternalInterface._argumentsToAS (static)");
};

flash.external.ExternalInterface._toJS = function () {
  SWFDEC_STUB ("ExternalInterface._toJS (static)");
};

flash.external.ExternalInterface._objectToJS = function () {
  SWFDEC_STUB ("ExternalInterface._objectToJS (static)");
};

flash.external.ExternalInterface._arrayToJS = function () {
  SWFDEC_STUB ("ExternalInterface._arrayToJS (static)");
};

ASSetNative (flash.external.ExternalInterface, 14, "8_initJS,8_objectID,8_addCallback,8_evalJS,8_callOut,8_escapeXML,8_unescapeXML,8_jsQuoteString");
ASSetNativeAccessor (flash.external.ExternalInterface, 14, "8available", 100);

ASSetPropFlags (flash.external.ExternalInterface, null, 4103);

/* SharedObject */

function SharedObject () {
};

SharedObject.deleteAll = function () {
  SWFDEC_STUB ("SharedObject.deleteAll (static)");
};

SharedObject.getDiskUsage = function () {
  SWFDEC_STUB ("SharedObject.getDiskUsage (static)");
};

SharedObject.getLocal = function () {
  SWFDEC_STUB ("SharedObject.getLocal (static)");
};

SharedObject.getRemote = function () {
  SWFDEC_STUB ("SharedObject.getRemote (static)");
};

ASSetPropFlags (SharedObject, "deleteAll,getDiskUsage", 1);

ASSetNative (SharedObject.prototype, 2106, "6connect,6send,6flush,6close,6getSize,6setFps,6clear");
ASSetPropFlags (SharedObject.prototype, null, 3);

/* AsSetupError */

/* This function added new Error classes in Flash player 7, in Flash player 9
 * it seems to be just broken, we just call new Error based on the number of
 * ,-characters in the given string */
function AsSetupError (names) {
  var count = names.split (",").length;
  for (var i = 0; i < count; i++) {
    var tmp = new Error ();
  }
}

/* RemoteLSOUsage */

function RemoteLSOUsage () {
};

RemoteLSOUsage.getURLPageSupport = function () {
  SWFDEC_STUB ("RemoteLSOUsage.getURLPageSupport (static)");
};
ASSetPropFlags (RemoteLSOUsage, "getURLPageSupport", 1);

/* Button */

Button = ASconstructor (105, 0);

Button.prototype.useHandCursor = true;
Button.prototype.enabled = true;

Button.prototype.getDepth = ASnative (105, 3);
ASSetNativeAccessor (Button.prototype, 105, "8scale9Grid,8filters,8cacheAsBitmap,8blendMode", 4);
// FIXME: this should be done inside the Button constructor
ASSetNativeAccessor (Button.prototype, 900, "8tabIndex", 200);

/* BitmapFilter */

flash.filters = {};

flash.filters.BitmapFilter = ASconstructor(1112, 0);

ASSetNative(flash.filters.BitmapFilter.prototype, 1112, "8clone", 1);

/* BevelFilter */

flash.filters.BevelFilter = ASconstructor (1107, 0);
flash.filters.BevelFilter.prototype = new flash.filters.BitmapFilter ();

ASSetNativeAccessor (flash.filters.BevelFilter.prototype, 1107, "8distance,8angle,8highlightColor,8highlightAlpha,8shadowColor,8shadowAlpha,8quality,8strength,8knockout,8blurX,8blurY,8type", 1);

/* BlurFilter */

flash.filters.BlurFilter = ASconstructor (1102, 0);
flash.filters.BlurFilter.prototype = new flash.filters.BitmapFilter ();

ASSetNativeAccessor (flash.filters.BlurFilter.prototype, 1102, "8blurX,8blurY,8quality", 1);

/* ColorMatrixFilter */

flash.filters.ColorMatrixFilter = ASconstructor (1110, 0);
flash.filters.ColorMatrixFilter.prototype = new flash.filters.BitmapFilter ();

ASSetNativeAccessor (flash.filters.ColorMatrixFilter.prototype, 1110, "8matrix", 1);

/* Global Functions */

setInterval = ASnative (250, 0);
clearInterval = ASnative (250, 1);
setTimeout = ASnative(250, 2);
clearTimeout = clearInterval;
showRedrawRegions = ASnative (1021, 1);
trace = ASnative (100, 4);
updateAfterEvent = ASnative (9, 0);

/*** OH THE HUMANITY ***/

o = null;

/*** GLOBAL PROPFLAGS */

ASSetPropFlags (this, null, 1, 6);
