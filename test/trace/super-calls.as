// makeswf -v 7 -s 200x150 -r 1 -o super-calls.swf super-calls.as

trace ("Check various behaviours of the super object");

function One () {
  trace (1);
};
function Two () {
  super ();
  trace (2);
};
function Three () {
  super ();
  trace (3);
};
asm {
  push "Two"
  getvariable
  push "One"
  getvariable
  extends
};
asm {
  push "Three"
  getvariable
  push "Two"
  getvariable
  extends
};
One.prototype.foo = function () {
  trace ("foo: 1");
};
Two.prototype.foo = function () {
  super.foo ();
  trace ("foo: 2");
};
Three.prototype.foo = function () {
  super.foo (this);
  trace ("foo: 3");
};
x = new Three ();
x.foo ();
x.foo.call (this);
Three.prototype.foo ();

loadMovie ("FSCommand:quit", "");
