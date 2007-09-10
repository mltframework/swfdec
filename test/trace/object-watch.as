// makeswf -v 7 -r 1 -o test-7.swf test.as

// create some functions
var watch_test = function (prop, old_value, new_value, user_data) {
  trace (this + "." + prop + ": " + old_value + " -> " + new_value + " (" + user_data + ") : " + arguments.length);
  return new_value;
};

var watch_loop = function (prop, old_value, new_value, user_data) {
  trace (old_value + " -> " + new_value);
  this[prop] = new_value + 1;
  return new_value;
};

var watch_loop_complex1 = function (prop, old_value, new_value, user_data) {
  trace ("1: " + old_value + " -> " + new_value);
  this["loop2"] = new_value + 1;
  return new_value;
};

var watch_loop_complex2 = function (prop, old_value, new_value, user_data) {
  trace ("2: " + old_value + " -> " + new_value);
  this["loop3"] = new_value + 1;
  return new_value;
};

var watch_loop_complex3 = function (prop, old_value, new_value, user_data) {
  trace ("3: " + old_value + " -> " + new_value);
  this["loop1"] = new_value + 1;
  return new_value;
};

// create an object
var o = new Object();
o.toString = function () { return "o"; };

// normal
o.test = 0;
trace (o.watch ("test", watch_test, "user data"));
o.test = 1;
trace (o.unwatch ("test"));
o.test = 2;

o.test = 0;
trace (o.watch ("test", watch_test));
o.test = 1;
trace (o.unwatch ("test"));
o.test = 2;

// missing params
trace (o.watch ());
trace (o.unwatch ());

o.test = 0;
trace (o.watch ("test"));
o.test = 1;
trace (o.unwatch ("test"));
o.test = 2;

// extra params
o.test = 0;
trace (o.watch ("test", watch_test, "user data", "user data2"));
o.test = 1;
trace (o.unwatch ("test"));
o.test = 2;

// loop
o.test = 0;
trace (o.watch ("test", watch_loop, "user data"));
o.test = 1;
trace (o.unwatch ("test"));
o.test = 2;

o.loop1 = 0;
trace (o.watch ("loop1", watch_loop_complex1));
trace (o.watch ("loop2", watch_loop_complex2));
trace (o.watch ("loop3", watch_loop_complex3));
o.loop1 = 1;
trace (o.unwatch ("loop1"));
o.loop1 = 2;

loadMovie ("FSCommand:quit", "");
