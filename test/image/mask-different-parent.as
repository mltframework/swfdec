// makeswf -v 7 -s 200x150 -r 1 -o mask-different-parent.swf mask-different-parent.as

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

createEmptyMovieClip ("a", 0);
a.createEmptyMovieClip ("a", 0);
a.a.createEmptyMovieClip ("a", 0);
a.a.a.createEmptyMovieClip ("a", 0);
a._xscale = 50;
a._yscale = 50;
a.a._x = 50;
a.a._y = 50;
a.a.a._xscale = 200;
a.a.a._yscale = 200;
rectangle (a.a.a.a, 0xFF0000, 0, 0, 100, 100);

createEmptyMovieClip ("b", 1);
rectangle (b, 0xFF, 0, 0, 100, 100);

b.setMask (a.a.a.a);

