// makeswf -v 7 -r 1 -o test-7.swf test.as

var o = {};
var tmp = {};

function func () {
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
}
func.valueOf = func.toString = function () { return "func"; };

function func_and_child () {
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
  trace ("Child:");
  func ();
}
func_and_child.valueOf = func_and_child.toString =
  function () { return "func_and_child"; };


trace ("Global:");
trace ("arguments: " + arguments);
trace ("caller: " + arguments.caller);
trace ("callee: " + arguments.callee);
trace ("Child CallFunction:");
func ();
trace ("Child CallMethod:");
o.func = func;
o.func ();


trace ("");
trace ("Method:");
o.func_and_child = func_and_child;
o.func_and_child ();


trace ("");
trace ("Call:");
func_and_child.call ();


trace ("");
trace ("Apply:");
func_and_child.apply ();


trace ("");
trace ("Sort:");
tmp = new Array (2);
tmp[0] = "a";
tmp.sort (func_and_child);


function prop_get () {
  trace ("");
  trace ("Prop get:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
  trace ("Child:");
  func ();
}
prop_get.valueOf = prop_get.toString = function () { return "prop_get"; };

function prop_set () {
  trace ("");
  trace ("Prop set:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
  trace ("Child:");
  func ();
}
prop_set.valueOf = prop_set.toString = function () { return "prop_set"; };

o.addProperty ("prop", prop_get, prop_set);

o.prop = tmp;
tmp = o.prop;


function watcher () {
  trace ("");
  trace ("Watcher:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
  trace ("Child:");
  func ();
}
watcher.valueOf = watcher.toString = function () { return "watcher"; };

o.watched = true;
o.watch ("watched", watcher);
o.watched = false;


var emitter = new Object ();
AsBroadcaster.initialize (emitter);
emitter._listeners.push (new Object ());
emitter._listeners[0].broadcast = function () {
  trace ("");
  trace ("Broadcast:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
  trace ("Child:");
  func ();
};
emitter._listeners[0].broadcast.valueOf = function () { return "broadcast"; };
emitter._listeners[0].broadcast.toString = function () { return "broadcast"; };

emitter.broadcastMessage ("broadcast");


function timeout () {
  trace ("");
  trace ("Timeout:");
  trace ("arguments: " + arguments);
  trace ("caller: " + arguments.caller);
  trace ("callee: " + arguments.callee);
  trace ("Child:");
  func ();

  loadMovie ("FSCommand:quit", "");
}
timeout.valueOf = timeout.toString = function () { return "timeout"; };

setTimeout (timeout, 0);
