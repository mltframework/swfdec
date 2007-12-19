// makeswf -v 7 -r 1 -s 200x150 -o text-field-empty-7.swf text-field-empty.as

this.createTextField ("t1", 0, 25, 25, 50, 50);

t1.background = true;
t1.backgroundColor = 0x00FF00;

this.createTextField ("t2", 1, 45, 45, 95, 95);

t2.border = true;
t2.borderColor = 0xFF0000;
t2.background = true;
t2.backgroundColor = 0x0000FF;

// this one won't show up at all
this.createTextField ("t3", 2, 75, 75, 50, 50);
