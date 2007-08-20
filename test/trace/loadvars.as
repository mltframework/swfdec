// makeswf -v 7 -r 1 -o loadvars-7.swf loadvars.as

//_global.unescape = function () { return "moi"; };
//_global.escape = function () { return "moi"; };

var lv = new LoadVars ();

trace (lv.onLoad);
trace (lv.loaded);

lv.onLoad = function (success) {
  trace (success);
  trace (this.loaded);
  trace (this._bytesLoaded);
  trace (this._bytesTotal);
  trace (this);
  trace ("--");
  var props = new Array ();
  //ASSetPropFlags (this, null, 0, 1);
  //ASSetPropFlags (this.__proto__, null, 0, 1);
  for (var prop in this) {
    props.push (prop);
  }
  props.sort ();
  for (var i = 0; i < props.length; i++) {
    trace (props[i] + " = " + this[props[i]]);
  }
};

lv.load ("params.txt");

function quit () {
  loadMovie ("FSCommand:quit", "");
};

setInterval (quit, 1000);
