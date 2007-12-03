// makeswf -v 7 -s 200x150 -r 1 -o gradient-beginGradientFill.swf gradient-beginGradientFill.as

x = 0;
y = 0;

function do_gradient (type, colors, alphas, ratios) {
  var matrix = { a:0, b:-80, d:80, e:0, g: x * 100 + 50, h: y * 100 + 50 };
  beginGradientFill (type, colors, alphas, ratios, matrix);
  moveTo (x * 100 + 10, y * 100 + 10);
  lineTo (x * 100 + 10, y * 100 + 90);
  lineTo (x * 100 + 90, y * 100 + 90);
  lineTo (x * 100 + 90, y * 100 + 10);
  lineTo (x * 100 + 10, y * 100 + 10);
  endFill ();
  x++;
  if (x > 3) {
    x = 0;
    y++;
  }
};

// 1
foo = function (o, s) {
  if (!o.x)
    o.x = 7;
  else
    o.x--;
  trace (s + " valueof: " + o.x); 
  return o.x;
};
c = { };
c.length = { valueOf: function () { return foo (this, "color"); } };
c[0] = c[2] = c[4] = c[6] = c[8] = 0xFF;
c[1] = c[3] = c[5] = c[7] = c[9] = 0xFF00;
a = { };
a.length = { valueOf: function () { return foo (this, "alpha"); } };
a[0] = a[2] = a[4] = a[6] = a[8] = 100;
a[1] = a[3] = a[5] = a[7] = a[9] = 50;
r = { };
r.length = { valueOf: function () { return foo (this, "ratios"); } };
for (i = 0; i < 10; i++)
  r[i] = i / 9 * 255;
do_gradient ("linear", c, a, r);

// 2 - 4
o = { };
o.length = { valueOf: function () { trace ("length valueOf"); return 3; } };
do_gradient ("linear", [0x40, 0xFF0000, 0xFF00], o, [0, 127, 255]);
do_gradient ("linear", [0xFF, 0xFF0000, 0xFF00], [100, 100, 100], o);
do_gradient ("linear", o, [100, 50, 100], [0, 127, 255]);

// 5 - 12
do_gradient ("Radial", [0xFF, 0xFF0000, 0xFF00], [255, 255, 255], [0, 127, 255]);
do_gradient ("linear", ["255", {valueOf: function () { return 0xFF0000; }}, 0xFF00], [255, 255, 255], [0, 127, 255]);
do_gradient ("linear", [ 0xFF, 0xFF00 ], [ undefined, 127 ], [ 0, 255 ]);
do_gradient ("linear", [ ], [ ], [ ]);
do_gradient ("linear", [ 0xFF ], [ 100 ], [ -1 ]);
do_gradient ("linear", [ undefined ], [ 100 ], [ 255.8 ]);
do_gradient ("linear", [ 0xFF ], [ 100 ], [ 256 ]);
do_gradient ("radial", [0xFF, 0xFF0000, 0xFF00], [50, 100, 255], [undefined, 127, 255]);
