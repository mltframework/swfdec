// makeswf -v 7 -r 1 -o error-properties-7.swf error-properties.as

#include "trace_properties.as"

var a = new flash.net.FileReference ();

trace_properties (_global.flash.net.FileReference, "_global.flash.net", "FileReference");
trace_properties (a, "local", "a");

loadMovie ("FSCommand:quit", "");
