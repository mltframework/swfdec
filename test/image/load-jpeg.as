// makeswf -v 7 -s 200x150 -r 1 -o load-jpeg.swf load-jpeg.as

MovieClip.prototype.draw_rectangle = function (color) {
  this.beginFill (color);
  this.moveTo (0, 0);
  this.lineTo (0, 100);
  this.lineTo (200, 100);
  this.lineTo (200, 0);
  this.lineTo (0, 0);
  this.endFill ();
};

createEmptyMovieClip ("a", 0);
a._x = 200;
a._y = 150;
a._rotation = 180;
l = new MovieClipLoader ();
l.loadClip ("swfdec.jpg", a);
l.onLoadInit = function (m) {
  a.createEmptyMovieClip ("b", 0);
  a.b._x = 50;
  a.b._y = 50;
  a.b.draw_rectangle (0xFFFF);
  a.clear();
  a.draw_rectangle (0xFF);
};

