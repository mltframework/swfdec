// makeswf -v 7 -s 200x150 -r 1 -o mouse-scaled.swf mouse-scaled.as

trace ("Simple check for mouse movements on scaled movie");

dump = function () {
  trace (_xmouse);
  trace (_ymouse);
};
Mouse.addListener ({ onMouseMove: dump, onMouseDown: dump, onMouseUp: dump });

_xscale = 10;
_yscale = 1000;

function quit () {
  loadMovie ("FSCommand:quit", "");
};
setInterval (quit, 3000);
