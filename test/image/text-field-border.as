// makeswf -v 7 -s 200x150 -r 1 -o text-field-border.swf text-field-border.as

function create (depth, x, y) {
  var t = createTextField ("t" + depth, depth, x, y, 50, 25);
  t.border = true;
  t.borderColor = 0;
};

create (0, 25, 25);
create (1, 100.5, 25);
create (2, 25, 75);
t2._y += 0.4;
create (3, 100.5, 75);
t3._y += 0.5;

