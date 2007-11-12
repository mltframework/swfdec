// makeswf -v 7 -r 1 -o file-reference-list-properties-7.swf file-reference-list-properties.as

#include "trace_properties.as"

var a = new flash.net.FileReferenceList ();

trace_properties (_global.flash.net.FileReferenceList, "_global.flash.net", "FileReferenceList");
trace_properties (a, "local", "a");

loadMovie ("FSCommand:quit", "");
