// makeswf -v 7 -r 1 -o array-no-object-7.swf array-no-object.as

trace ("Before deleting Array");
var a = [1, 2];
trace (a);
trace (a.__constructor__);
trace (a.length);

delete Array;

trace ("After deleting Array");
var b = [1, 2];
trace (b);
trace (b.__constructor__);
trace (b.length);

loadMovie ("FSCommand:quit", "");
