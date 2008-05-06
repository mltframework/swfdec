// makeswf -v 7 -s 200x150 -r 1 -o key-isdown-mouse.swf key-isdown-mouse.as

trace ("Check if mouse presses show up as in Key.isDown and Key.getCode");

dump = function () {
  trace ("1: " + Key.isDown (1));
  trace ("2: " + Key.isDown (2));
  trace ("code: " + Key.getCode ());
};
Mouse.addListener ({ onMouseDown: dump, onMouseUp: dump });
dump ();

function quit () {
  dump ();
  getURL ("FSCommand:quit", "");
};
setTimeout (quit, 3000);
