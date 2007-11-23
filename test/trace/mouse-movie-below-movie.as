// makeswf -v 7 -s 200x150 -r 1 -o mouse-movie-below-movie.swf mouse-movie-below-movie.as

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
  mc.onRollOver = function () { trace ("onRollOver: " + this); };
  mc.onRollOut = function () { trace ("onRollOut: " + this); };
  mc.onDragOut = function () { trace ("onDragOut: " + this); };
  mc.onDragOver = function () { trace ("onDragOver: " + this); };
  mc.onPress = function () { trace ("onPress: " + this); };
  mc.onRelease = function () { trace ("onRelease: " + this); };
  mc.onReleaseOutside = function () { trace ("onReleaseOutside: " + this); };
};

createEmptyMovieClip ("a", 0);
install (MovieClip.prototype);
createEmptyMovieClip ("b", 1);
rectangle (a, 0, 0, 0, 200, 150);
rectangle (b, 0xFF, 25, 25, 100, 100);
