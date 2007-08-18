// makeswf -v 7 -r 1 -o loadobject-7.swf loadobject.as

var obj = new Object ();

obj.load = ASnative (301, 0);

obj.onData = function (str) {
  trace ("Got: " + typeof (str) + " '" + str + "'");
  trace ("Loaded: " + this._bytesLoaded);
  trace ("Total: " + this._bytesTotal);
};

trace ("Loaded: " + obj._bytesLoaded);
trace ("Total: " + obj._bytesTotal);
trace (obj.load ());
trace ("Loaded: " + obj._bytesLoaded);
trace ("Total: " + obj._bytesTotal);
trace (obj.load ("loadobject.as"));
trace ("Loaded: " + obj._bytesLoaded);
trace ("Total: " + obj._bytesTotal);
trace (obj.load ("404"));
trace ("Loaded: " + obj._bytesLoaded);
trace ("Total: " + obj._bytesTotal);

function quit () {
  loadMovie ("FSCommand:quit", "");
};

setInterval (quit, 1000);
