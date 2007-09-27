// makeswf -v 7 -r 1 -o test-7.swf test.as

var tests = [
  "a { a: 1; b: 2; }", // normal
  "a { a: 1; b: 2; } a { a: 0; c:3; }", // overwrite
  "a, a { a: 1; }", // same selector twice
  ",a,, ,, c,, { a: 1; }, b, ,,{ b: 2; }", // extra chars in selector
  "a { a: 1   ; }", // space before ;
  "a { a: 1 } b { b: 2   } c { c: 3}", // missing ;
  "a { a: 1; b; }" // extra ;
];

var style = new TextField.StyleSheet ();

for (var i = 0; i < tests.length; i++)
{
  if (i > 0)
    trace ("");
  trace ("Parsing: '" + tests[i] + "'");

  trace (style.parseCSS (tests[i]));

  var names = style.getStyleNames ();
  names.sort ();
  for (var j = 0; j < names.length; j++) {
    trace ("* '" + names[j] + "'");

    var o = style.getStyle (names[j]);

    var props = [];
    for (var prop in o) {
      props.push (prop);
    }
    props.sort ();
    for (var k = 0; k < props.length; k++) {
      var prop = props[k];
      trace (prop + ": '" + o[prop] + "'");
    }
  }
}

loadMovie ("FSCommand:quit", "");
