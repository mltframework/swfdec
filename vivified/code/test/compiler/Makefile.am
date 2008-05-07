check-local: $(top_builddir)/vivified/code/vivi-compile
	RESULT=""; \
	for file in $(srcdir)/*.as; do \
	  echo "$$file ..." && $(top_builddir)/vivified/code/vivi-compile -a $$file | diff -u $$file.expect - || RESULT="$$RESULT $$file"; \
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
	builtin_statement.as \
	builtin_statement.as.expect \
	comment.as \
	comment.as.expect \
	empty.as \
	empty.as.expect \
	empty_statement.as \
	empty_statement.as.expect \
	trace.as \
	trace.as.expect
