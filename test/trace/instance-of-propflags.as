// makeswf -v 7 -r 1 -o object-instanceof-propflags-7.swf object-instanceof-propflags.as

var f = { prototype: {} };
var o = { __proto__: f.prototype };

for (var i = 1; i <= 13; i++) {
  trace ("o.__proto__: " + (1 << i));
  ASSetPropFlags (o, "__proto__", 1 << i, 0);
  trace (o instanceOf f);
  ASSetPropFlags (o, "__proto__", 0, 1 << i);
}

for (var i = 1; i <= 13; i++) {
  trace ("f.prototype: " + (1 << i));
  ASSetPropFlags (f, "prototype", 1 << i, 0);
  trace (o instanceOf f);
  ASSetPropFlags (f, "prototype", 0, 1 << i);
}

loadMovie ("FSCommand:quit", "");
