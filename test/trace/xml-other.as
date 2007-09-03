// makeswf -v 7 -r 1 -o xml-node-7.swf xml-node.as

#include "values.as"

// Test XML issues that are not handled by the other tests

function test_create ()
{
  var x = new XML ("<root/>");

  // normal
  trace (x);
  trace (x.createElement ("name"));
  trace (x);
  trace (x.createTextNode ("value"));
  trace (x);

  // weird parameters
  trace (x.createElement ());
  trace (x.createTextNode ());
  trace (x.createElement ("name", "name2"));
  trace (x.createTextNode ("value", "value2"));

  for (var i = 0; i < values.length; i++) {
    trace ("Creating with: " + names[i]);
    trace (x.createElement (values[i]));
    trace (x.createTextNode (values[i]));
  }
}

function test_properties ()
{
  var x = new XML ("<?xml XML declaration ?><!DOCTYPE doctype declaration ><root/>");

  // normal
  trace (x.contentType);
  trace (x.docTypeDecl);
  trace (x.ignoreWhite);
  trace (x.loaded);
  trace (x.status);
  trace (x.xmlDecl);

  // write
  for (var i = 0; i < values.length; i++) {
    trace ("Testing with: " + names[i]);
    x.contentType = values[i];
    trace (x.contentType);
    x.docTypeDecl = values[i];
    trace (x.docTypeDecl);
    x.ignoreWhite = values[i];
    trace (x.ignoreWhite);
    x.loaded = values[i];
    trace (x.loaded);
    x.status = values[i];
    trace (x.status);
    x.xmlDecl = values[i];
    trace (x.xmlDecl);
  }
}

test_create ();
test_properties ();

loadMovie ("FSCommand:quit", "");
