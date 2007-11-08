// makeswf -v 7 -r 1 -o context-menu-properties-7.swf context-menu-properties.as

#include "trace_properties.as"

var a = new ContextMenu ();
var b = a.copy ();

trace_properties (_global.ContextMenu, "_global", "ContextMenu");
// FIXME: disabled for version 5 and 6 because of init script version issues
#if __SWF_VERSION__ >= 7
trace_properties (a, "local", "a");
#endif
trace_properties (b, "local", "b");

loadMovie ("FSCommand:quit", "");
