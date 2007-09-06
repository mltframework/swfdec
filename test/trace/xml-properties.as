// makeswf -v 7 -r 1 -o xml-properties-7.swf xml-properties.as

#include "trace_properties.as"

// Missing send etc. and version 6 has extra constructor in prototype
//trace_properties (_global.XML, "_global", "XML");
trace_properties (_global.XMLNode, "_global", "XMLNode");
// Not implemented
//trace_properties (_global.XMLSocket, "_global", "XMLSocket");

loadMovie ("FSCommand:quit", "");
