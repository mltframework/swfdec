// makeswf -v 7 -s 200x150 -r 1 -o setPixel.swf setPixel.as

bg = new flash.display.BitmapData (200, 150, true, 0x80808080);
for (x = 0; x < bg.width; x++) {
  for (y = 0; y < bg.height; y++) {
    bg.setPixel (x, y, 255 - int (256 * x / bg.width));
  }
}
this.attachBitmap (bg, 0);

fg = new flash.display.BitmapData (_width, _height, true, 0);
for (x = 0; x < fg.width; x++) {
  for (y = 0; y < fg.height; y++) {
    fg.setPixel32 (x, y, 0x1010000 * int (256 * y / fg.height) | 0x100 * int (256 * x / fg.width));
  }
}
this.attachBitmap (fg, 1);
