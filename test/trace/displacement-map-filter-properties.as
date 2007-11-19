// makeswf -v 7 -r 1 -o displacement-map-filter-properties-7.swf displacement-map-filter-properties.as

// enable flash structure for version < 8 too for this test
ASSetPropFlags (_global, "flash", 0, 4096);

#include "trace_properties.as"

var a = new flash.filters.DisplacementMapFilter ();

trace_properties (_global.flash.filters.DisplacementMapFilter,
        "_global.flash.filters", "DisplacementMapFilter");
trace_properties (a, "local", "a");

loadMovie ("FSCommand:quit", "");
