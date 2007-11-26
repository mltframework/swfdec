// makeswf -v 7 -r 1 -o propflags-set-proto-7.swf propflags-set-proto.as

trace ("See how property with version propflags in __proto__ is handled");

var a = {};
var b = {};
var c = {};

b.x = 2;
c.x = 3;

a.__proto__ = b;
b.__proto__ = c;

ASSetPropFlags (b, "x", 1 << 12);

trace (a.x);
trace (b.x);
trace (c.x);

a.x = 1;

trace (a.x);
trace (b.x);
trace (c.x);

delete b.__proto__;
delete a.__proto__;

ASSetPropFlags (b, "x", 0, 1 << 12);

trace (a.x);
trace (b.x);
trace (c.x);

loadMovie ("FSCommand:quit", "");
