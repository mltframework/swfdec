// makeswf -v 7 -r 1 -o xml-constructor-properties-7.swf xml-constructor-properties.as

// Tests for when XML.prototype's native properties are created

var properties = [
  "__constructor__",
  "contentType",
  "docTypeDecl",
  "ignoreWhite",
  "loaded",
  "status",
  "xmlDecl"
];

var XMLReal = XML;
XML = String;

#if __SWF_VERSION__ == 5
XML.prototype.hasOwnProperty = ASnative (101, 5);
XMLReal.prototype.hasOwnProperty = ASnative (101, 5);
#endif

trace ("Before anything:");

for (var i = 0; i < properties.length; i++) {
  trace ("XML: " + properties[i] + ": " +
      XML.prototype.hasOwnProperty (properties[i]));
  trace ("XMLReal: " + properties[i] + ": " +
      XMLReal.prototype.hasOwnProperty (properties[i]));
}

var a = new XMLReal ();

trace ("After creating an XML object:");

for (var i = 0; i < properties.length; i++) {
  trace ("XML: " + properties[i] + ": " +
      XML.prototype.hasOwnProperty (properties[i]));
  trace ("XMLReal: " + properties[i] + ": " +
      XMLReal.prototype.hasOwnProperty (properties[i]));
}

for (var i = 0; i < properties.length; i++) {
  delete XML.prototype[properties[i]];
  delete XMLReal.prototype[properties[i]];
}

trace ("After deleting properties:");

for (var i = 0; i < properties.length; i++) {
  trace ("XML: " + properties[i] + ": " +
      XML.prototype.hasOwnProperty (properties[i]));
  trace ("XMLReal: " + properties[i] + ": " +
      XMLReal.prototype.hasOwnProperty (properties[i]));
}

var b = new XMLReal ();

trace ("After creating an XML object again:");

for (var i = 0; i < properties.length; i++) {
  trace ("XML: " + properties[i] + ": " +
      XML.prototype.hasOwnProperty (properties[i]));
  trace ("XMLReal: " + properties[i] + ": " +
      XMLReal.prototype.hasOwnProperty (properties[i]));
}

loadMovie ("FSCommand:quit", "");
