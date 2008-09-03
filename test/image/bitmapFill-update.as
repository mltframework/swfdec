// makeswf -v 7 -s 200x150 -r 1 -o bitmapFill-update.swf bitmapFill-update.as

bd = new flash.display.BitmapData (50, 50, true);

r = 1234567;
function myrandom (x) {
  r = r * r + 1234567;
  r = r % 100000000;
  return int (r / 100) % x;
}

rectangle = function (mc, bd, x, y, w, h)
{
  mc.beginBitmapFill (bd, null, false);
  mc.moveTo (x, y);
  mc.lineTo (x, y + h);
  mc.lineTo (x + w, y + h);
  mc.lineTo (x + w, y);
  mc.lineTo (x, y);
  mc.endFill ();
};

createEmptyMovieClip ("a", 0);
rectangle (a, bd, 0, 0, 200, 150);
a._xscale = 200;
a._yscale = 200;
trace (bd);

onEnterFrame = function () {
  for (i = 0; i < 1024; i++) {
    bd.setPixel32 (myrandom (50), myrandom (50), 0xFF000000 | (0x10000 * (i % 256)) | (0xFF - 0x1 * (i % 256)));
  }
  delete onEnterFrame;
};

