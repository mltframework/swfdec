
#ifndef _SWFDEC_FONT_H_
#define _SWFDEC_FONT_H_

#include <pango/pangocairo.h>
#include <swfdec_types.h>
#include <swfdec_object.h>

G_BEGIN_DECLS
//typedef struct _SwfdecFont SwfdecFont;
typedef struct _SwfdecFontEntry SwfdecFontEntry;
typedef struct _SwfdecFontClass SwfdecFontClass;

typedef enum {
  SWFDEC_LANGUAGE_NONE		= 0,
  SWFDEC_LANGUAGE_LATIN		= 1,
  SWFDEC_LANGUAGE_JAPANESE	= 2,
  SWFDEC_LANGUAGE_KOREAN	= 3,
  SWFDEC_LANGUAGE_CHINESE	= 4,
  SWFDEC_LANGUAGE_CHINESE_TRADITIONAL = 5
} SwfdecLanguage;

#define SWFDEC_TYPE_FONT                    (swfdec_font_get_type())
#define SWFDEC_IS_FONT(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_FONT))
#define SWFDEC_IS_FONT_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_FONT))
#define SWFDEC_FONT(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_FONT, SwfdecFont))
#define SWFDEC_FONT_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_FONT, SwfdecFontClass))

struct _SwfdecFontEntry {
  SwfdecShape *		shape;		/* shape to use as fallback */
  gunichar		value;		/* UCS2 value of glyph */
};

struct _SwfdecFont
{
  SwfdecObject		object;

  char *		name;		/* name of the font (FIXME: what name?) */
  PangoFontDescription *desc;
  gboolean		bold;		/* font is bold */
  gboolean		italic;		/* font is italic */
  gboolean		small;		/* font is rendered at small sizes */
  GArray *		glyphs;		/* key: glyph index, value: UCS2 character */
};

struct _SwfdecFontClass
{
  SwfdecObjectClass	object_class;
};

GType swfdec_font_get_type (void);

SwfdecShape *swfdec_font_get_glyph (SwfdecFont * font, unsigned int glyph);

int tag_func_define_font_info (SwfdecDecoder *s, unsigned int version);

G_END_DECLS
#endif
