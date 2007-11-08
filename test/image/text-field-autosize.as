// makeswf -v 7 -r 1 -s 200x150 -o text-field-autosize-7.swf text-field-autosize.as

this.createTextField ("t1", 0, 25, 25, 100, 100);

t1.autoSize = "left";
t1.background = true;
t1.backgroundColor = 0xFF0000;

this.createTextField ("t2", 1, 25, 25, 100, 100);

t2.autoSize = "center";
t2.background = true;
t2.backgroundColor = 0x00FF00;

this.createTextField ("t3", 2, 25, 25, 100, 100);

t3.autoSize = "right";
t3.background = true;
t3.backgroundColor = 0x0000FF;
