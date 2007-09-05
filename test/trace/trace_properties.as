#if __SWF_VERSION__ == 5
// create a _global object, since it doesn't have one, these are ver 6 values
_global = new_empty_object ();
_global.ASSetNative = ASSetNative;
_global.ASSetNativeAccessor = ASSetNativeAccessor;
_global.ASSetPropFlags = ASSetPropFlags;
_global.ASconstructor = ASconstructor;
_global.ASnative = ASnative;
_global.Accessibility = Accessibility;
_global.Array = Array;
_global.AsBroadcaster = AsBroadcaster;
_global.AsSetupError = AsSetupError;
_global.Boolean = Boolean;
_global.Button = Button;
_global.Camera = Camera;
_global.Color = Color;
_global.ContextMenu = ContextMenu;
_global.ContextMenuItem = ContextMenuItem;
_global.Date = Date;
_global.Error = Error;
_global.Function = Function;
_global.Infinity = Infinity;
_global.Key = Key;
_global.LoadVars = LoadVars;
_global.LocalConnection = LocalConnection;
_global.Math = Math;
_global.Microphone = Microphone;
_global.Mouse = Mouse;
_global.MovieClip = MovieClip;
_global.MovieClipLoader = MovieClipLoader;
_global.NaN = NaN;
_global.NetConnection = NetConnection;
_global.NetStream = NetStream;
_global.Number = Number;
_global.Object = Object;
_global.PrintJob = PrintJob;
_global.RemoteLSOUsage = RemoteLSOUsage;
_global.Selection = Selection;
_global.SharedObject = SharedObject;
_global.Sound = Sound;
_global.Stage = Stage;
_global.String = String;
_global.System = System;
_global.TextField = TextField;
_global.TextFormat = TextFormat;
_global.TextSnapshot = TextSnapshot;
_global.Video = Video;
_global.XML = XML;
_global.XMLNode = XMLNode;
_global.XMLSocket = XMLSocket;
_global.clearInterval = clearInterval;
_global.clearTimeout = clearTimeout;
_global.enableDebugConsole = enableDebugConsole;
_global.escape = escape;
_global.flash = flash;
_global.isFinite = isFinite;
_global.isNaN = isNaN;
_global.o = o;
_global.parseFloat = parseFloat;
_global.parseInt = parseInt;
_global.setInterval = setInterval;
_global.setTimeout = setTimeout;
_global.showRedrawRegions = showRedrawRegions;
_global.textRenderer = textRenderer;
_global.trace = trace;
_global.unescape = unescape;
_global.updateAfterEvent = updateAfterEvent;
#endif

function new_empty_object () {
  var hash = new Object ();
  ASSetPropFlags (hash, null, 0, 7);
  for (var prop in hash) {
    hash[prop] = "to-be-deleted";
    delete hash[prop];
  }
  return hash;
}

#if __SWF_VERSION__ >= 6
function hasOwnProperty (o, prop)
{
  if (o.hasOwnProperty != undefined)
    return o.hasOwnProperty (prop);

  o.hasOwnProperty = _global.Object.prototype.hasOwnProperty;
  var result = o.hasOwnProperty (prop);
  delete o.hasOwnProperty;
  return result;
}
#else
// this gets the same result as the above, with following limitations:
// - if there is a child __proto__[prop] with value that can't be changed, no
//   test can be done and false is returned
// - native properties that have value undefined by default get overwritten by
//   __proto__[prop]'s value (atleast in version 6 and 7) so their existance
//   won't be detected by this function
function hasOwnProperty (o, prop)
{
  if (o.__proto__ == undefined)
  {
    o.__proto__ = new_empty_object ();

    o.__proto__[prop] = "safdlojasfljsaiofhiwjhefa";
    if (o[prop] != o.__proto__[prop]) {
      result = true;
    } else {
      result = false;
    }

    ASSetPropFlags (o, "__proto__", 0, 2);
    o.__proto__ = "to-be-deleted";
    delete o.__proto__;
    if (o.__proto__ != undefined) {
      o.__proto__ = undefined;
      if (o.__proto__ != undefined)
	trace ("ERROR: Couldn't delete temporary __proto__");
    }

    return result;
  }

  if (hasOwnProperty (o.__proto__, prop))
  {
    var constant = false;
    var old = o.__proto__[prop];

    o.__proto__[prop] = "safdlojasfljsaiofhiwjhefa";
    if (o.__proto__[prop] != "safdlojasfljsaiofhiwjhefa") {
      constant = true;
      ASSetPropFlags (o.__proto__, prop, 0, 4);
      o.__proto__[prop] = "safdlojasfljsaiofhiwjhefa";
      if (o.__proto__[prop] != "safdlojasfljsaiofhiwjhefa") {
	if (o[prop] != o.__proto__[prop]) {
	  return true;
	} else {
	  //trace ("ERROR: can't test property '" + prop +
	  //    "', __proto__ has superconstant version");
	  return false;
	}
      }
    }

    if (o[prop] != o.__proto__[prop]) {
      result = true;
    } else {
      result = false;
    }

    o.__proto__[prop] = old;
    if (o.__proto__[prop] != old)
      trace ("Error: Couldn't set __proto__[\"" + prop +
	  "\"] back to old value");
    if (constant)
      ASSetPropFlags (o.__proto__, prop, 4);

    return result;
  }
  else
  {
    o.__proto__[prop] = "safdlojasfljsaiofhiwjhefa";

    if (o[prop] != o.__proto__[prop]) {
      result = true;
    } else {
      result = false;
    }

    ASSetPropFlags (o, prop, 0, 4);
    o.__proto__[prop] = "to-be-deleted";
    delete o.__proto__[prop];
    if (o.__proto__[prop] != undefined) {
      o.__proto__[prop] = undefined;
      if (o.__proto__[prop] != undefined)
	trace ("ERROR: Couldn't delete temporary __proto__[\"" + prop + "\"]");
    }

    return result;
  }
}
#endif

