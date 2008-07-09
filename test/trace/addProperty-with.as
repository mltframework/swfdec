// makeswf -v 7 -r 15 -o addProperty-with.swf addProperty-with.as

trace ("Properties with get and set functions in the scope chain");

function get_a () { trace ("get_a"); return "a"; };
function set_a () { trace ("set_a: " + arguments); };

function get_b () { trace ("get_b"); return "b"; };
function set_b () { trace ("set_b: " + arguments); };

function get_c () { trace ("get_c"); return "c"; };

var a = { b: {}, c: {} };
a.addProperty ("test", get_a, set_a);
a.b.__proto__ = {};
a.b.__proto__.addProperty ("test", get_b, set_b);
with (a) {
  test = "hello1";
  with (b) {
    test = "hello2";
  }
}

a.c.__proto__.addProperty ("test", get_c, null);
with (a) {
  with (c) {
    test = "hello3";
  }
}
ASSetPropFlags (a.c, "__proto__", 0, 7);
a.c.__proto__ = null;
trace (a.c.test);

getURL ("FSCommand:quit", "");
