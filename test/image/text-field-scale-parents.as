// makeswf -v 7 -s 200x150 -r 1 -o text-field-scale-parents.swf text-field-scale-parents.as

createEmptyMovieClip ("a", 0);
a._xscale = -100;
a.createEmptyMovieClip ("a", 0);
a.a._xscale = -100;
a.a.createTextField ("t", 0, 0, 0, 50, 40);
a.a.t.background = true;
a.a.t.backgroundColor = 0xFF;

a.createTextField ("t", 1, -100, 0, 50, 40);
a.t._xscale = -200;
a.t.background = true;
a.t.backgroundColor = 0xFF00;
