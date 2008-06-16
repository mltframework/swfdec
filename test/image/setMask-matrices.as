// makeswf -v 7 -s 200x150 -r 1 -o setMask-matrices.swf setMask-matrices.as

rectangle = function (mc, color, x, y, w, h)
{
  mc.beginFill (color);
  mc.moveTo (x + w, y);
  mc.lineTo (x + w, y + h);
  mc.lineTo (x, y + h);
  mc.lineTo (x, y);
  mc.lineTo (x + w, y);
  mc.endFill ();
};

createEmptyMovieClip ("a", 0);
a.createEmptyMovieClip ("a", 0);
a._x = 10;
a.a._x = 10;
rectangle (a.a, 0xFF, 0, 0, 50, 100);

createEmptyMovieClip ("b", 1);
b.createEmptyMovieClip ("b", 1);
b._y = 10;
b.b._y = 10;
rectangle (b.b, 0xFF00, 0, 0, 100, 50);

a.a.setMask (b.b);
