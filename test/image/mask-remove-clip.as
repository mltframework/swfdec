// makeswf -v 7 -s 200x150 -r 1 -o mask-remove-clip.swf mask-remove-clip.as

rectangle = function (mc, color, x, y, w, h)
{
  mc.beginFill (color);
  mc.moveTo (x, y);
  mc.lineTo (x, y + h);
  mc.lineTo (x + w, y + h);
  mc.lineTo (x + w, y);
  mc.lineTo (x, y);
  mc.endFill ();
};

createEmptyMovieClip ("clip", 0);
rectangle (clip, 0, 0, 0, 100, 100);

createEmptyMovieClip ("a", 1);
rectangle (a, 0xFF, 50, 50, 100, 100);

a.setMask (clip);
clip.removeMovieClip ();

