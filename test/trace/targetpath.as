// makeswf -v 7 -s 200x150 -r 1 -o targetpath.swf targetpath.as

asm {
  push "_root"
  getvariable
  targetpath
  trace
};
createEmptyMovieClip ("foo", 0);
asm {
  push "foo"
  getvariable
  targetpath
  trace
};
foo.createEmptyMovieClip ("foo", 0);
asm {
  push "foo"
  getvariable
  push "foo"
  getmember
  targetpath
  trace
};


loadMovie ("FSCommand:quit", "");
