// makeswf -v 7 -s 200x150 -r 1 -o bitmap-data-fillRect.swf bitmap-data-fillRect.as

var RECT_SIZE = 25;

var mc1 = createEmptyMovieClip("mc1", getNextHighestDepth());
var b1 = new flash.display.BitmapData(RECT_SIZE, RECT_SIZE, false, 0xffff0000);
var mc2 = createEmptyMovieClip("mc2", getNextHighestDepth());
var b2 = new flash.display.BitmapData(RECT_SIZE, RECT_SIZE, true, 0xffff0000);
b1.fillRect(b1.rectangle, 0x80000000);
b2.fillRect(b2.rectangle, 0x80000000);
mc1.attachBitmap(b1, mc1.getNextHighestDepth());
mc2.attachBitmap(b2, mc2.getNextHighestDepth());
mc1._x = 0;
mc1._y = 0;
mc2._x = RECT_SIZE;
mc2._y = RECT_SIZE;
