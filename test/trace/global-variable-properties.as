// makeswf -v 7 -r 1 -o global-variable-properties-7.swf global-variable-properties.as

#include "trace_properties.as"

trace_properties (_global.Infinity, "_global", "Infinity");
trace_properties (_global.NaN, "_global", "NaN");
trace_properties (_global.o, "_global", "o");
// _global.flash is undefined, so we need to check extra
//trace (hasOwnProperty (_global, "flash"));
//trace_properties (_global.flash, "_global", "flash");

loadMovie ("FSCommand:quit", "");

