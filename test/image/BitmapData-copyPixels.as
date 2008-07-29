// makeswf -v 7 -s 200x150 -r 1 -o BitmapData-copyPixels.swf BitmapData-copyPixels.as

trans = new flash.display.BitmapData (50, 50, true, 0);
for (x = 0; x < 17; x++) {
  for (y = 0; y < 17; y++) {
    trans.setPixel32 (2 * x + 8, 2 * y + 8, x * 0xF0F0F00 | y * 0xF);
    trans.setPixel32 (2 * x + 9, 2 * y + 8, x * 0xF0F0F00 | y * 0xF);
    trans.setPixel32 (2 * x + 8, 2 * y + 9, x * 0xF0F0F00 | y * 0xF);
    trans.setPixel32 (2 * x + 9, 2 * y + 9, x * 0xF0F0F00 | y * 0xF);
  }
}

opa = new flash.display.BitmapData (50, 50, false, 0);
for (x = 0; x < 17; x++) {
  for (y = 0; y < 17; y++) {
    opa.setPixel32 (2 * x + 8, 2 * y + 8, x * 0xF000F0F | y * 0xF0000);
    opa.setPixel32 (2 * x + 9, 2 * y + 8, x * 0xF000F0F | y * 0xF0000);
    opa.setPixel32 (2 * x + 8, 2 * y + 9, x * 0xF000F0F | y * 0xF0000);
    opa.setPixel32 (2 * x + 9, 2 * y + 9, x * 0xF000F0F | y * 0xF0000);
  }
}

mask = new flash.display.BitmapData (50, 50, true, 0);
for (x = 0; x < 50; x += 3) {
  mask.setPixel32 (x, 0, x * 0x05000000);
  mask.setPixel32 (x + 2, 0, x * 0x05000000);
  mask.setPixel32 (x + 1, 0, x * 0x05000000);
}
for (y = 1; y < 50; y *= 2) {
  mask.copyPixels (mask, { x: 0, y: 0, width: 50, height: y }, { x: 0, y: y });
}

array = [ trans, opa, mask ];

for (y = 0; y < 3; y++) {
  for (x = 0; x < 4; x++) {
    bm = array[y].clone ();
    if (x < 2)
      bm.copyPixels (array[x % 2], { x: 20, y: 20, width: 20, height: 20 }, { x: 11, y: 11 });
    else
      bm.copyPixels (trans, { x: 20, y: 20, width: 20, height: 20 }, { x: 11, y: 11 }, mask, { x: 0, y: 0}, x == 3);
    a = createEmptyMovieClip ("image" + getNextHighestDepth (), getNextHighestDepth ());
    a.attachBitmap (bm, 0);
    a._x = x * 50;
    a._y = y * 50;
  }
}
