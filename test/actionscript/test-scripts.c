#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include "swfdec_decoder.h"
#include "swfdec_compiler.h"
#include "swfdec_js.h"

static gboolean 
check_funcall (SwfdecDecoder *s)
{
  gboolean ret;
  g_object_get (s, "mouse-visible", &ret, NULL);
  if (ret)
    return FALSE;

  return TRUE;
}

struct {
  char *name;
  gboolean (* check) (SwfdecDecoder *);
} tests[] = {
  { "funcall", check_funcall }
};

gboolean
run_test (guint id)
{
  SwfdecDecoder *s;
  SwfdecBuffer *buffer;
  char *str;
  JSScript *script;
  GError *error = NULL;
  gboolean ret;

  g_print ("Testing %s:\n", tests[id].name);
  s = swfdec_decoder_new ();
  str = g_strdup_printf ("%s.as", tests[id].name);
  buffer = swfdec_buffer_new_from_file (str, &error);
  if (buffer == NULL) {
    g_free (str);
    g_print ("  ERROR: %s\n", error->message);
    g_error_free (error);
    g_object_unref (s);
    return FALSE;
  }
  g_free (str);
  s->b.buffer = buffer;
  s->b.ptr = buffer->data;
  s->b.idx = 0;
  s->b.end = buffer->data + buffer->length;
  s->parse_sprite = s->main_sprite;
  script = swfdec_compile (s);
  s->parse_sprite = NULL;
  if (script) {
    swfdec_js_execute_script (s, s->root, script);
    JS_DestroyScript (s->jscx, script);
    if (tests[id].check) {
      ret = tests[id].check (s);
      if (ret) {
	g_print ("  OK\n");
      } else {
	g_print ("  ERROR: Checks failed\n");
      }
    } else {
      ret = TRUE;
      g_print ("  OK\n");
    }
  } else {
    g_print ("  ERROR: Script compilation failed\n");
    ret = FALSE;
  }
  swfdec_buffer_unref (buffer);
  g_object_unref (s);
  return ret;
}

int
main (int argc, char **argv)
{
  guint i, failed_tests = 0;

  for (i = 0; i < G_N_ELEMENTS (tests); i++) {
    if (!run_test (i))
      failed_tests++;
  }

  if (failed_tests) {
    g_print ("\nFAILURES: %u\n", failed_tests);
  } else {
    g_print ("\nEVERYTHING OK\n");
  }
  return failed_tests;
}

