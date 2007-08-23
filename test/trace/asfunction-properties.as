// makeswf -v 7 -r 1 -o asfunction-properties-7.swf asfunction-properties.as

#include "trace_properties.as"

trace_properties (_global.ASSetNative, "_global", "ASSetNative");
trace_properties (_global.ASSetNativeAccessor, "_global", "ASSetNativeAccessor");
trace_properties (_global.ASSetPropFlags, "_global", "ASSetPropFlags");
//trace_properties (_global.ASnative, "_global", "ASnative");
//trace_properties (_global.ASconstructor, "_global", "ASconstructor");

loadMovie ("FSCommand:quit", "");
