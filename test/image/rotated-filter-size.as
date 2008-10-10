// makeswf -v 7 -s 200x150 -r 1 -o rotated-filter-size.swf rotated-filter-size.as

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
rectangle (a, 0xFF, 0, 0, 50, 50);
a._x = 100;
a._y = 75;
a.filters = [ new flash.filters.ColorMatrixFilter ([0, 0, 0, 0, 255,
						    0, 0, 0, 0, 0,
						    0, 0, 0, 0, 0,
						    0, 0, 0, 0, 255])];

a._rotation = 120;

//getURL ("fscommand:quit", "");
