// makeswf -v 7 -r 1 -o transform-properties-7.swf transform-properties.as

// enable flash structure for version < 8 too for this test
ASSetPropFlags (_global, "flash", 0, 4096);

#include "trace_properties.as"

var a = new flash.geom.Transform (this);

trace_properties (_global.flash.geom.Transform, "_global.flash.geom",
    "Transform");
trace_properties (a, "local", "a");

loadMovie ("FSCommand:quit", "");
