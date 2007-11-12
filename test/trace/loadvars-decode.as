// makeswf -v 7 -s 200x150 -r 1 -o loadvars-decode.swf loadvars-decode.as

test = function (encoded) {
  // print something about the test we're about to run
  trace (">>> " + encoded);

  // create a new Object.
  o = {};

  // get a special function that we know is used to decode urlencoded data.
  // This is actually the function used as LoadVars.prototype.decode, but it
  // works on any type of object.
  o.decode = ASnative (301, 3);

  // decode the string we got
  o.decode (encoded);

  // now print the properties that got set. Note that we can't just use a for-in
  // loop, because they have an undefined order. Instead we collect all 
  // variables that were set into an array, sort that array, and then print the
  // data sorted
  var array = [];
  for (var i in o) {
    array.push (i);
  };
  array.sort ();
  for (var i = 0; i < array.length; i++) {
    trace (array[i] + " = " + o[array[i]]);
  };
};

// Let's create an array of all the strings we want to test
tests = [
  "a=b&c=d",
  "%26=%3d&%3D=%26",
  "",
  "???=???",
  "a=",
  "=b",
  "a=&=b&c&d=e"
  //add more here :)
];

for (i = 0; i < tests.length; i++) {
  test (tests[i]);
};


loadMovie ("fscommand:quit", "");
