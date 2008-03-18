// makeswf -v 7 -s 200x150 -r 1 -o if.swf if.as

if (1) {
  trace ("Hello World!");
} else {
  trace ("Goodbye cruel world");
}

if ("foo")
  trace ("Hello World");

if (true)
  ;
else
  trace ("Goodbye cruel world");

loadMovie ("fscommand:quit", "");
