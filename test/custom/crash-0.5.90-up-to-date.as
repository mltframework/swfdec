// makeswf -v 7 -s 200x150 -r 1 -o crash-0.5.90-up-to-date.swf crash-0.5.90-up-to-date.as

createEmptyMovieClip ("a", 0);
a.beginFill (0);
a.lineTo (0, 10);
a.lineTo (10, 10);
a.lineTo (10, 0);
a.lineTo (0, 0);
a.endFill ();
a._xscale = 50;
Mouse.addListener ({ onMouseMove: function () { trace ("hi"); a._xscale = 10000 / a._xscale; } });
startDrag (a, 1);
