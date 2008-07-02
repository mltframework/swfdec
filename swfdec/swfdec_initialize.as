/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
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

ASSetPropFlags = ASnative (1, 0);
ASSetNative = ASnative (4, 0);
ASSetNativeAccessor = ASnative (4, 1);

flash = {};

/*** Object ***/
/* Only Flash extensions here, rest to swfdec_as_initialize.as */

Object.registerClass = ASnative(101, 8);
ASSetPropFlags (Object, null, 7);

// so we can use it in this script
ASSetPropFlags (Object, "addProperty", 0, 128);

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

Key = { ALT: 18, BACKSPACE: 8, CAPSLOCK: 20, CONTROL: 17, DELETEKEY: 46, 
    DOWN: 40, END: 35, ENTER: 13, ESCAPE: 27, HOME: 36, INSERT: 45, LEFT: 37, 
    PGDN: 34, PGUP: 33, RIGHT: 39, SHIFT: 16, SPACE: 32, TAB: 9, UP: 38 };
ASSetNative(Key, 800, "getAscii,getCode,isDown,isToggled,isAccessible");
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
ASSetNativeAccessor (Stage, 666, "scaleMode,align,width,height,showMenu,displayState", 1);
ASSetNativeAccessor (Stage, 666, "9fullScreenSourceRect,9fullScreenHeight,9fullScreenWidth", 100);

/*** NetConnection ***/

// FIXME: this should actually be a non-native function that calls the ASnative
// function
NetConnection = ASconstructor (2100, 200);

ASSetNative (NetConnection.prototype, 2100, "6connect,6close,6call,6addHeader");
NetConnection.prototype.addProperty ("connectedProxyType", ASnative (2100, 4), null);
NetConnection.prototype.proxyType = "none";
NetConnection.prototype.addProperty ("usingTLS", ASnative (2100, 5), null);
ASSetPropFlags (NetConnection.prototype, null, 3);

/*** NetStream ***/

NetStream.prototype.publish = function () {
  var o = {}; o["Implement NetStream.publish"] ();
};

NetStream.prototype.receiveAudio = function (flag) {
  var o = {}; o["Implement NetStream.receiveAudio"] ();
};

NetStream.prototype.receiveVideo = function (flag) {
  var o = {}; o["Implement NetStream.receiveVideo"] ();
};

ASSetNative (NetStream.prototype, 2101, "6close,6attachAudio,6attachVideo,6send,6setBufferTime");
ASSetNativeAccessor (NetStream.prototype, 2101, "checkPolicyFile", 5);
ASSetPropFlags (NetStream.prototype, null, 3);

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
delete XML.prototype.constructor;
ASSetPropFlags (XML, "prototype", 3);

XML.prototype.load = ASnative (301, 0);
XML.prototype.send = ASnative (301, 1);
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

XML.prototype.addRequestHeader = function (key, value) {
  if (typeof (this._customHeaders) == "undefined") {
    this._customHeaders = new Array ();
    ASSetPropFlags (this, "_customHeaders", 131);
  }
  if (typeof (key) == "string" && typeof (value) == "string") {
    this._customHeaders.push (key, value);
  } else if (key instanceof Array) {
    for (var i = 0; i + 1 < key.length; i += 2) {
      this.addRequestHeader (key[i], key[i + 1]);
    }
  }
};

/*** LOADVARS ***/

function LoadVars () { };

LoadVars.prototype.contentType = "application/x-www-form-urlencoded";

LoadVars.prototype.load = ASnative (301, 0);
LoadVars.prototype.send = ASnative (301, 1);
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
  var a = [];
  for (var x in this) {
    a.push (escape(x) + "=" + escape(this[x]));
  }
  return a.join ("&");
};

LoadVars.prototype.getBytesLoaded = function () {
  return this._bytesLoaded;
};

LoadVars.prototype.getBytesTotal = function () {
  return this._bytesTotal;
};

// Duplicated from XML
// Just using the same function here fails loadvars-properties test
LoadVars.prototype.addRequestHeader = function (key, value) {
  if (typeof (this._customHeaders) == "undefined") {
    this._customHeaders = new Array ();
    ASSetPropFlags (this, "_customHeaders", 131);
  }
  if (typeof (key) == "string" && typeof (value) == "string") {
    this._customHeaders.push (key, value);
  } else if (key instanceof Array) {
    for (var i = 0; i + 1 < key.length; i += 2) {
      this.addRequestHeader (key[i], key[i + 1]);
    }
  }
};

