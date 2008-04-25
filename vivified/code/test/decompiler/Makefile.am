check-local: $(top_builddir)/vivified/code/vivi-decompile
	RESULT=""; \
	for file in $(srcdir)/*.swf; do \
	  echo "$$file ..." && $(top_builddir)/vivified/code/vivi-decompile $$file | diff -u $$file.expect - || RESULT="$$RESULT $$file"; \
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
	and-or.as \
	and-or.swf \
	and-or.swf.expect \
	function-calls.as \
	function-calls.swf \
	function-calls.swf.expect \
	hello-world.as \
	hello-world.swf \
	hello-world.swf.expect \
	if.as \
	if.swf \
	if.swf.expect \
	if-nested.as \
	if-nested.swf \
	if-nested.swf.expect \
	loop-empty.swf \
	loop-empty.swf.expect \
	loop-empty.xml \
	loop-infinite.swf \
	loop-infinite.swf.expect \
	loop-infinite.xml \
	loop-nested.as \
	loop-nested.swf \
	loop-nested.swf.expect \
	simple-loop.as \
	simple-loop.swf \
	simple-loop.swf.expect

