// makeswf -v 7 -r 1 -o test-7.swf test.as

trace ("Global:");
trace ("arguments: " + arguments);
trace ("caller: " + arguments.caller);
trace ("callee: " + arguments.callee);

function func () {
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
}
func.toString = function () { return "func"; };

function prop_get () {
  trace ("");
  trace ("Prop get:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
}
prop_get.toString = function () { return "prop_get"; };

function prop_set () {
  trace ("");
  trace ("Prop set:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
}
prop_set.toString = function () { return "prop_set"; };

function watcher () {
  trace ("");
  trace ("Watcher:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
}
watcher.toString = function () { return "watcher"; };

var o = {};
o.func = func;
o.addProperty ("prop", prop_get, prop_set);
o.watched = true;
o.watch ("watched", watcher);

var emitter = new Object ();
AsBroadcaster.initialize (emitter);
emitter._listeners.push (new Object ());
emitter._listeners[0].broadcast = function () {
  trace ("");
  trace ("Broadcast:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
};
emitter._listeners[0].broadcast.toString = function () { return "broadcast"; };

function timeout () {
  trace ("");
  trace ("Timeout:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);

  loadMovie ("FSCommand:quit", "");
}
timeout.toString = function () { return "timeout"; };

trace ("");
trace ("Calling from outside of function");

trace ("");
trace ("CallFunction");
func ();

trace ("");
trace ("CallMethod");
o.func ();

trace ("");
trace ("Call:");
func.call ();

trace ("");
trace ("Apply:");
func.apply ();

trace ("");
trace ("New:");
var a = new func ();

trace ("");
trace ("Sort:");
a = new Array (2);
a[0] = "a";
a.sort (func);

o.prop = 2;
a = o.prop;

o.watched = false;

emitter.broadcastMessage ("broadcast");

function check () {
  trace ("");
  trace ("CallFunction");
  func ();

  trace ("");
  trace ("CallMethod");
  o.func ();

  trace ("");
  trace ("Call:");
  func.call ();

  trace ("");
  trace ("Apply:");
  func.apply ();

  trace ("");
  trace ("New:");
  var a = new func ();

  trace ("");
  trace ("Sort:");
  a = new Array (2);
  a[0] = "a";
  a.sort (func);

  o.prop = 2;
  a = o.prop;

  o.watched = false;

  emitter.broadcastMessage ("broadcast");

  setTimeout (timeout, 0);
}
check.toString = function () { return "check"; };

trace ("");
trace ("Calling from inside a function");

trace ("");
trace ("Check");
check ();
