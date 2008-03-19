check-local: ../vivi-decompile
	RESULT=""; \
	for file in $(srcdir)/*.swf; do \
	  ../vivi-decompile $$file | diff -u $$file.expect - || RESULT="$$RESULT $$file"; \
	done; \
	if test "x$$RESULT" == "x"; then \
	  echo "OK"; \
	else \
	  echo "FAILED:"; \
	  for fail in $$RESULT; do \
	    echo "  $$fail"; \
	  done; \
	  exit 1; \
	fi

EXTRA_DIST = \
	hello-world.as \
	hello-world.swf \
	hello-world.swf.trace \
	if.as \
	if.swf \
	if.swf.expect \
	if-nested.as \
	if-nested.swf \
	if-nested.swf.expect

