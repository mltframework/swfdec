// makeswf -v 7 -r 1 -o movieclip-get-swf-version-load-7.swf movieclip-get-swf-version-load.as

for (var i = 5; i <= 8; i++) {
  this.createEmptyMovieClip ("m"+i, i);
  this["m"+i].loadMovie ("movieclip-get-swf-version-" + i + ".swf");
}

function quit () {
  loadMovie ("FSCommand:quit", "");
}

setTimeout (quit, 1500);
