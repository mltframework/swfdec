// makeswf -v 7 -s 200x150 -r 1 -o beginBitmapFill.swf beginBitmapFill.as

bd = new flash.display.BitmapData (48, 48, true);

for (x = 0; x < bd.width; x++) {
  for (y = 0; y < bd.height; y++) {
    bd.setPixel32 (x, y, 0x11110000 * int (x / 3) + 0x11 * int (y / 3));
  };
};

count = 0;
rectangle = function (mc)
{
  mc.moveTo (50 * (count % 4), 50 * int (count / 4));
  mc.lineTo (50 * (count % 4), 50 * int (count / 4) + 50);
  mc.lineTo (50 * (count % 4) + 50, 50 * int (count / 4) + 50);
  mc.lineTo (50 * (count % 4) + 50, 50 * int (count / 4));
  mc.lineTo (50 * (count % 4), 50 * int (count / 4));
  mc.endFill ();
  count++;
};

createEmptyMovieClip ("a", 0);

a.beginBitmapFill (bd);
rectangle (a);

a.beginBitmapFill (bd, undefined);
rectangle (a);

m = new flash.geom.Matrix ();
m.rotate (Math.PI);
a.beginBitmapFill (bd, m);
rectangle (a);

m = new flash.geom.Matrix ();
m.rotate (Math.PI / 2);
a.beginBitmapFill (bd, m);
rectangle (a);


a.beginBitmapFill (bd, null, true, false);
rectangle (a);
a.beginBitmapFill (bd, null, true, true);
rectangle (a);
a.beginBitmapFill (bd, null, false, false);
rectangle (a);
a.beginBitmapFill (bd, null, false, true);
rectangle (a);


a.beginBitmapFill ();
rectangle (a);
a.beginBitmapFill ({});
rectangle (a);

m = new flash.geom.Matrix ();
m.translate (50, 0);
a.beginBitmapFill (bd, m);
rectangle (a);

a.beginBitmapFill (bd, {});
rectangle (a);

