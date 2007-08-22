// makeswf -v 7 -r 1 -o function-properties-7.swf function-properties.as

#include "trace_properties.as"

o = new_empty_object ();
o.myFunction = function () { };
trace_properties (o, "local", "o");

loadMovie ("FSCommand:quit", "");
