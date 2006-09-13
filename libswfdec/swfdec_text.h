
#ifndef _SWFDEC_TEXT_H_
#define _SWFDEC_TEXT_H_

#include "swfdec_types.h"
#include <swfdec_object.h>

G_BEGIN_DECLS
//typedef struct _SwfdecText SwfdecText;
typedef struct _SwfdecTextClass SwfdecTextClass;

//typedef struct _SwfdecTextGlyph SwfdecTextGlyph;
typedef struct _SwfdecTextChunk SwfdecTextChunk;

#define SWFDEC_TYPE_TEXT                    (swfdec_text_get_type())
#define SWFDEC_IS_TEXT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_TEXT))
#define SWFDEC_IS_TEXT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_TEXT))
#define SWFDEC_TEXT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_TEXT, SwfdecText))
#define SWFDEC_TEXT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_TEXT, SwfdecTextClass))

struct _SwfdecTextGlyph
{
  int x, y;
  int glyph;
  int font;
  int height;
  unsigned int color;
};

struct _SwfdecText
{
  SwfdecObject object;

  GArray *glyphs;
  cairo_matrix_t transform;
};

struct _SwfdecTextClass
{
  SwfdecObjectClass object_class;

};

GType swfdec_text_get_type (void);

int tag_func_define_font (SwfdecDecoder * s);
int tag_func_define_font_2 (SwfdecDecoder * s);
int tag_func_define_text (SwfdecDecoder * s);
int tag_func_define_text_2 (SwfdecDecoder * s);

void swfdec_font_free (SwfdecObject * object);

G_END_DECLS
#endif
