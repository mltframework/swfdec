// makeswf -v 7 -s 200x150 -r 1 -o string-properties-7.swf string-properties.as

#include "trace_properties.as"

var a = new Object ();
var b = { };

trace_properties (_global.Object, "_global", "Object");
trace_properties (a, "local", "a");
trace_properties (b, "local", "b");

loadMovie ("FSCommand:quit", "");
