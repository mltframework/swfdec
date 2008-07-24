// makeswf -v 7 -s 200x150 -r 1 -o BitmapData-creation.swf BitmapData-creation.as

test = function (color)
{
  var d = getNextHighestDepth();
  var x = createEmptyMovieClip ("a" + d, d);
  var bd = new flash.display.BitmapData (10, 10, false, color);
  x.attachBitmap (bd, 0);
  x._x = d % 20 * 10;
  x._y = int (d / 20) * 10;

  d = getNextHighestDepth();
  x = createEmptyMovieClip ("a" + d, d);
  bd = new flash.display.BitmapData (10, 10, true, color);
  x.attachBitmap (bd, 0);
  x._x = d % 20 * 10;
  x._y = int (d / 20) * 10;
};

test (0);
test (0xFF);
test (0xFF0000FF);
test (0xFF0000);
test (0xFFFF0000);
test (0x1FF008000);
test (-1);
test ();

#include "values.as"
for (i = 0; i < values.length; i++) {
  test (values[i]);
}

