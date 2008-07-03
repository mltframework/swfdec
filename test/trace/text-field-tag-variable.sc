.flash bbox=100x150 filename="text-field-tag-variable.swf" background=white version=8 fps=15

.frame 1
  .edittext text1 width=100 height=100 text="Hello1" variable="variable1"
  .put text1 x=0 y=0
  .edittext text2 width=100 height=100 html text="Hello2" variable="variable2"
  .put text2 x=0 y=0
  .action:
    trace (text1.variable);
    trace (_root.variable1);
    trace (text2.variable);
    trace (_root.variable2);
    getURL ('fscommand:quit', '');
  .end

.end
