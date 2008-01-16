// makeswf -v 7 -r 1 -o rectangle-contains-7.swf rectangle-contains.as

#include "values.as"

// enable flash structure for version < 8 too for this test
ASSetPropFlags (_global, "flash", 0, 5248);

var Rectangle = flash.geom.Rectangle;
var Point = flash.geom.Point;

for (var i = 0; i < values.length; i++) {
  var rect = new Rectangle (values[i], 20, 30, 40);
  trace (names[i] + ": " + rect);
  trace (rect.contains (20, 20));
  trace (rect.containsPoint (new Point (20, 20)));
  trace (rect.containsRectangle (new Rectangle (20, 20, 10, 10)));
  trace (rect.containsRectangle (rect));

  rect = new Rectangle (0, 20, 30, 40);
  trace (rect.contains (values[i], 20));
}

loadMovie ("FSCommand:quit", "");
