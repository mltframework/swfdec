// makeswf -v 7 -s 200x150 -r 1 -o focusrect-rotated.swf focusrect-rotated.as

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
rectangle (a, 0xff, 0, 0, 50, 50);
a.focusEnabled = true;
Selection.setFocus (a);
a._x = 50;
a._y = 50;
a._rotation = 45;

setInterval (function () { Selection.setFocus (a); }, 100);
