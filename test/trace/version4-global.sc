.flash bbox=150x200 filename="version4-global.swf" background=white version=4 fps=15

.frame 1
  .action:
    trace (_global);
    trace (ASSetNative);
    trace (ASSetNativeAccessor);
    trace (ASSetPropFlags);
    trace (ASconstructor);
    trace (ASnative);
    trace (Accessibility);
    trace (Array);
    trace (AsBroadcaster);
    trace (AsSetupError);
    trace (AssetCache);
    trace (Boolean);
    trace (Button);
    trace (Camera);
    trace (Color);
    trace (ContextMenu);
    trace (ContextMenuItem);
    trace (Date);
    trace (Error);
    trace (Function);
    trace (Infinity);
    trace (Key);
    trace (LoadVars);
    trace (LocalConnection);
    trace (Math);
    trace (Microphone);
    trace (Mouse);
    trace (MovieClip);
    trace (MovieClipLoader);
    trace (NaN);
    trace (NetConnection);
    trace (NetStream);
    trace (Number);
    trace (Object);
    trace (PrintJob);
    trace (RemoteLSOUsage);
    trace (Selection);
    trace (SharedObject);
    trace (Sound);
    trace (Stage);
    trace (String);
    trace (System);
    trace (TextField);
    trace (TextFormat);
    trace (TextSnapshot);
    trace (Video);
    trace (XML);
    trace (XMLNode);
    trace (XMLSocket);
    trace (clearInterval);
    trace (clearTimeout);
    trace (enableDebugConsole);
    trace (escape);
    trace (flash);
    trace (isFinite);
    trace (isNaN);
    trace (o);
    trace (parseFloat);
    trace (parseInt);
    trace (setInterval);
    trace (setTimeout);
    trace (showRedrawRegions);
    trace (textRenderer);
    trace (unescape);
    trace (updateAfterEvent);
    getURL("fscommand:quit", "");
  .end

.end
