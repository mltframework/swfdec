// makeswf -v 7 -s 200x150 -r 1 -o text-field-align.swf text-field-align.as

function create (depth, align) {
  var x = int (depth / 4);
  var y = depth % 4;
  createTextField ("t" + depth, depth, 100 * x, y * 30, 90, 25);
  var t = this["t" + depth];
  var tf = new TextFormat ();
  tf.font = "Bitstream Vera Sans";
  tf.align = align;
  t.setNewTextFormat (tf);
  t.border = true;
  t.text = "Hello World";

  return t;
};

create (0, "left");
create (1, "right");
create (2, "center");
create (3, "justify");

t = create (4, "left");
t.wordWrap = true;
t = create (5, "right");
t.wordWrap = true;
t = create (6, "center");
t.wordWrap = true;
t = create (7, "justify");
t.wordWrap = true;
