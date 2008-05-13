// makeswf -v 7 -s 200x150 -r 1 -o text-field-autoSize.swf text-field-autoSize.as

function create (depth) {
  createTextField ("t" + depth, depth, 50, depth * 30, 100, 25);
  var t = this["t" + depth];
  var tf = new TextFormat ();
  tf.font = "Bitstream Vera Sans";
  t.setNewTextFormat (tf);
  t.border = true;
  t.text = "Hello World";

  return t;
};

t = create (0);
t.autoSize = "none";
t = create (1);
t.autoSize = "left";
t = create (2);
t.autoSize = "center";
t = create (3);
t.autoSize = "right";

