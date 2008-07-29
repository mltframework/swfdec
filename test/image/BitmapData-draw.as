// makeswf -v 7 -s 200x150 -r 1 -o BitmapData-draw.swf BitmapData-draw.as

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
createEmptyMovieClip ("b", 1);
rectangle (b, 0xFF00, 10, 10, 30, 30);
a.setMask (b);

a._visible = false;
bg = new flash.display.BitmapData (50, 50);
bg.draw (a);
createEmptyMovieClip ("c", 2);
c.attachBitmap (bg, 0);
c._y = 50;

a._visible = true;
bg = new flash.display.BitmapData (50, 50);
bg.draw (a);
createEmptyMovieClip ("d", 3);
d.attachBitmap (bg, 0);
d._y = 100;

bg = new flash.display.BitmapData (50, 50);
bg.draw (b);
createEmptyMovieClip ("e", 4);
e.attachBitmap (bg, 0);
e._x = 50;
e._y = 50;

b._visible = false;
bg = new flash.display.BitmapData (50, 50);
bg.draw (b);
createEmptyMovieClip ("f", 5);
f.attachBitmap (bg, 0);
f._x = 50;
f._y = 100;

bg = new flash.display.BitmapData (50, 50);
bg.draw (a);
fg = new flash.display.BitmapData (50, 50);
fg.draw (bg);
createEmptyMovieClip ("g", 6);
g.attachBitmap (fg, 0);
g._x = 50;

createEmptyMovieClip ("aa", 100);
rectangle (aa, 0xFF0000, 0, 0, 50, 50);
aa._visible = false;
fg = new flash.display.BitmapData (50, 50);
fg.draw (aa, { a: "1.0", b: { valueOf: function () { return 0; }}, c: false, d: 1, tx: 0, ty: NaN });
createEmptyMovieClip ("h", 7);
h.attachBitmap (fg, 0);
h._x = 100;

fg = new flash.display.BitmapData (50, 50);
fg.draw (bg, { a: 0, b: 1, c: 0.5, d: 0, tx: 5, ty: -5 });
createEmptyMovieClip ("i", 8);
i.attachBitmap (fg, 0);
i._x = 100;
i._y = 50;

fg = new flash.display.BitmapData (50, 50);
fg.draw (a, new flash.geom.Matrix (), new flash.geom.ColorTransform (1, 0, 0, 1, 0, 255, 0, 255));
createEmptyMovieClip ("j", 9);
j.attachBitmap (fg, 0);
j._x = 100;
j._y = 100;

bg = new flash.display.BitmapData (50, 50, true, 0);
for (x = 0; x < 17; x++) {
  for (y = 0; y < 17; y++) {
    bg.setPixel32 (2 * x + 8, 2 * y + 8, x * 0xF0F0000 | y * 0xF0);
    bg.setPixel32 (2 * x + 9, 2 * y + 8, x * 0xF0F0000 | y * 0xF0);
    bg.setPixel32 (2 * x + 8, 2 * y + 9, x * 0xF0F0000 | y * 0xF0);
    bg.setPixel32 (2 * x + 9, 2 * y + 9, x * 0xF0F0000 | y * 0xF0);
  }
}
bg.setPixel32 (0, 0, 0xFF00FF00);
bg.setPixel32 (1, 0, 0);

createEmptyMovieClip ("k", 10);
k.attachBitmap (bg, 0);
k._x = 150;

fg = new flash.display.BitmapData (50, 50);
fg.draw (bg, new flash.geom.Matrix (), new flash.geom.ColorTransform (0.5, 0, 0, 1, 0, 255, 0, 255));
createEmptyMovieClip ("l", 11);
l.attachBitmap (fg, 0);
l._x = 150;
l._y = 50;
