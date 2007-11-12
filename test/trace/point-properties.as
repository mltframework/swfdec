// makeswf -v 7 -r 1 -o point-properties-7.swf point-properties.as

#include "trace_properties.as"

var a = new flash.geom.Point ();

trace_properties (_global.flash.geom.Point, "_global.flash.geom", "Point");
trace_properties (a, "local", "a");

loadMovie ("FSCommand:quit", "");
