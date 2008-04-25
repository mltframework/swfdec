// makeswf -v 7 -s 200x150 -r 1 -o and-or.swf and-or.as

if (1 && 2)
  trace (3);
if (1 || 2)
  trace (3);

if (1 && 2 && 3)
  trace (4);
if (1 || 2 || 3)
  trace (4);

if ((1 && 2) || 3)
  trace (4);
if ((1 || 2) && 3)
  trace (4);
