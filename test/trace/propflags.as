// makeswf -v 7 -r 1 -o propflags-7.swf propflags.as

#include "trace_properties.as"

var o = new Object ();
o[0] = 0;
for (var i = 1; i <= 7; i++) {
  o[i] = i;
  ASSetPropFlags (o, i, i, 0);
}

trace_properties (o);

loadMovie ("FSCommand:quit", "");
