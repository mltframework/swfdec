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
	call_function.as \
	call_function.as.expect \
	comment.as \
	comment.as.expect \
	empty.as \
	empty.as.expect \
	empty_statement.as \
	empty_statement.as.expect \
	function_expression.as \
	function_expression.as.expect \
	if.as \
	if.as.expect \
	if_else.as \
	if_else.as.expect \
	trace.as \
	trace.as.expect \
	while.as \
	while.as.expect
