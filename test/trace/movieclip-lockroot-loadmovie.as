// makeswf -v 7 -s 200x150 -r 1 -o movieclip-lockroot-loadmovie.swf movieclip-lockroot-loadmovie.as

if (_level0.value == undefined) {
  _root.value = "parent";
  for (i = 5; i <= 8; i++) {
    var c = this.createEmptyMovieClip ("m" + i, i);
    c._lockroot = true;
    loadMovie ("movieclip-lockroot-loadmovie-" + i + ".swf", "m" + i);
  }
} else {
  trace (this._lockroot);
  trace (_root.value);
  getURL ("FSCommand:quit", "");
}
