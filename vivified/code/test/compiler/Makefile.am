check-local: $(top_builddir)/vivified/code/vivi-compile
	RESULT=""; \
	for file in $(srcdir)/*.swf; do \
	  echo "$$file ..." && $(top_builddir)/vivified/code/vivi-compile $$file | diff -u $$file.expect - || RESULT="$$RESULT $$file"; \
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

EXTRA_DIST =

