// makeswf -v 7 -r 1 -o xml-escape-7.swf xml-escape.as

// xmlEscape method is only available as ASnative

var xmlEscape = ASnative (100, 5);
trace (xmlEscape ("t\"'e&s;:''at"));
trace (xmlEscape ("te&lt;st"));
trace (xmlEscape ());
trace (xmlEscape ("ma'ny", "param&eters"));
trace (xmlEscape ("hmm&amp;hrr"));
// FIXME: Make it work in v5 too
#if __SWF_VERSION__ > 5
trace (xmlEscape ("non breaking space:  "));
#endif

loadMovie ("FSCommand:quit", "");
