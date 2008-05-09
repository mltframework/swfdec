// makeswf -v 7 -s 200x150 -r 1 -o text-field-border-scale.swf text-field-border-scale.as

createTextField ("s", 0, 0, 0, 90, 20);
var tf = new TextFormat ();
tf.font = "Bitstream Vera Sans";
s.setNewTextFormat (tf);
s.border = true;
s.text = "Hello World";

createTextField ("t", 1, 0, 30, 45, 20);
t.setNewTextFormat (tf);
t.border = true;
t.text = "Hello World";
t._xscale = 200;

createTextField ("u", 2, 0, 60, 100, 20);
u.setNewTextFormat (tf);
u.border = true;
u.text = "Hello World";
u._xscale = 90;

tf.size = 6;

createTextField ("a", 10, 100, 0, 90, 10);
a.setNewTextFormat (tf);
a.border = true;
a.text = "Hello World";
a._yscale = 200;

