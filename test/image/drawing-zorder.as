// makeswf -v 7 -s 200x150 -r 1 -o drawing-zorder.swf drawing-zorder.as

beginFill (0xFF);
lineStyle (10, 0xFF00);
moveTo (0, 0);
lineTo (0, 100);
lineTo (100, 100);
lineTo (100, 0);
lineTo (0, 0);
endFill ();
beginFill (0xFF0000);
moveTo (50, 50);
lineTo (50, 150);
lineTo (150, 150);
lineTo (150, 50);
lineTo (50, 50);
endFill ();

//loadMovie ("FSCommand:quit", "");
