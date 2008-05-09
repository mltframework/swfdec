// makeswf -v 7 -s 200x150 -r 1 -o movie23.swf movie23.as

function create (depth, x, y) {
  var t = createTextField ("t" + depth, depth, x, y, 50, 25);
  var t = this["t" + depth];
  t.background = true;
  t.backgroundColor = 0xFF0000;
};

create (0, 25, 25);
t0.border = true;
create (1, 100.5, 25);
create (2, 25, 75);
t2._y += 0.4;
create (3, 100.5, 75);
t3._y += 0.5;
t3.border = true;