ASSetPropFlags(LoadVars.prototype, null, 131);

/*** Sound ***/

Sound = ASconstructor (500, 16);
ASSetNative (Sound.prototype, 500, "getPan,getTransform,getVolume,setPan,setTransform,setVolume,stop,attachSound,start,6getDuration,6setDuration,6getPosition,6setPosition,6loadSound,6getBytesLoaded,6getBytesTotal,9areSoundsInaccessible");
ASSetNativeAccessor (Sound.prototype, 500, "checkPolicyFile", 18);
ASSetPropFlags (Sound.prototype, null, 7);

/*** System ***/

System = {};

System.capabilities = {};
System.capabilities.Query = ASnative (11, 0);
System.capabilities.Query ();
delete System.capabilities.Query;

System.setClipboard = ASnative (1066, 0);
System.showSettings = ASnative (2107, 0);
ASSetNativeAccessor (System, 2107, "exactSettings,useCodepage", 1);
ASSetPropFlags (System, "exactSettings,useCodePage", 128);

/*** System.security */

System.security = new Object();
delete System.security.constructor;
ASSetNative (System.security, 12, "allowDomain,7allowInsecureDomain,loadPolicyFile,chooseLocalSwfPath,escapeDomain");
ASSetNativeAccessor (System.security, 12, "sandboxType", 5);

/*** System.security.PolicyFileResolver */

System.security.PolicyFileResolver = function () {
  var o = {}; o["Implement System.security.PolicyFileResolver"] ();
};
System.security.PolicyFileResolver.prototype.resolve = ASnative (15, 0);
AsBroadcaster.initialize (System.security.PolicyFileResolver.prototype);

/*** System.Product ***/

System.Product = function () {
  var o = {}; o["Implement System.Product"] ();
};

System.Product.prototype.isRunning = function () {
  var o = {}; o["Implement System.Product.isRunning"] ();
};

System.Product.prototype.isInstalled = function () {
  var o = {}; o["Implement System.Product.isInstalled"] ();
};

System.Product.prototype.launch = function () {
  var o = {}; o["Implement System.Product.launch"] ();
};

System.Product.prototype.download = function () {
  var o = {}; o["Implement System.Product.download"] ();
};

System.Product.prototype.installedVersion = function () {
  var o = {}; o["Implement System.Product.installedVersion"] ();
};

System.Product.prototype.validate = function () {
  var o = {}; o["Implement System.Product.validate"] ();
};

ASSetPropFlags (System.Product.prototype, null, 3);

/*** System.IME ***/

System.IME = {
  ALPHANUMERIC_FULL: "ALPHANUMERIC_FULL",
  ALPHANUMERIC_HALF: "ALPHANUMERIC_HALF",
  CHINESE: "CHINESE",
  JAPANESE_HIRAGANA: "JAPANESE_HIRAGANA",
  JAPANESE_KATAKANA_FULL: "JAPANESE_KATAKANA_FULL",
  JAPANESE_KATAKANA_HALF: "JAPANESE_KATAKANA_HALF",
  KOREAN: "KOREAN",
  UNKNOWN: "UNKNOWN"
};

AsBroadcaster.initialize (System.IME);

ASSetNative (System.IME, 13, "8getEnabled,8setEnabled,8getConversionMode,8setConversionMode,8setCompositionString,8doConversion");
ASSetPropFlags (System.IME, null, 7);

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

MovieClip.prototype.useHandCursor = true;
MovieClip.prototype.enabled = true;

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


ASSetNative (MovieClip.prototype, 900, "attachMovie,swapDepths,localToGlobal,globalToLocal,hitTest,getBounds,getBytesTotal,getBytesLoaded,6attachAudio,6attachVideo,6getDepth,6setMask,play,stop,nextFrame,prevFrame,gotoAndPlay,gotoAndStop,duplicateMovieClip,removeMovieClip,startDrag,stopDrag,7getNextHighestDepth,7getInstanceAtDepth,getSWFVersion,8attachBitmap,8getRect");
ASSetNativeAccessor (MovieClip.prototype, 900, "tabIndex", 200);
ASSetNativeAccessor (MovieClip.prototype, 900, "_lockroot", 300);
ASSetNativeAccessor (MovieClip.prototype, 900, "8cacheAsBitmap,8opaqueBackground,8scrollRect", 401);
ASSetNativeAccessor (MovieClip.prototype, 900, "8filters,8transform", 417);
ASSetNativeAccessor (MovieClip.prototype, 900, "8blendMode", 500);

