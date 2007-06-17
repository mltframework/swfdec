// makeswf -v 7 -s 200x150 -r 1 -o string-split-empty.swf string-split-empty.as

trace ("Check String.split() on empty string");

s = "";
a = s.split ("x");
trace (typeof (a));
trace (a.length);
trace (a);
for (i = -1; i < 2; i++) {
  a = s.split ("x", i);
  trace (typeof (a));
  trace (a.length);
  trace (a);
}

loadMovie ("FSCommand:quit", "");
