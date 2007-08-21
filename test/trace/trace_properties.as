// doesn't work for Flash 5

function new_empty_object () {
  var hash = new Object ();
  for (var prop in hash) {
    delete hash[prop];
  }
  return hash;
}

function hasOwnProperty (o, prop)
{
  if (o.__proto__ == undefined || o.__proto__ == "assdofjkaofjojsafjpojsafa")
  {
    if (o[prop] != undefined)
      return true;
    else
      return false;
    o.__proto__ = new_empty_object ();
    o.__proto__[prop] = "rapojdfoajiofjhaijhfidsjnfiohrbma";
    var result;
    if (o[prop] != o.__proto__[prop]) {
      result = true;
    } else {
      result = false;
    }
    // have to be set to string first, otherwise it won't delete
    o.__proto__ = "to-be-deleted";
    delete o.__proto__;
    return result;
  }
  else
  {
    var old = o.__proto__;
    o.__proto__ = "assdofjkaofjojsafjpojsafa";
    delete o.__proto__;
    if (o.__proto__ != undefined && o.__proto__ != "assdofjkaofjojsafjpojsafa") {
      if (hasOwnProperty (o.__proto__, prop))
      {
	var constant = false;
	var old = o.__proto__[prop];
	o.__proto__[prop] = "rapojdfoajiofjhaijhfidsjnfiohrbma";
	if (o.__proto__[prop] != "rapojdfoajiofjhaijhfidsjnfiohrbma") {
	  constant = true;
	  ASSetPropFlags (o.__proto__, prop, 0, 4);
	  o.__proto__[prop] = "rapojdfoajiofjhaijhfidsjnfiohrbma";
	}
	var result;
	if (o[prop] != o.__proto__[prop]) {
	  result = true;
	} else {
	  result = false;
	}
	o.__proto__[prop] = "to-be-replaced";
	o.__proto__[prop] = old;
	if (constant)
	  ASSetPropFlags (o.__proto__, prop, 4);
	return result;
      }
      else
      {
	o.__proto__[prop] = "rapojdfoajiofjhaijhfidsjnfiohrbma";
	var result;
	if (o[prop] != o.__proto__[prop]) {
	  result = true;
	} else {
	  result = false;
	}
	o.__proto__[prop] = "to-be-deleted";
	delete o.__proto__[prop];
	return result;
      }
    }
    result = hasOwnProperty (o, prop);
    o.__proto__ = old;
    return result;
  }
}


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

  return false;
}

function trace_properties_recurse (o, level, nextSecretId)
{
  // to collect info about different properties
  var info = new_info ();

  // calculate prefix
  var prefix = "";
  for (var j = 0; j < level; j++) {
    prefix = prefix + "  ";
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
    trace (prefix + "no children");
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

    // handle secretId that keeps track what things we have seen earlier
    var seen = false;
    if (hasOwnProperty (o[prop], "mySecretId")) {
      seen = true;
    } else {
      o[prop]["mySecretId"] = nextSecretId;
      if (o[prop]["mySecretId"] != undefined) {
	ASSetPropFlags (o[prop], "mySecretId", 7);
	nextSecretId++;
      }
    }
    var id_string = "";
    if (o[prop]["mySecretId"] != undefined)
      id_string = "[" + o[prop]["mySecretId"] + "]";

    // put things together
    var output = prop + flags + " = " + typeof (o[prop]) + id_string;

    // add value depending on the type
    if (typeof (o[prop]) == "number" || typeof (o[prop]) == "boolean") {
      output += " : " + o[prop];
    } else if (typeof (o[prop]) == "string") {
      output += " : \"" + o[prop] + "\"";
    } else if (typeof (o[prop]) == "object") {
      output += " : toString => \"" + o[prop] + "\"";
    }

    // print it out
    trace (prefix + output);

    // recurse if it's object or function that hasn't been seen earlier
    if (seen == false &&
	(typeof (o[prop]) == "object" || typeof (o[prop]) == "function"))
    {
      // move the next mySecretId to next hundred, this is to avoid screwing up
      // all the ids between two runs if they disagree at the start
      nextSecretId += 99;
      nextSecretId -= nextSecretId % 100;

      // recurse
      nextSecretId =
	trace_properties_recurse (o[prop], level + 1, nextSecretId);
    }
  }

  return nextSecretId;
}

function trace_properties (o, identifier)
{
  var output = identifier + " " + typeof (o) + "[0]";
  if (typeof (o) == "object")
    output += " : toString => \"" + o[prop] + "\"";
  trace (output);

  trace_properties_recurse (o, 1, 1);
}