ASSetNative (MovieClip.prototype, 901, "6createEmptyMovieClip,6beginFill,6beginGradientFill,6moveTo,6lineTo,6curveTo,6lineStyle,6endFill,6clear,8lineGradientStyle,8beginMeshFill,8beginBitmapFill");
ASSetNativeAccessor (MovieClip.prototype, 901, "8scale9Grid", 12);

MovieClip.prototype.createTextField = ASnative (104, 200);

ASSetPropFlags (MovieClip.prototype, null, 3);

/* MovieClipLoader */

MovieClipLoader = ASconstructor (112, 0);
ASSetNative(MovieClipLoader.prototype, 112, "7loadClip,7getProgress,7unloadClip", 100);
AsBroadcaster.initialize(MovieClipLoader.prototype);
ASSetPropFlags(MovieClipLoader.prototype, null, 1027);

/* TextField */

TextField = ASconstructor (104, 0);
TextField.getFontList = ASnative (104, 201);

ASSetNative (TextField.prototype, 104, "6replaceSel,6getTextFormat,6setTextFormat,6removeTextField,6getNewTextFormat,6setNewTextFormat,6getDepth,7replaceText", 100);
AsBroadcaster.initialize (TextField.prototype);

ASSetPropFlags (TextField.prototype, null, 131);

ASSetNativeAccessor (TextField.prototype, 104, "8gridFitType,8antiAliasType,8thickness,8sharpness,8filters", 300);
// FIXME: this should be done inside the TextField constructor
ASSetNativeAccessor (TextField.prototype, 900, "8tabIndex", 200);

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

/* Video */

ASSetNative(Video.prototype, 667, "6attachVideo,6clear", 1);
ASSetPropFlags(Video.prototype, null, 3);

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

flash.geom.Rectangle = function (x, y, width, height) {
  if (arguments.length == 0) {
    this.setEmpty ();
  } else {
    this.x = x;
    this.y = y;
    this.width = width;
    this.height = height;
  }
};

flash.geom.Rectangle.prototype.clone = function () {
  return new flash.geom.Rectangle (this.x, this.y, this.width, this.height);
};

flash.geom.Rectangle.prototype.toString = function () {
  return "(x=" + this.x + ", y=" + this.y + ", w=" + this.width + ", h=" + this.height + ")";
};

flash.geom.Rectangle.prototype.equals = function (rect) {
  return (rect instanceof flash.geom.Rectangle &&
      this.x == rect.x && this.y == rect.y &&
      this.width == rect.width && this.height == rect.height);
};

flash.geom.Rectangle.prototype.contains = function (x, y) {
  return (this.x <= x && this.y <= y &&
      this.x + this.width > x && this.y + this.height > y);
};

flash.geom.Rectangle.prototype.containsPoint = function (point) {
  return (this.x <= point.x && this.y <= point.y &&
      this.x + this.width > point.x && this.y + this.height > point.y);
};

flash.geom.Rectangle.prototype.containsRectangle = function (rect) {
  var tr = this.x + this.width;
  var tb = this.y + this.height;
  var rr = rect.x + rect.width;
  var rb = rect.y + rect.height;
  return (rect.x >= this.x && rect.x < tr && rect.y >= this.y && rect.y < tb &&
      rr > this.x && rr <= tr && rb > this.y && rb <= tb);
};

flash.geom.Rectangle.prototype.isEmpty = function () {
  return (this.width <= 0 || this.height <= 0);
};

flash.geom.Rectangle.prototype.setEmpty = function () {
  this.x = 0;
  this.y = 0;
  this.width = 0;
  this.height = 0;
};

flash.geom.Rectangle.prototype.intersection = function (rect) {
  var ints = new flash.geom.Rectangle ();

  if (this.isEmpty () || rect.isEmpty ()) {
    ints.setEmpty ();
    return ints;
  }

  ints.x = Math.max (this.x, rect.x);
  ints.y = Math.max (this.y, rect.y);
  ints.width = Math.min (this.x + this.width, rect.x + rect.width) - ints.x;
  ints.height = Math.min (this.y + this.height, rect.y + rect.height) - ints.y;

  if (ints.width <= 0 || ints.height <= 0)
      ints.setEmpty ();

  return ints;
};

