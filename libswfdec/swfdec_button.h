
#ifndef _SWFDEC_BUTTON_H_
#define _SWFDEC_BUTTON_H_

#include <swfdec_object.h>
#include <color.h>
#include <swfdec_event.h>

G_BEGIN_DECLS
//typedef struct _SwfdecButton SwfdecButton;
typedef struct _SwfdecButtonClass SwfdecButtonClass;
typedef struct _SwfdecButtonRecord SwfdecButtonRecord;

#define SWFDEC_TYPE_BUTTON                    (swfdec_button_get_type())
#define SWFDEC_IS_BUTTON(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_BUTTON))
#define SWFDEC_IS_BUTTON_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_BUTTON))
#define SWFDEC_BUTTON(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_BUTTON, SwfdecButton))
#define SWFDEC_BUTTON_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_BUTTON, SwfdecButtonClass))

/* these values have to be kept in line with record parsing */
typedef enum {
  SWFDEC_BUTTON_UP = (1 << 0),
  SWFDEC_BUTTON_OVER = (1 << 1),
  SWFDEC_BUTTON_DOWN = (1 << 2),
  SWFDEC_BUTTON_HIT = (1 << 3)
} SwfdecButtonState;

/* these values have to be kept in line with condition parsing */
typedef enum {
  SWFDEC_BUTTON_IDLE_TO_OVER_UP = (1 << 0),
  SWFDEC_BUTTON_OVER_UP_TO_IDLE = (1 << 1),
  SWFDEC_BUTTON_OVER_UP_TO_OVER_DOWN = (1 << 2),
  SWFDEC_BUTTON_OVER_DOWN_TO_OVER_UP = (1 << 3),
  SWFDEC_BUTTON_OVER_DOWN_TO_OUT_DOWN = (1 << 4),
  SWFDEC_BUTTON_OUT_DOWN_TO_OVER_DOWN = (1 << 5),
  SWFDEC_BUTTON_OUT_DOWN_TO_IDLE = (1 << 6),
  SWFDEC_BUTTON_IDLE_TO_OVER_DOWN = (1 << 7),
  SWFDEC_BUTTON_OVER_DOWN_TO_IDLE = (1 << 8)
} SwfdecButtonCondition;

struct _SwfdecButtonRecord
{
  SwfdecButtonState	states;
  SwfdecObject *	object;
  cairo_matrix_t	transform;
  SwfdecColorTransform	color_transform;
};

struct _SwfdecButton
{
  SwfdecObject object;

  gboolean menubutton;

  GArray *records;
  SwfdecEventList *events;
};

struct _SwfdecButtonClass
{
  SwfdecObjectClass object_class;

};

GType swfdec_button_get_type (void);

void swfdec_button_render (SwfdecButton *button, SwfdecButtonState state, cairo_t *cr, 
    const SwfdecColorTransform *trans, const SwfdecRect *inval, gboolean fill);
SwfdecButtonState swfdec_button_change_state (SwfdecMovieClip *movie, gboolean was_in, 
  int old_button);

G_END_DECLS
#endif
