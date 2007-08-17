// makeswf -v 7 -r 1 -o test-7.swf test.as

function hidden_properties (obj)
{
  normal = new Array ();
  for (prop in obj) {
    normal.push (prop);
  }

  hidden = new Array ();
  ASSetPropFlags (obj, null, 0, 1);
  for (prop in obj) {
    for (i = 0; i < normal.length; i++) {
      if (normal[i] == prop)
	break;
    }
    if (i == normal.length)
      hidden.push (prop);
  }
  ASSetPropFlags (obj, hidden, 1, 0);

  return hidden.sort ();
}

// loses flags from the properties that are not permanent
function permanent_properties (obj)
{
  hidden = hidden_properties (obj);
  constant = constant_properties (obj);

  ASSetPropFlags (obj, hidden, 0, 1);

  permanent = new Array();
  for (var prop in obj) {
    var old = obj[prop];
    delete obj[prop];
    if (obj.hasOwnProperty (prop)) {
      permanent.push (prop);
    } else {
      obj[prop] = old;
    }
  }

  ASSetPropFlags (obj, hidden, 1, 0);
  ASSetPropFlags (obj, constant, 3, 0);

  return permanent.sort ();
}

function constant_properties (obj)
{
  hidden = hidden_properties (obj);

  ASSetPropFlags (obj, hidden, 0, 1);

  constant = new Array();
  for (var prop in obj) {
    var old = obj[prop];
    var val = "hello " + obj[prop];
    obj[prop] = val;
    if (obj[prop] != val) {
      constant.push (prop);
    } else {
      obj[prop] = old;
    }
  }

  ASSetPropFlags (obj, hidden, 1, 0);

  return constant.sort ();
}

var obj = new Object ();
obj[0] = 0;
for (var i = 1; i <= 7; i++) {
  obj[i] = i;
  ASSetPropFlags (obj, i, i, 0);
}
trace ("Hidden: " + hidden_properties (obj));
trace ("Constant: " + constant_properties (obj));
trace ("Permanent: " + permanent_properties (obj));

loadMovie ("FSCommand:quit", "");
