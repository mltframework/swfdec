// makeswf -v 7 -r 1 -o text-field-variable-7.swf text-field-variable.as

this.createTextField ("t", 0, 0, 0, 50, 50);

// movie clip
this.createEmptyMovieClip ("m", 1);
m.test = 1;
t.variable = "m.test";
trace (t.text);
m.test = 2;
trace (t.text);
t.text = 3;
trace (m.test);

// object
o = new Object();
o.test = 1;
t.variable = "o.test";
trace (t.text);
o.test = 2;
trace (o.text);
t.text = 3;
trace (o.test);

// nothing
delete n;
n.test = 1;
t.variable = "n.test";
trace (t.text);
n.test = 2;
trace (o.text);
t.text = 3;
trace (n.test);

loadMovie ("FSCommand:quit", "");

