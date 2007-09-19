// makeswf -v 7 -r 1 -o test-7.swf test.as

// FIXME: We don't test for _level0 onData because it's called at the start by
// Flash player, but not by swfdec atm.

var a = this.createEmptyMovieClip ("a", this.getNextHighestDepth ());

a.onData = function () {
  trace ("onData a: " + this.test);
};

_level0.test = 1;

trace ("start a: " + a.test);
trace ("start _level0: " + _level0.test);

loadVariables ("loadvars.txt", "a");
loadVariablesNum ("loadvars.txt", 0);

function quit () {
  trace ("quit a: " + a.test);
  trace ("quit _level0: " + _level0.test);
  loadMovieNum ("FSCommand:quit", 0);
}

setTimeout (quit, 250);
