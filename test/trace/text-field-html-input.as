// makeswf -v 7 -r 1 -o test-7.swf test.as

var properties = [
  "align",
  "blockIndent",
  "bold",
  "bullet",
  "color",
  "display",
  "font",
  "indent",
  "italic",
  "kerning",
  "leading",
  "leftMargin",
  "letterSpacing",
  "rightMargin",
  "size",
  "tabStops",
  "target",
  "underline",
  "url"
];

function format_to_string (fmt) {
  str = "";
  for (var i = 0; i < properties.length; i++) {
    str += " " + properties[i] + "=" + fmt[properties[i]];
  }
  return str;
}

this.createTextField ("t", 1, 0, 0, 200, 200);

var texts = [
  "a",
  "a\rb",
  "a\r\nb",
  "a\r\rb",
  "a<p align='right'>b</p>c",
  "a<p align='right'>ä</p>c",
  "a<!-- b -->c",
  "a<!--->b",
  "a<br><li>b</li>c<p>d</p>e<br>"
];

t.html = true;

for (var i = 0; i < texts.length; i++) {
  for (var j = 0; j <= 3; j++) {
    t.multiline = j & 1;
    t.condenseWhite = j & 2;
    t.htmlText = texts[i];

    trace (i + ": multiline: " + t.multiline + " condenseWhite: " + t.condenseWhite);

    trace (t.text);
    trace (t.htmlText);

    for (var k = 0; k < t.length; k++) {
      trace (k + "/" + t.text.charAt (k) + ":" + format_to_string (t.getTextFormat (k)));
    }
  }
}

loadMovie ("FSCommand:quit", "");
