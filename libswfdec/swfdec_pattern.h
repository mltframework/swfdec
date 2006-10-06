

#ifndef _SWFDEC_PATTERN_H_
#define _SWFDEC_PATTERN_H_

#include <glib-object.h>
#include <cairo.h>
#include <libswfdec/swfdec.h>
#include <libswfdec/color.h>

G_BEGIN_DECLS

typedef struct _SwfdecPattern SwfdecPattern;
typedef struct _SwfdecPatternClass SwfdecPatternClass;

#define SWFDEC_TYPE_PATTERN                    (swfdec_pattern_get_type())
#define SWFDEC_IS_PATTERN(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_PATTERN))
#define SWFDEC_IS_PATTERN_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_PATTERN))
#define SWFDEC_PATTERN(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_PATTERN, SwfdecPattern))
#define SWFDEC_PATTERN_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_PATTERN, SwfdecPatternClass))
#define SWFDEC_PATTERN_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_PATTERN, SwfdecPatternClass))

struct _SwfdecPattern
{
  GObject		object;

  cairo_matrix_t	transform;	/* user-to-pattern transform */
};

struct _SwfdecPatternClass
{
  GObjectClass		object_class;

  void			(* fill)		(SwfdecPattern *		pattern, 
					         cairo_t *			cr,
						 const SwfdecColorTransform *	trans,
						 unsigned int			ratio);
};

GType		swfdec_pattern_get_type		(void);

SwfdecPattern *	swfdec_pattern_new_color	(swf_color			color);
SwfdecPattern *	swfdec_pattern_parse		(SwfdecDecoder *		dec,
						 gboolean			rgba);

void		swfdec_pattern_fill		(SwfdecPattern *		pattern, 
						 cairo_t *			cr,
						 const SwfdecColorTransform *	trans,
						 unsigned int			ratio);

G_END_DECLS
#endif
