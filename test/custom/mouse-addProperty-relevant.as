// makeswf -v 7 -s 200x150 -r 1 -o mouse-addProperty-relevant.swf mouse-addProperty-relevant.as

trace ("Check if addProperty proeprties make movies sensitive to mouse events");

rectangle = function (mc, color, x, y, w, h) {
  mc.beginFill (color);
  mc.moveTo (x, y);
  mc.lineTo (x, y + h);
  mc.lineTo (x + w, y + h);
  mc.lineTo (x + w, y);
  mc.lineTo (x, y);
  mc.endFill ();
};

install = function (mc) {
  mc.addProperty ("onRollOver", function () { trace ("onRollOver: " + this); }, null);
};

install (Object.prototype);
createEmptyMovieClip ("a", 0);
createEmptyMovieClip ("b", 1);
rectangle (a, 0, 0, 0, 200, 150);
rectangle (b, 0xFF, 25, 25, 100, 100);
