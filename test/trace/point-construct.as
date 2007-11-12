// makeswf -v 7 -r 1 -o point-construct-7.swf point-construct.as

#include "values.as"

trace ("Testing (no params):");
var p = new flash.geom.Point ();
trace ("new: " + p);
p = new flash.geom.Polar ();
trace ("polar: " + p);

for (var i = 0; i < values.length; i++) {
  trace ("Testing " + names[i] + ":");
  p = new flash.geom.Point (values[i]);
  trace ("new: " + p);
  p = new flash.geom.Polar (values[i]);
  trace ("polar: " + p);
  for (var j = i; j < values.length; j++) {
    trace ("Testing " + names[i] + ", " + names[j] + ":");
    p = new flash.geom.Point (values[i], values[j]);
    trace ("new: " + p);
    p = new flash.geom.Polar (values[i], values[j]);
    trace ("polar: " + p);
  }
}

loadMovie ("FSCommand:quit", "");
