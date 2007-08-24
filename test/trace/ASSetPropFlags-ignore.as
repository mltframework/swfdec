// makeswf -v 7 -s 200x150 -r 1 -o ASSetPropFlags-ignore.swf ASSetPropFlags-ignore.as

trace ("Check how referencing variables with the 4 magic version flags works");
o = {};
o.__proto__ = {};
o.__proto__.a = 42;
o.__proto__.b = 42;
o.__proto__.c = 42;
o.__proto__.d = 42;
o.a = 21;
o.b = 21;
o.c = 21;
o.d = 21;
ASSetPropFlags (o, "a", 4096);
ASSetPropFlags (o, "b", 1024);
ASSetPropFlags (o, "c", 256);
ASSetPropFlags (o, "d", 128);
trace (o.a);
trace (o.b);
trace (o.c);
trace (o.d);

loadMovie ("FSCommand:quit", "");
