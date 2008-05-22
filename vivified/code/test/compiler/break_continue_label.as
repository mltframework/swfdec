before:
trace ("Hello");
if (a) {
  break before;
} else {
  continue after;
}
trace ("World");
after:
