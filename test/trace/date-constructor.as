// makeswf -v 7 -r 1 -o date-constructor-7.swf date-constructor.as

trace (Date ());
trace (new Date ());

trace (Date (1983, 11, 9));
trace (new Date (1983, 11, 9));

trace (Date (1983, 11, 9, 22, 43, 54, 89));
trace (new Date (1983, 11, 9, 22, 43, 54, 89));

loadMovie ("FSCommand:quit", "");
