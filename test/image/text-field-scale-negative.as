// makeswf -v 7 -s 200x150 -r 1 -o text-field-scale-negative.swf text-field-scale-negative.as

function create (depth, x, y) {
  var t = createTextField ("t" + depth, depth, x, y, 50, 40);
  var t = this["t" + depth];
  t.background = true;
  t.backgroundColor = 0xFF;
};

create (0, 25, 25);
create (1, 100.5, 25);
t1._xscale = -100;
t1._x += 50;
create (2, 25, 75.4);
t2._yscale = -100;
t2._y += 40;
create (3, 100.5, 75.5);
t3._xscale = -100;
t3._x += 50;
t3._yscale = -100;
t3._y += 40;

