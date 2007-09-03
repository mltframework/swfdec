// makeswf -v 7 -r 1 -o xml-node-7.swf xml-node.as

#include "values.as"

// Test XMLNode issues that are not handled by xml-parse test

function test_childNodes ()
{
  var x = new XML ("<root><a/><b/>c<d/></root>");
  var root = x.firstChild;

  // add fake elements to childNodes array and test how they effect the methods
  // and what methods reset the array
  trace (root);
  trace (root.childNodes);
  root.childNodes.push ("fake");
  trace (root.lastChild);
  trace (root);
  trace (root.childNodes);
  root.firstChild.removeNode ();
  trace (root.lastChild);
  trace (root.childNodes);
  root.childNodes.push (new XMLNode (1, "fake"));
  trace (root.lastChild);
  trace (root.childNodes);
  root.appendChild (new XMLNode (1, "e"));
  trace (root.childNodes);
  ASSetPropFlags (root.childNodes, "0", 0, 4); // remove write-protection
  root.childNodes[0] = new XMLNode (3, "fake");
  trace (root.firstChild);
  trace (root.childNodes);
  root.insertBefore (new XMLNode (1, "f"), root.lastChild);
  trace (root.childNodes);
}

function test_references ()
{
  var x = new XML ("<parent><previoussibling/><root><firstchild/><middlechild/><lastchild/></root><nextsibling/></parent>");
  var root = x.firstChild.firstChild.nextSibling;

  // normal
  trace (root.parentNode.nodeName);
  trace (root.firstChild.nodeName);
  trace (root.lastChild.nodeName);
  trace (root.nextSibling.nodeName);
  trace (root.previousSibling.nodeName);

  // writing
  root.parentNode = new XMLNode (1, "test");
  root.firstChild = new XMLNode (1, "test");
  root.lastChild = new XMLNode (1, "test");
  root.nextSibling = new XMLNode (1, "test");
  root.previousSibling = new XMLNode (1, "test");
  trace (root.parentNode.nodeName);
  trace (root.firstChild.nodeName);
  trace (root.lastChild.nodeName);
  trace (root.nextSibling.nodeName);
  trace (root.previousSibling.nodeName);

  // not found
  root = new XMLNode (1, "root");
  trace (root.parentNode);
  trace (root.firstChild);
  trace (root.lastChild);
  trace (root.nextSibling);
  trace (root.previousSibling);
}

function test_cloneNode ()
{
  var x = new XML ("<root><a/><b/>c<d/></root>");
  var root = x.firstChild;

  // normal
  var root2 = root.cloneNode ();
  trace (root2.childNodes);
  root2 = root.cloneNode (false);
  trace (root2.childNodes);
  root2 = root.cloneNode (true);
  trace (root2.childNodes);

  // fake elements in childNodes array
  root.childNodes.push (new XMLNode (1, "fake"));
  root2 = root.cloneNode (true);
  trace (root2.childNodes);
}

function test_nodeName ()
{
  var a = new XMLNode (1, "a:b");

  // normal
  trace (a.nodeName);
  trace (a.prefix);
  trace (a.localName);
  a.nodeName = "c:d";
  trace (a.nodeName);
  trace (a.prefix);
  trace (a.localName);
  // these are read-only
  a.prefix = "e";
  trace (a.nodeName);
  trace (a.prefix);
  trace (a.localName);
  a.localName = "f";
  trace (a.nodeName);
  trace (a.prefix);
  trace (a.localName);

  // weird values
  var testvalues = ["", ":", ":b", "a:", "a::b", "a::", "::b", "::" ];
  testvalues = testvalues.concat (values);
  for (var i = 0; i < testvalues.length; i++) {
    a.nodeName = testvalues[i];
    trace (a.nodeName);
    trace (a.prefix);
    trace (a.localName);
  }
}

test_childNodes ();
test_references ();
test_cloneNode ();
test_nodeName ();

loadMovie ("FSCommand:quit", "");
