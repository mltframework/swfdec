// makeswf -v 7 -s 200x150 -r 1 -o text-field-wordWrap.swf text-field-wordWrap.as

function create (depth, x, y) {
  var t = createTextField ("t" + depth, depth, x, y, 50, 40);
  var t = this["t" + depth];
  t.border = true;
  var tf = new TextFormat ();
  tf.font = "Bitstream Vera Sans";
  //tf.leading = -5;
  t.setNewTextFormat (tf);
  t.text = "Hello,\nfinest World";
};

create (0, 25, 25);
create (1, 100.5, 25);
t1.wordWrap = true;
create (2, 25, 75);
t2._y += 0.4;
t2.multiline = true;
create (3, 100.5, 75);
t3._y += 0.5;
t3.wordWrap = true;
t3.multiline = true;

