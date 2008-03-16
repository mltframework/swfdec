// makeswf -v 7 -s 200x150 -r 1 -o if-nested.swf if-nested.as

if (1) {
  if (2) {
    trace ("one");
  } else {
    trace ("two");
  }
} else {
  if (3) {
    trace ("three");
  } else {
    trace ("four");
  }
}
