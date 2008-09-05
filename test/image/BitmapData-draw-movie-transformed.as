// makeswf -v 7 -s 200x150 -r 1 -o BitmapData-draw-movie-transformed.swf BitmapData-draw-movie-transformed.as

createEmptyMovieClip ("a", 0);

a._x = 200;
a._y = 150;
a._xscale = -100;
a._yscale = -20000;

m = new flash.geom.Matrix ();
m.createGradientBox (50, 50, -1.57, 0, 70);
a.beginGradientFill ("linear", [0xFF, 0xFF00], [100, 100], [0, 255], m);
a.lineTo (0, 150);
a.lineTo (200, 150);
a.lineTo (200, 0);
a.lineTo (0, 0);
a.endFill ();

bd = new flash.display.BitmapData (200, 150, true);
bd.draw (a);
a.removeMovieClip ();
attachBitmap (bd, 0);
