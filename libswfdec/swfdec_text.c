
#include <swfdec_text.h>
#include "swfdec_internal.h"
#include <swfdec_font.h>
#include <swfdec_render.h>


static void swfdec_text_base_init (gpointer g_class);
static void swfdec_text_class_init (SwfdecTextClass * g_class);
static void swfdec_text_init (SwfdecText * text);
static void swfdec_text_dispose (SwfdecText * text);

SWFDEC_OBJECT_BOILERPLATE (SwfdecText, swfdec_text)

     static void swfdec_text_base_init (gpointer g_class)
{

}

static void
swfdec_text_class_init (SwfdecTextClass * g_class)
{
  //SWFDEC_OBJECT_CLASS (g_class)->render = swfdec_text_render;
}

static void
swfdec_text_init (SwfdecText * text)
{
  text->glyphs = g_array_new (FALSE, TRUE, sizeof (SwfdecTextGlyph));
}

static void
swfdec_text_dispose (SwfdecText * text)
{
  g_array_free (text->glyphs, TRUE);
}