flash.geom.Rectangle.prototype.intersects = function (rect) {
  return !this.intersection (rect).isEmpty ();
};

flash.geom.Rectangle.prototype.union = function (rect) {
  if (this.isEmpty ())
    return rect.clone ();

  if (rect.isEmpty ())
    return this.clone ();

  var union = new flash.geom.Rectangle ();
  union.x = Math.min (this.x, rect.x);
  union.y = Math.min (this.y, rect.y);
  union.width = Math.max (this.x + this.width, rect.x + rect.width) - union.x;
  union.height =
    Math.max (this.y + this.height, rect.y + rect.height) - union.y;

  return union;
};

flash.geom.Rectangle.prototype.offset = function (dx, dy) {
  this.x += dx;
  this.y += dy;
};

flash.geom.Rectangle.prototype.offsetPoint = function (d) {
  this.x += d.x;
  this.y += d.y;
};

flash.geom.Rectangle.prototype.inflate = function (dx, dy) {
  this.x -= dx;
  this.width += 2 * dx;

  this.y -= dy;
  this.height += 2 * dy;
};

flash.geom.Rectangle.prototype.inflatePoint = function (d) {
  this.x -= d.x;
  this.width += 2 * d.x;

  this.y -= d.y;
  this.height += 2 * d.y;
};

flash.geom.Rectangle.prototype.addProperty ("left",
  function () {
    return this.x;
  },
  function (left) {
    this.width += this.x - left;
    this.x = left;
  }
);

flash.geom.Rectangle.prototype.addProperty ("right",
  function () {
    return this.x + this.width;
  },
  function (right) {
    this.width = right - this.x;
  }
);

flash.geom.Rectangle.prototype.addProperty ("top",
  function () {
    return this.y;
  },
  function (top) {
    this.height += this.y - top;
    this.y = top;
  }
);

flash.geom.Rectangle.prototype.addProperty ("bottom",
  function () {
    return this.y + this.height;
  },
  function (bottom) {
    this.height = bottom - this.y;
  }
);

flash.geom.Rectangle.prototype.addProperty ("topLeft",
  function () {
    return new flash.geom.Point (this.x, this.y);
  },
  function (topLeft) {
    this.width += this.x - topLeft.x;
    this.height += this.y - topLeft.y;
    this.x = topLeft.x;
    this.y = topLeft.y;
  }
);

flash.geom.Rectangle.prototype.addProperty ("bottomRight",
  function () {
    return new flash.geom.Point (this.x + this.width, this.y + this.height);
  },
  function (bottomRight) {
    this.width = bottomRight.x - this.x;
    this.height = bottomRight.y - this.y;
  }
);

flash.geom.Rectangle.prototype.addProperty ("size",
  function () {
    return new flash.geom.Point (this.width, this.height);
  },
  function (size) {
    this.width = size.x;
    this.height = size.y;
  }
);

/* Matrix */

flash.geom.Matrix = function (pa, pb, pc, pd, ptx, pty) {
  if (!arguments.length) {
    this.identity ();
  } else {
    this.a = pa;
    this.b = pb;
    this.c = pc;
    this.d = pd;
    this.tx = ptx;
    this.ty = pty;
  }
};

flash.geom.Matrix.prototype.clone = function () {
  return new flash.geom.Matrix (this.a, this.b, this.c, this.d, this.tx,
      this.ty);
};

flash.geom.Matrix.prototype.concat = function () {
  var o = {}; o["Implement Matrix.concat"] ();
};

flash.geom.Matrix.prototype.createBox = function () {
  var o = {}; o["Implement Matrix.createBox"] ();
};

flash.geom.Matrix.prototype.createGradientBox = function () {
  var o = {}; o["Implement Matrix.createGradientBox"] ();
};

flash.geom.Matrix.prototype.deltaTransformPoint = function () {
  var o = {}; o["Implement Matrix.deltaTransformPoint"] ();
};

flash.geom.Matrix.prototype.identity = function () {
  this.a = this.d = 1;
  this.b = this.c = this.tx = this.ty = 0;
};

flash.geom.Matrix.prototype.invert = function () {
  var o = {}; o["Implement Matrix.invert"] ();
};

flash.geom.Matrix.prototype.rotate = function () {
  var o = {}; o["Implement Matrix.rotate"] ();
};

flash.geom.Matrix.prototype.scale = function () {
  var o = {}; o["Implement Matrix.scale"] ();
};

