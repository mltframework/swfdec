check-local: ../vivi-decompile
	RESULT=0; \
	for file in $(srcdir)/*.swf; do \
	  ../vivi-decompile $$file | diff -u - $$file.expect || RESULT=1; \
	done; \
	exit $$RESULT

EXTRA_DIST = \
	hello-world.as \
	hello-world.swf \
	hello-world.swf.trace \
	if.as \
	if.swf \
	if.swf.expect

