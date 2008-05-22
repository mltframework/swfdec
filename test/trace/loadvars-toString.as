// makeswf -v 7 -s 200x150 -r 1 -o loadvars-toString.swf loadvars-toString.as

#include "trace_properties.as"

trace (LoadVars.prototype);

Array.prototype.push = function (x) {
  trace ("pushed " + x);
  ASnative (252,1).call (this, x);
};
Array.prototype.join = function (x) {
  trace ("joining " + this + " with " + x);
  return ASnative (252,7).call (this, x);
};
_global.escape = function (s) {
  trace ("escaping " + s);
  var fun = ASnative (100, 0);
  return fun (s);
};
l = new LoadVars ();
l.x = 42;
l.y = "Hello World";
trace (l.toString ());

getURL ("fscommand:quit", "");