flash.geom.Matrix.prototype.transformPoint = function () {
  var o = {}; o["Implement Matrix.transformPoint"] ();
};

flash.geom.Matrix.prototype.translate = function () {
  var o = {}; o["Implement Matrix.translate"] ();
};

flash.geom.Matrix.prototype.toString = function () {
  return "(a=" + this.a + ", b=" + this.b + ", c=" + this.c + ", d=" + this.d +
    ", tx=" + this.tx + ", ty=" + this.ty + ")";
};

/* ColorTransform */

flash.geom.ColorTransform = ASconstructor (1105, 0);

flash.geom.ColorTransform.prototype.toString = function () {
  return "(redMultiplier=" + this.redMultiplier +
    ", greenMultiplier=" + this.greenMultiplier +
    ", blueMultiplier=" + this.blueMultiplier +
    ", alphaMultiplier=" + this.alphaMultiplier +
    ", redOffset=" + this.redOffset +
    ", greenOffset=" + this.greenOffset +
    ", blueOffset=" + this.blueOffset +
    ", alphaOffset=" + this.alphaOffset + ")";
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

flash.external.ExternalInterface.addCallback = function (name, instance, method) {
  if (!method || !flash.external.ExternalInterface.available)
    return false;

  flash.external.ExternalInterface._initJS ();
  return flash.external.ExternalInterface._addCallback (name, function (request) {
    return flash.external.ExternalInterface._callIn (instance, method, request);
  });
};

flash.external.ExternalInterface.call = function (name) {
  if (!flash.external.ExternalInterface.available)
    return null;

  flash.external.ExternalInterface._initJS ();
  var request = "try { ";
  var id = flash.external.ExternalInterface._objectID ();
  if (id != null)
    request += id + ".SetReturnValue(";
  request += "__flash__toXML (" + name + "(";
  for (var i = 1; i < arguments.length; i++) {
    if (i > 1)
      request += ",";
    request += flash.external.ExternalInterface._toJS (arguments[i]);
  }
  request += ")) ";
  if (id != null)
    request += ")";
  request += "; } catch (e) { ";
  if (id != null) {
    request += id + ".SetReturnValue(\"<undefined/>\"); }";
  } else {
    request += "\"<undefined/>\"; }";
  }
  var result = flash.external.ExternalInterface._evalJS (request);
  if (result == null) {
    request = "<invoke name=\"" + name + "\" returntype=\"xml\">" + flash.external.ExternalInterface._argumentsToXML (arguments) + "</invoke>";
    result = flash.external.ExternalInterface._callOut (request);
  }
  if (result == null)
    return null;

  var xml = new XML ();
  xml.ignoreWhite = true;
  xml.parseXML (result);
  return flash.external.ExternalInterface._toAS (xml.firstChild);
};

flash.external.ExternalInterface._callIn = function (instance, method, request) {
  var xml = new XML();
  xml.ignoreWhite = true;
  xml.parseXML (request);
  var args = null;
  for (var i = 0; i < xml.firstChild.childNodes.length; i++) {
    if (xml.firstChild.childNodes[i].nodeName == "arguments") {
      args = xml.firstChild.childNodes[i];
      break;
    }
  }
  var result = method.apply (instance, flash.external.ExternalInterface._argumentsToAS (args));
  if (xml.firstChild.attributes.returntype == "javascript")
    return flash.external.ExternalInterface._toJS (result);
  else
    return flash.external.ExternalInterface._toXML (result);
};

flash.external.ExternalInterface._arrayToXML = function (array) {
  var s = "<array>";
  for (var i = 0; i < array.length; i++) {
    s += "<property id=\"" + i + "\">" + flash.external.ExternalInterface._toXML (array[i]) + "</property>";
  }
  return s + "</array>";
};

flash.external.ExternalInterface._argumentsToXML = function (args) {
  var s = "<arguments>";
  for (var i = 0; i < args.length; i++) {
    s += flash.external.ExternalInterface._toXML (args[i]);
  }
  return s + "</arguments>";
};

flash.external.ExternalInterface._objectToXML = function (obj) {
  var s = "<object>";
  for (var prop in obj) {
      s += "<property id=\"" + prop + "\">" + flash.external.ExternalInterface._toXML (obj[prop]) + "</property>";
  }
  return s + "</object>";
};

flash.external.ExternalInterface._toXML = function (value) {
  var type = typeof(value);
  if (type == "string") {
    return "<string>" + flash.external.ExternalInterface._escapeXML(value) + "</string>";
  } else if (type == "undefined") {
    return "<undefined/>";
  } else if (type == "number") {
    return "<number>" + value + "</number>";
  } else if (value == null) {
    return "<null/>";
  } else if (type == "boolean") {
    return value ? "<true/>" : "<false/>";
  } else if (value.hasOwnProperty ("length")) {
    return flash.external.ExternalInterface._arrayToXML (value);
  } else if (type == "object") {
    return flash.external.ExternalInterface._objectToXML (value);
  } else {
    return "<null/>";
  }
};

flash.external.ExternalInterface._objectToAS = function (xml) {
  var o = {};
  for (i = 0; i < xml.childNodes.length; i++) {
    if (xml.childNodes[i].nodeName == "property")
      o[xml.childNodes[i].attributes.id] = flash.external.ExternalInterface._toAS (xml.childNodes[i].firstChild);
  }
  return o;
};

flash.external.ExternalInterface._arrayToAS = function (xml) {
  var a = [];
  for (i = 0; i < xml.childNodes.length; i++) {
    if (xml.childNodes[i].nodeName == "property")
      a[xml.childNodes[i].attributes.id] = flash.external.ExternalInterface._toAS (xml.childNodes[i].firstChild);
  }
  return a;
};

flash.external.ExternalInterface._toAS = function (xml) {
  var type = xml.nodeName;
  if (type == "number") {
    return Number (xml.firstChild.toString());
  } else if (type == "string") {
    return flash.external.ExternalInterface._unescapeXML (String (xml.firstChild));
  } else if (type == "false") {
    return false;
  } else if (type == "true") {
    return true;
  } else if (type == "null") {
    return null;
  } else if (type == "undefined") {
    return undefined;
  } else if (type == "object") {
    return flash.external.ExternalInterface._objectToAS (xml);
  } else if (type == "array") {
    return flash.external.ExternalInterface._arrayToAS (xml);
  } else if (type == "class") {
    return String (xml.firstChild);
  } else {
    return undefined;
  }
};

flash.external.ExternalInterface._argumentsToAS = function (xml) {
  var args = [];
  for (var i = 0; i < xml.childNodes.length; i++) {
    args.push (flash.external.ExternalInterface._toAS (xml.childNodes [i]));
  }
  return args;
};

flash.external.ExternalInterface._arrayToJS = function (array) {
  var s = "[";
  for (var i = 0; i < array.length; i++) {
    if (i != 0)
      s += ",";
    s += flash.external.ExternalInterface._toJS (array[_l1]);
  }
  return s + "]";
};

flash.external.ExternalInterface._objectToJS = function (obj) {
  var s = "({";
  var needs_comma = false;
  for (var prop in obj) {
    if (needs_comma) {
      s += ",";
    }
    needs_comma = true;
    s += prop + ":" + flash.external.ExternalInterface._toJS (obj[prop]);
  }
  return s + "})";
};

flash.external.ExternalInterface._toJS = function (value) {
  var type = typeof (value);
  if (type == "string") {
    return "\"" + flash.external.ExternalInterface._jsQuoteString (flash.external.ExternalInterface._unescapeXML (value)) + "\"";
  } else if (type == "object") {
    if (value instanceof Array) {
      return flash.external.ExternalInterface._arrayToJS (value);
    } else {
      return flash.external.ExternalInterface._objectToJS (value);
    }
  } else {
    return String (value);
  }
};

ASSetNative (flash.external.ExternalInterface, 14, "8_initJS,8_objectID,8_addCallback,8_evalJS,8_callOut,8_escapeXML,8_unescapeXML,8_jsQuoteString");
ASSetNativeAccessor (flash.external.ExternalInterface, 14, "8available", 100);

ASSetPropFlags (flash.external.ExternalInterface, null, 4103);

/* SharedObject */

function SharedObject () {
};

SharedObject.deleteAll = function () {
  var o = {}; o["Implement SharedObject.deleteAll (static)"] ();
};

SharedObject.getDiskUsage = function () {
  var o = {}; o["Implement SharedObject.getDiskUsage (static)"] ();
};

SharedObject.getLocal = function () {
  var o = {}; o["Implement SharedObject.getLocal (static)"] ();
};

SharedObject.getRemote = function () {
  var o = {}; o["Implement SharedObject.getRemote (static)"] ();
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
  var o = {}; o["Implement RemoteLSOUsage.getURLPageSupport (static)"] ();
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
delete flash.filters.BevelFilter.prototype.constructor;

ASSetNativeAccessor (flash.filters.BevelFilter.prototype, 1107, "8distance,8angle,8highlightColor,8highlightAlpha,8shadowColor,8shadowAlpha,8quality,8strength,8knockout,8blurX,8blurY,8type", 1);

/* BlurFilter */

flash.filters.BlurFilter = ASconstructor (1102, 0);
flash.filters.BlurFilter.prototype = new flash.filters.BitmapFilter ();
delete flash.filters.BlurFilter.prototype.constructor;

ASSetNativeAccessor (flash.filters.BlurFilter.prototype, 1102, "8blurX,8blurY,8quality", 1);

/* ColorMatrixFilter */

flash.filters.ColorMatrixFilter = ASconstructor (1110, 0);
flash.filters.ColorMatrixFilter.prototype = new flash.filters.BitmapFilter ();
delete flash.filters.ColorMatrixFilter.prototype.constructor;

ASSetNativeAccessor (flash.filters.ColorMatrixFilter.prototype, 1110, "8matrix", 1);

/* ConvolutionFilter */

flash.filters.ConvolutionFilter = ASconstructor (1109, 0);
flash.filters.ConvolutionFilter.prototype = new flash.filters.BitmapFilter ();
delete flash.filters.ConvolutionFilter.prototype.constructor;

ASSetNativeAccessor (flash.filters.ConvolutionFilter.prototype, 1109, "8matrixX,8matrixY,8matrix,8divisor,8bias,8preserveAlpha,8clamp,8color,8alpha", 1);

/* DisplacementMapFilter */

flash.filters.DisplacementMapFilter = ASconstructor (1111, 0);
flash.filters.DisplacementMapFilter.prototype = new flash.filters.BitmapFilter ();
delete flash.filters.DisplacementMapFilter.prototype.constructor;

ASSetNativeAccessor (flash.filters.DisplacementMapFilter.prototype, 1111, "8mapBitmap,8mapPoint,8componentX,8componentY,8scaleX,8scaleY,8mode,8color,8alpha", 1);

/* DropShadowFilter */

flash.filters.DropShadowFilter = ASconstructor (1101, 0);
flash.filters.DropShadowFilter.prototype = new flash.filters.BitmapFilter();
delete flash.filters.DropShadowFilter.prototype.constructor;

ASSetNativeAccessor (flash.filters.DropShadowFilter.prototype, 1101, "8distance,8angle,8color,8alpha,8quality,8inner,8knockout,8blurX,8blurY,8strength,8hideObject", 1);

/* GlowFilter */

flash.filters.GlowFilter = ASconstructor (1103, 0);
flash.filters.GlowFilter.prototype = new flash.filters.BitmapFilter ();
delete flash.filters.GlowFilter.prototype.constructor;

ASSetNativeAccessor (flash.filters.GlowFilter.prototype, 1103, "8color,8alpha,8quality,8inner,8knockout,8blurX,8blurY,8strength", 1);

/* GradientBevelFilter */

flash.filters.GradientBevelFilter = ASconstructor (1108, 1000);
flash.filters.GradientBevelFilter.prototype = new flash.filters.BitmapFilter ();
delete flash.filters.GradientBevelFilter.prototype.constructor;

ASSetNativeAccessor (flash.filters.GradientBevelFilter.prototype, 1108, "8distance,8angle,8colors,8alphas,8ratios,8blurX,8blurY,8quality,8strength,8knockout,8type", 1);

/* GradientGlowFilter */

flash.filters.GradientGlowFilter = ASconstructor (1108, 0);
flash.filters.GradientGlowFilter.prototype = o = new flash.filters.BitmapFilter ();
delete flash.filters.GradientGlowFilter.prototype.constructor;
ASSetNativeAccessor (flash.filters.GradientGlowFilter.prototype, 1108, "8distance,8angle,8colors,8alphas,8ratios,8blurX,8blurY,8quality,8strength,8knockout,8type", 1);

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

/*** Set propflags that would have interfered with running this script */

ASSetPropFlags (this, "flash", 4096);
ASSetPropFlags (Object, "addProperty", 128);
ASSetPropFlags (AsBroadcaster, null, 131);

/*** GLOBAL PROPFLAGS */

ASSetPropFlags (this, null, 1, 6);
