// makeswf -v 7 -r 1 -o error-properties-7.swf error-properties.as

// enable flash structure for version < 8 too for this test
ASSetPropFlags (_global, "flash", 0, 4096);

#include "trace_properties.as"

var a = new flash.net.FileReference ();

trace_properties (_global.flash.net.FileReference, "_global.flash.net", "FileReference");
trace_properties (a, "local", "a");

loadMovie ("FSCommand:quit", "");
