// makeswf -v 7 -r 1 -o xml-properties-7.swf xml-properties.as

#include "trace_properties.as"

function trace_xml_node_properties (o, prefix, identifier)
{
  trace_properties (o, prefix, identifier);
  trace_properties (o.attributes, "local.o", "attributes");
  trace_properties (o.childNodes, "local.o", "childNodes");
  trace_properties (o.firstChild, "local.o", "firstChild");
  trace_properties (o.lastChild, "local.o", "lastChild");
  trace_properties (o.localName, "local.o", "localName");
  trace_properties (o.namespaceURI, "local.o", "namespaceURI");
  trace_properties (o.nextSibling, "local.o", "nextSibling");
  trace_properties (o.nodeName, "local.o", "nodeName");
  trace_properties (o.nodeType, "local.o", "nodeType");
  trace_properties (o.nodeValue, "local.o", "nodeValue");
  trace_properties (o.parentNode, "local.o", "parentNode");
  trace_properties (o.prefix, "local.o", "prefix");
  trace_properties (o.previousSibling, "local.o", "previousSibling");
}

function trace_xml_properties (o, prefix, identifier)
{
  trace_xml_node_properties (o, prefix, identifier);
  trace_properties (o.contentType, "local.o", "contentType");
  trace_properties (o.docTypeDecl, "local.o", "docTypeDecl");
  trace_properties (o.ignoreWhite, "local.o", "ignoreWhite");
  trace_properties (o.loaded, "local.o", "loaded");
  trace_properties (o.status, "local.o", "status");
  trace_properties (o.xmlDecl, "local.o", "xmlDecl");
}

var a = new XMLNode (1, "element");
var b = new XMLNode (3, "text");
var c = new XMLNode ();
var d = new XML ("<element attribute='value'>text</element>");
var e = d.firstChild;
var f = new XML ();

// need to do instances first, because tracing the objects will do evil things
// to the native properties
trace_xml_node_properties (a, "local", "a");
trace_xml_node_properties (b, "local", "b");
trace_xml_node_properties (c, "local", "c");
trace_xml_properties (d, "local", "d");
trace_xml_node_properties (e, "local", "e");
trace_xml_properties (f, "local", "f");

// Missing send etc. and version 6 has extra constructor in prototype
//trace_properties (_global.XML, "_global", "XML");
trace_properties (_global.XMLNode, "_global", "XMLNode");
// Not implemented
//trace_properties (_global.XMLSocket, "_global", "XMLSocket");

loadMovie ("FSCommand:quit", "");
