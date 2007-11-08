// makeswf -v 7 -r 1 -s 200x150 -o text-field-color-transform-7.swf text-field-color-transform.as

var c1 = new Color (this);
c1.setTransform ({ ra: 50, rb: 244, ga: 40, gb: 112, ba: 12, bb: 90, aa: 40, ab: 70});

this.createTextField ("t1", 0, 25, 25, 300, 300);

t1.border = true;
t1.background = true;
t1.backgroundColor = 0xAA0000;

this.createTextField ("t2", 1, 25, 50, 300, 300);

t2.border = true;
t2.background = true;
t2.backgroundColor = 0x00AA00;

this.createTextField ("t3", 2, 50, 25, 300, 300);

t2.border = true;
t3.background = true;
t3.backgroundColor = 0x0000AA;
