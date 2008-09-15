// makeswf -v 7 -s 200x150 -r 1 -o BlurFilter-rendering.swf BlurFilter-rendering.as

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


for (x = 0; x < 4; x++) {
  for (y = 0; y < 3; y++) {
    a = createEmptyMovieClip ("a" + getNextHighestDepth (), getNextHighestDepth ());
    rectangle (a, 0xFF, 10, 10, 30, 30);
    a._x = 50 * x;
    a._y = 50 * y;
    a.filters = [ new flash.filters.BlurFilter (2 * y, 2 * y, x) ];
  };
};

createEmptyMovieClip ("mask", 1000);
rectangle (mask, 0xFF00, 5, 5, 30, 30);
a.setMask (mask);