function new_info () {
  return new_empty_object ();
}

function set_info (info, prop, id, value) {
  info[prop + "_-_" + id] = value;
}

function get_info (info, prop, id) {
  return info[prop + "_-_" + id];
}

function is_blaclisted (o, prop)
{
  if (prop == "mySecretId" || prop == "globalSecretId")
    return true;

  if (o == _global.Camera && prop == "names")
    return true;

  if (o == _global.Microphone && prop == "names")
    return true;

#if __SWF_VERSION__ < 6
  if (prop == "__proto__" && o[prop] == undefined)
    return true;
#endif

  return false;
}

function trace_properties_recurse (o, prefix, identifier, level)
{
  // to collect info about different properties
  var info = new_info ();

  // calculate indentation
  var indentation = "";
  for (var j = 0; j < level; j++) {
    indentation = indentation + "  ";
  }

  // mark the ones that are not hidden
  for (var prop in o)
  {
    // only get the ones that are not only in the __proto__
    if (is_blaclisted (o, prop) == false) {
      if (hasOwnProperty (o, prop) == true)
	set_info (info, prop, "hidden", false);
    }
  }

  // unhide everything
  ASSetPropFlags (o, null, 0, 1);

  var all = new Array ();
  var hidden = new Array ();
  for (var prop in o)
  {
    // only get the ones that are not only in the __proto__
    if (is_blaclisted (o, prop) == false) {
      if (hasOwnProperty (o, prop) == true) {
	all.push (prop);
	if (get_info (info, prop, "hidden") != false) {
	  set_info (info, prop, "hidden", true);
	  hidden.push (prop);
	}
      }
    }
  }
  all.sort ();

  // hide the ones that were already hidden
  ASSetPropFlags (o, hidden, 1, 0);

  if (all.length == 0) {
    trace (indentation + "no children");
    return nextSecretId;
  }

  for (var i = 0; i < all.length; i++)
  {
    var prop = all[i];

    // try changing value
    var old = o[prop];
    var val = "hello " + o[prop];
    o[prop] = val;
    if (o[prop] != val)
    {
      set_info (info, prop, "constant", true);

      // try changing value after removing constant propflag
      ASSetPropFlags (o, prop, 0, 4);
      o[prop] = val;
      if (o[prop] != val) {
	set_info (info, prop, "superconstant", true);
      } else {
	set_info (info, prop, "superconstant", false);
	o[prop] = old;
      }
      ASSetPropFlags (o, prop, 4);
    }
    else
    {
      set_info (info, prop, "constant", false);
      set_info (info, prop, "superconstant", false);
      o[prop] = old;
    }
  }

  for (var i = 0; i < all.length; i++)
  {
    var prop = all[i];

    // remove constant flag
    ASSetPropFlags (o, prop, 0, 4);

    // try deleting
    var old = o[prop];
    delete o[prop];
    if (hasOwnProperty (o, prop))
    {
      set_info (info, prop, "permanent", true);
    }
    else
    {
      set_info (info, prop, "permanent", false);
      o[prop] = old;
    }

    // put constant flag back, if it was set
    if (get_info (info, prop, "constant") == true)
      ASSetPropFlags (o, prop, 4);
  }

  for (var i = 0; i < all.length; i++) {
    var prop = all[i];

    // format propflags
    var flags = "";
    if (get_info (info, prop, "hidden") == true) {
      flags += "h";
    }
    if (get_info (info, prop, "superpermanent") == true) {
      flags += "P";
    } else if (get_info (info, prop, "permanent") == true) {
      flags += "p";
    }
    if (get_info (info, prop, "superconstant") == true) {
      flags += "C";
    } else if (get_info (info, prop, "constant") == true) {
      flags += "c";
    }
    if (flags != "")
      flags = " (" + flags + ")";

    var value = "";
    // add value depending on the type
    if (typeof (o[prop]) == "number" || typeof (o[prop]) == "boolean") {
      value += " : " + o[prop];
    } else if (typeof (o[prop]) == "string") {
      value += " : \"" + o[prop] + "\"";
    }

    // recurse if it's object or function and this is the place it has been
    // named after
    if (typeof (o[prop]) == "object" || typeof (o[prop]) == "function")
    {
      if (prefix + (prefix != "" ? "." : "") + identifier + "." + prop ==
	  o[prop]["mySecretId"])
      {
	trace (indentation + prop + flags + " = " + typeof (o[prop]));
	trace_properties_recurse (o[prop], prefix + (prefix != "" ? "." : "") +
	    identifier, prop, level + 1);
      }
      else
      {
	trace (indentation + prop + flags + " = " + o[prop]["mySecretId"]);
      }
    }
    else
    {
      trace (indentation + prop + flags + " = " + typeof (o[prop]) + value);
    }
  }
}

