// makeswf -v 7 -r 1 -o text-field-values-7.swf text-field-values.as

#include "values.as"

// textColor is limited between 0 and 2^24-1
// type can take "input" or "dynamic"
values.push (17000000);
names.push ("(" + (values.length - 1) + ") 17000000 (number)");
values.push (-17000000);
names.push ("(" + (values.length - 1) + ") -17000000 (number)");
values.push ("input");
names.push ("(" + (values.length - 1) + ") input (string)");
values.push (34000000);
names.push ("(" + (values.length - 1) + ") 34000000 (number)");
values.push (new TextField.StyleSheet ());
names.push ("(" + (values.length - 1) + ") " + values[values.length - 1] + " (StyleSheet)");
values.push (-34000000);
names.push ("(" + (values.length - 1) + ") -34000000 (number)");
values.push ("dynamic");
names.push ("(" + (values.length - 1) + ") input (dynamic)");

// Won't test here:
//  length
//  textHeight
//  textWidth
//  bottomScroll
//  hscroll
//  maxhscroll
//  maxscroll
//  scroll

var properties = [
  // text
  "text",
  "html",
  "htmlText",

  // input
  "condenseWhite",
  "maxChars",
  "multiline",
  "restrict",
  "selectable",
  //"tabEnabled",
  //"tabIndex",
  "type",
  "variable",

  // border & background
  "background",
  "backgroundColor",
  "border",
  "borderColor",

  // scrolling
  "mouseWheelEnabled",

  // display
  "autoSize",
  "password",
  "wordWrap",

  // format
  //"antiAliasType",
  "embedFonts",
  //"gridFitType",
  //"sharpness",
  "styleSheet",
  "textColor"//,
  //"thickness",

  // misc
  //"menu",
  //"filters"
  ];

this.createTextField ("field", 1, 0, 0, 50, 50);

for (var i = 0; i < properties.length; i++) {
  var prop = properties[i];
  trace ("Testing: " + prop + " (default: " + field[prop] + ")");
  for (var j = 0; j < values.length; j++) {
    field[prop] = values[j];
    trace (prop + ": " + names[j] + " = " + field[prop] + " (" +
	typeof (field[prop]) + ")");
  }
}

loadMovie ("FSCommand:quit", "");
