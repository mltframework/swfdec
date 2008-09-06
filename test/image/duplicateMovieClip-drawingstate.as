// makeswf -v 7 -s 200x150 -r 1 -o duplicateMovieClip-drawingstate.swf duplicateMovieClip-drawingstate.as

#if __SWF_VERSION__ == 6
depth = 0;
getNextHighestDepth = function () {
  return depth++;
};
#endif

rectangle_start = function (mc, x, y, w, h)
{
  mc.moveTo (x, y);
  mc.lineTo (x, y + h);
  mc.lineTo (x + w, y + h);
};

rectangle_end = function (mc, x, y, w, h)
{
  mc.lineTo (x + w, y);
  mc.lineTo (x, y);
  mc.endFill ();
};

test_count = 0;
test = function (fun) {
  var x = createEmptyMovieClip ("a" + getNextHighestDepth (), getNextHighestDepth ());
  fun (x);
  x._x = (test_count % 2) * 100;
  x._y = int (test_count / 2) * 50;
  rectangle_start (x, 0, 0, 50, 50);
  y = x.duplicateMovieClip ("b" + getNextHighestDepth (), getNextHighestDepth ());
  y._x += 50;
  if (test_count % 2)
    y = x;
  trace (y);
  rectangle_end (y, 0, 0, 50, 50);
  test_count++;
};

test (function (mc) { m = new flash.geom.Matrix (); m.createGradientBox (50, 50); 
                      mc.beginGradientFill ("radial", [0xFF0000, 0xFF00], [100, 100], [0, 255], m); });
test (function (mc) { bd = new flash.display.BitmapData (50, 50, false, 0xFF0000); mc.beginBitmapFill (bd); });
test (function (mc) { mc.beginFill (0xFF); });