function generate_names (o, prefix, identifier)
{
  // mark the ones that are not hidden
  var nothidden = new Array ();
  for (var prop in o) {
    nothidden.push (prop);
  }

  // unhide everything
  ASSetPropFlags (o, null, 0, 1);

  var all = new Array ();
  for (var prop in o)
  {
    if (is_blaclisted (o, prop) == false) {
      // only get the ones that are not only in the __proto__
      if (hasOwnProperty (o, prop) == true) {
	all.push (prop);
      }
    }
  }
  all.sort ();

  // hide the ones that were already hidden
  ASSetPropFlags (o, null, 1, 0);
  ASSetPropFlags (o, nothidden, 0, 1);

  for (var i = 0; i < all.length; i++) {
    var prop = all[i];

    if (typeof (o[prop]) == "object" || typeof (o[prop]) == "function") {
      if (hasOwnProperty (o[prop], "mySecretId")) {
	all[i] = null; // don't recurse to it again
      } else {
	o[prop]["mySecretId"] = prefix + (prefix != "" ? "." : "") +
	  identifier + "." + prop;
      }
    }
  }

  for (var i = 0; i < all.length; i++) {
    var prop = all[i];

    if (prop != null) {
      if (typeof (o[prop]) == "object" || typeof (o[prop]) == "function")
	generate_names (o[prop], prefix + (prefix != "" ? "." : "") +
	    identifier, prop);
    }
  }
}

function trace_properties (o, prefix, identifier)
{
  _global["mySecretId"] = "_global";
  _global.Object["mySecretId"] = "_global.Object";
  _global.XMLNode["mySecretId"] = "_global.XMLNode";
  generate_names (_global.Object, "_global", "Object");
  generate_names (_global.XMLNode, "_global", "XMLNode");
  generate_names (_global, "", "_global");

  if (typeof (o) == "object" || typeof (o) == "function")
  {
    if (!o.hasOwnProperty ("mySecretId")) {
      o["mySecretId"] = prefix + (prefix != "" ? "." : "") + identifier;
      generate_names (o, prefix, identifier);
    }

    if (prefix + (prefix != "" ? "." : "") + identifier == o["mySecretId"])
    {
      trace (prefix + (prefix != "" ? "." : "") + identifier + " = " +
	  typeof (o));
    }
    else
    {
      trace (prefix + (prefix != "" ? "." : "") + identifier + " = " +
	  o["mySecretId"]);
    }
    trace_properties_recurse (o, prefix, identifier, 1);
  }
  else
  {
    var value = "";
    if (typeof (o) == "number" || typeof (o) == "boolean") {
      value += " : " + o;
    } else if (typeof (o) == "string") {
      value += " : \"" + o + "\"";
    }
    trace (prefix + (prefix != "" ? "." : "") + identifier + " = " +
	typeof (o) + value);
  }
}
