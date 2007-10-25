// makeswf -v 7 -r 1 -o test-7.swf test.as

trace ("Test how different functions handle __proto__ and it's propflags");

trace ("instanceOf:");

var p = { prototype: {} };
var o = { __proto__: p.prototype };

trace (o instanceOf p);
for (var i = 1; i <= 12; i++) {
  trace ("o.__proto__: " + Math.pow (2, i));
  ASSetPropFlags (o, "__proto__", Math.pow (2, i), 0);
  trace (o instanceOf p);
  ASSetPropFlags (o, "__proto__", 0, Math.pow (2, i));
}
trace ("o.__proto__: deleted");
delete o.__proto__;
trace (o instanceOf p);


trace ("get:");

p = { test: true };
o = { __proto__: p };

trace (o.test);
for (var i = 1; i <= 12; i++) {
  trace ("o.__proto__: " + Math.pow (2, i));
  ASSetPropFlags (o, "__proto__", Math.pow (2, i), 0);
  trace (o.test);
  ASSetPropFlags (o, "__proto__", 0, Math.pow (2, i));
}
trace ("o.__proto__: deleted");
delete o.__proto__;
trace (o.test);


trace ("set:");

function test_get () {
  trace ("test_get");
  return true;
}

function test_set () {
  trace ("test_set: " + arguments);
}

p = {};
p.addProperty ("test", test_get, test_set);
o = { __proto__: p };
o.hasOwnProperty = ASnative (101, 5);

o.test = true;
delete o.test;
for (var i = 1; i <= 12; i++) {
  trace ("o.__proto__: " + Math.pow (2, i));
  ASSetPropFlags (o, "__proto__", Math.pow (2, i), 0);
  o.test = true;
  delete o.test;
  ASSetPropFlags (o, "__proto__", 0, Math.pow (2, i));
}
trace ("o.__proto__: deleted");
delete o.__proto__;
o.test = true;


trace ("for ... in:");

p = { test: true };
o = { __proto__: p };

var found = false;
for (var prop in o) {
  if (prop == "test")
    found = true;
}
trace (found);
for (var i = 1; i <= 12; i++) {
  trace ("o.__proto__: " + Math.pow (2, i));
  ASSetPropFlags (o, "__proto__", Math.pow (2, i), 0);
  found = false;
  for (var prop in o) {
    if (prop == "test")
      found = true;
  }
  trace (found);
  ASSetPropFlags (o, "__proto__", 0, Math.pow (2, i));
}
trace ("o.__proto__: deleted");
delete o.__proto__;
found = false;
for (var prop in o) {
  if (prop == "test")
    found = true;
}
trace (found);


trace ("isPrototypeOf:");

p = { test: true };
o = { __proto__: p };

trace (p.isPrototypeOf (o));
for (var i = 1; i <= 12; i++) {
  trace ("o.__proto__: " + Math.pow (2, i));
  ASSetPropFlags (o, "__proto__", Math.pow (2, i), 0);
  trace (p.isPrototypeOf (o));
  ASSetPropFlags (o, "__proto__", 0, Math.pow (2, i));
}
trace ("o.__proto__: deleted");
delete o.__proto__;
trace (p.isPrototypeOf (o));


loadMovie ("FSCommand:quit", "");
