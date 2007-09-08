// makeswf -v 7 -r 1 -o test-7.swf test.as

// Test how propflags influence and are influenced by variable setting

var o = new Object ();
o.hasOwnProperty = ASnative (101, 5);

o[0] = 0;
for (var i = 1; i <= 8191; i++) {
  if (i & 2048)
    continue;
  o[i] = i;
  ASSetPropFlags (o, i, i, 0);
}

for (var i = 0; i <= 8191; i++) {
  if (i & 2048)
    continue;
  trace (i + ": " + o[i]);
  o[i] = i + " reset";
  trace (i + ": " + o[i]);
  ASSetPropFlags (o, i, 0, 8191);
  trace (i + ": " + o[i]);
}

loadMovie ("FSCommand:quit", "");
