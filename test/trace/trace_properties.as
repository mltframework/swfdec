// doesn't work right in Flash 5

function new_empty_object () {
  var hash = new Object ();
  ASSetPropFlags (hash, null, 0, 7);
  for (var prop in hash) {
    delete hash[prop];
  }
  return hash;
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

// print all properties of a given object, flags are:
// h = hidden
// p = permanent
// P = permanent even without propflag
// c = constant
// C = constant even without propflag
function trace_properties (o)
{
  var info = new_info ();
  for (var prop in o) {
    set_info (info, prop, "hidden", false);
  }

  var hidden = new Array ();

  ASSetPropFlags (o, null, 0, 1);

  var all = new Array ();
  for (var prop in o) {
    all.push (prop);
    if (o.hasOwnProperty (prop)) {
      set_info (info, prop, "outproto", true);
    } else {
      set_info (info, prop, "outproto", false);
    }
    if (o.__proto__.hasOwnProperty (prop)) {
      set_info (info, prop, "inproto", true);
    } else {
      set_info (info, prop, "inproto", false);
    }
  }
  all.sort ();

  for (var prop in o) {
    if (get_info (info, prop, "hidden") != false) {
      set_info (info, prop, "hidden", true);
      hidden.push (prop);
    }
  }

  for (var prop in o) {
    var old = o[prop];
    var val = "hello " + o[prop];
    o[prop] = val;
    if (o[prop] != val) {
      set_info (info, prop, "constant", true);
      ASSetPropFlags (o, prop, 0, 4);
      o[prop] = val;
      if (o[prop] != val) {
	set_info (info, prop, "superconstant", true);
      } else {
	set_info (info, prop, "superconstant", false);
	o[prop] = old;
      }
      ASSetPropFlags (o, prop, 4);
    } else {
      set_info (info, prop, "constant", false);
      set_info (info, prop, "superconstant", false);
      o[prop] = old;
    }
  }

  for (var prop in o) {
    ASSetPropFlags (o, prop, 0, 4);
    var old = o[prop];
    delete o[prop];
    if (o.hasOwnProperty (prop)) {
      set_info (info, prop, "permanent", true);
      ASSetPropFlags (o, prop, 0, 2);
      delete o[prop];
      if (o.hasOwnProperty (prop)) {
	set_info (info, prop, "superpermanent", true);
      } else {
	set_info (info, prop, "superpermanent", false);
	o[prop] = old;
      }
      ASSetPropFlags (o, prop, 4);
    } else {
      set_info (info, prop, "permanent", false);
      o[prop] = old;
    }
    if (get_info (info, prop, "constant") == true)
      ASSetPropFlags (o, prop, 4);
  }

  ASSetPropFlags (o, hidden, 1, 0);

  for (var i = 0; i < all.length; i++) {
    var flags = "";

    if (get_info (info, all[i], "hidden") == true) {
      flags += "h";
    } else {
      flags += " ";
    }

    if (get_info (info, all[i], "superpermanent") == true) {
      flags += "P";
    } else if (get_info (info, all[i], "permanent") == true) {
      flags += "p";
    } else {
      flags += " ";
    }

    if (get_info (info, all[i], "superconstant") == true) {
      flags += "C";
    } else if (get_info (info, all[i], "constant") == true) {
      flags += "c";
    } else {
      flags += " ";
    }

    values = "";

    if (get_info (info, all[i], "outproto") == true) {
      values += " " + all[i] + " = " + o[all[i]];
    }
    if (get_info (info, all[i], "inproto") == true) {
      values += " __proto__." + all[i] + " = " + o.__proto__[all[i]];
    }

    trace (flags + values);
  }
}
