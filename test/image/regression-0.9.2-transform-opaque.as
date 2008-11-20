// makeswf -v 7 -s 200x150 -r 15 -o regression-0.9.2-transform-opaque.swf regression-0.9.2-transform-opaque.as

createEmptyMovieClip ("x", 0);
x.loadMovie ("bw.jpg");
ct = new flash.geom.ColorTransform ();
ct.alphaOffset = -128;
t = new flash.geom.Transform (x);
t.colorTransform = ct;
