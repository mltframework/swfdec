// makeswf -v 7 -r 1 -o prototype-propflags-7.swf prototype-propflags.as

var a = new Object ();
a.test_property = true;
a.test_method = function () { return "true"; };

var b = new Object ();
b.__proto__ = a;

trace ("0: " + b.test);
for (var i = 1; i <= 8191; i++) {
  ASSetPropFlags (b, "__proto__", i, 0);
  trace (i + " property: " + b.test_property);
  trace (i + " method: " + b.test_method ());
  trace (i + " isPrototypeOf: " + a.isPrototypeOf (b));
  var props = [];
  for (var prop in b) {
    props.push (prop);
  }
  props.sort ();
  trace (i + " for-in: " + props);
  ASSetPropFlags (b, "__proto__", 0, i);
}

loadMovie ("FSCommand:quit", "");
