
#ifndef _SWFDEC_FONT_H_
#define _SWFDEC_FONT_H_

#include <swfdec_types.h>
#include <swfdec_object.h>

G_BEGIN_DECLS
//typedef struct _SwfdecFont SwfdecFont;
typedef struct _SwfdecFontClass SwfdecFontClass;

#define SWFDEC_TYPE_FONT                    (swfdec_font_get_type())
#define SWFDEC_IS_FONT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_FONT))
#define SWFDEC_IS_FONT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_FONT))
#define SWFDEC_FONT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_FONT, SwfdecFont))
#define SWFDEC_FONT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_FONT, SwfdecFontClass))

struct _SwfdecFont
{
  SwfdecObject object;

  GPtrArray *glyphs;
};

struct _SwfdecFontClass
{
  SwfdecObjectClass object_class;

};

GType swfdec_font_get_type (void);

SwfdecShape *swfdec_font_get_glyph (SwfdecFont * font, int glyph);


G_END_DECLS
#endif
