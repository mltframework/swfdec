// makeswf -v 7 -r 1 -o glow-filter-properties-7.swf glow-filter-properties.as

// enable flash structure for version < 8 too for this test
ASSetPropFlags (_global, "flash", 0, 4096);

#include "trace_properties.as"

var a = new flash.filters.GlowFilter ();

trace_properties (_global.flash.filters.GlowFilter, "_global.flash.filters",
    "GlowFilter");
trace_properties (a, "local", "a");

loadMovie ("FSCommand:quit", "");
