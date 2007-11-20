// makeswf -v 7 -s 200x150 -r 1 -o mask-textfield.swf mask-textfield.as

rectangle = function (mc, color, x, y, w, h)
{
  mc.beginFill (color);
  mc.moveTo (x, y);
  mc.lineTo (x, y + h);
  mc.lineTo (x + w, y + h);
  mc.lineTo (x + w, y);
  mc.lineTo (x, y);
  mc.endFill ();
};

createTextField ("text", 0, 0, 0, 100, 100);
text.background = true;
text.backgroundColor = 0xFF00;
text.text = "Hello World";

createEmptyMovieClip ("a", 1);
rectangle (a, 0xFF, 50, 50, 100, 100);

text.setMask = a.setMask;
text.setMask (a);

