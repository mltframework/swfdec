// makeswf -v 7 -s 200x150 -r 1 -o invalid-varable-name.swf invalid-varable-name.as

trace ("Check invalid variable names don't work");

this[""] = 42;
trace (this [""]);

loadMovie ("FSCommand:quit", "");
