SUBDIRS = test

noinst_LTLIBRARIES = libvivified-compiler-lex.la libvivified-compiler.la

# we create own .la for lex generated code, so we can disable some warnings
libvivified_compiler_lex_la_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS) -Wno-switch-default -Wno-missing-declarations -Wno-missing-prototypes -Wno-sign-compare -Wno-unused-function -Wno-missing-noreturn
libvivified_compiler_lex_la_LDFLAGS = $(SWFDEC_LIBS)

libvivified_compiler_lex_la_SOURCES = \
	vivi_compiler_scanner_lex.l

libvivified_compiler_la_CFLAGS = $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS)
libvivified_compiler_la_LDFLAGS = $(SWFDEC_LIBS)

libvivified_compiler_la_LIBADD = \
	libvivified-compiler-lex.la

libvivified_compiler_la_SOURCES = \
	vivi_code_assignment.c \
	vivi_code_binary.c \
	vivi_code_block.c \
	vivi_code_break.c \
	vivi_code_comment.c \
	vivi_code_constant.c \
	vivi_code_continue.c \
	vivi_code_function.c \
	vivi_code_function_call.c \
	vivi_code_get.c \
	vivi_code_get_url.c \
	vivi_code_goto.c \
	vivi_code_if.c \
	vivi_code_init_array.c \
	vivi_code_init_object.c \
	vivi_code_label.c \
	vivi_code_loop.c \
	vivi_code_printer.c \
	vivi_code_return.c \
	vivi_code_statement.c \
	vivi_code_text_printer.c \
	vivi_code_throw.c \
	vivi_code_token.c \
	vivi_code_trace.c \
	vivi_code_unary.c \
	vivi_code_value.c \
	vivi_code_value_statement.c \
	vivi_compiler.c \
	vivi_compiler_empty_statement.c \
	vivi_compiler_get_temporary.c \
	vivi_compiler_goto_name.c \
	vivi_compiler_scanner.c \
	vivi_decompiler_block.c \
	vivi_decompiler_duplicate.c \
	vivi_decompiler_state.c \
	vivi_decompiler_unknown.c \
	vivi_decompiler.c

noinst_HEADERS = \
	vivi_code_assignment.h \
	vivi_code_binary.c \
	vivi_code_block.h \
	vivi_code_break.h \
	vivi_code_comment.h \
	vivi_code_constant.h \
	vivi_code_continue.h \
	vivi_code_function.h \
	vivi_code_function_call.h \
	vivi_code_get.h \
	vivi_code_get_url.h \
	vivi_code_goto.h \
	vivi_code_if.h \
	vivi_code_init_array.h \
	vivi_code_init_object.h \
	vivi_code_label.h \
	vivi_code_loop.h \
	vivi_code_printer.h \
	vivi_code_return.h \
	vivi_code_statement.h \
	vivi_code_text_printer.h \
	vivi_code_throw.h \
	vivi_code_token.h \
	vivi_code_trace.h \
	vivi_code_unary.h \
	vivi_code_value.h \
	vivi_code_value_statement.h \
	vivi_compiler.h \
	vivi_compiler_empty_statement.h \
	vivi_compiler_get_temporary.h \
	vivi_compiler_goto_name.h \
	vivi_compiler_scanner.h \
	vivi_compiler_scanner_lex.h \
	vivi_compiler_scanner_lex_include.h \
	vivi_decompiler.h \
	vivi_decompiler_block.h \
	vivi_decompiler_duplicate.h \
	vivi_decompiler_state.h
	vivi_decompiler_unknown.h \
	vivified-compiler.h

noinst_PROGRAMS = vivi-decompile vivi-compile

vivi_decompile_SOURCES = \
	decompiler.c

vivi_decompile_CFLAGS =  $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS)
vivi_decompile_LDFLAGS = $(SWFDEC_LIBS)
vivi_decompile_LDADD = libvivified-compiler.la

vivi_compile_SOURCES = \
	compiler.c

vivi_compile_CFLAGS =  $(GLOBAL_CFLAGS) $(SWFDEC_CFLAGS)
vivi_compile_LDFLAGS = $(SWFDEC_LIBS)
vivi_compile_LDADD = libvivified-compiler.la
