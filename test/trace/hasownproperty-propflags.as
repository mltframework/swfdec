// makeswf -v 7 -r 1 -o hasownproperty-propflags-7.swf hasownproperty-propflags.as

var o = new Object ();
o.hasOwnProperty = ASnative (101, 5);

o[0] = 0;
for (var i = 1; i <= 8191; i++) {
  o[i] = i;
  ASSetPropFlags (o, i, i, 0);
}

for (var i = 0; i <= 8191; i++) {
  trace (i + ": " + o.hasOwnProperty (i));
}

loadMovie ("FSCommand:quit", "");
