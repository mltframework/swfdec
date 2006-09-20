#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <js/jsapi.h>
#include "swfdec_event.h"
#include "swfdec_decoder.h"
#include "swfdec_compiler.h"

typedef struct _SwfdecEvent SwfdecEvent;

struct _SwfdecEvent {
  unsigned int	conditions;
  guint8	key;
  JSScript *	script;
};

struct _SwfdecEventList {
  SwfdecDecoder *	dec;
  guint			refcount;
  GArray *		events;
};


SwfdecEventList *
swfdec_event_list_new (SwfdecDecoder *dec)
{
  SwfdecEventList *list;

  g_return_val_if_fail (SWFDEC_IS_DECODER (dec), NULL);

  list = g_new0 (SwfdecEventList, 1);
  list->dec = dec;
  list->refcount = 1;
  list->events = g_array_new (FALSE, FALSE, sizeof (SwfdecEvent));

  return list;
}

/* FIXME: this is a bit nasty because of modifying */
SwfdecEventList *
swfdec_event_list_copy (SwfdecEventList *list)
{
  g_return_val_if_fail (list != NULL, NULL);

  list->refcount++;

  return list;
}

void
swfdec_event_list_free (SwfdecEventList *list)
{
  unsigned int i;

  g_return_if_fail (list != NULL);

  list->refcount--;
  if (list->refcount > 0)
    return;

  for (i = 0; i < list->events->len; i++) {
    SwfdecEvent *event = &g_array_index (list->events, SwfdecEvent, i);
    JS_DestroyScript (list->dec->jscx, event->script);
  }
  g_array_free (list->events, TRUE);
  g_free (list);
}

void
swfdec_event_list_parse (SwfdecEventList *list, unsigned int conditions,
    guint8 key)
{
  SwfdecEvent event;

  g_return_if_fail (list != NULL);
  g_return_if_fail (list->refcount == 1);

  event.conditions = conditions;
  event.key = key;
  event.script = swfdec_compile (list->dec);
  if (event.script) 
    g_array_append_val (list->events, event);
}

void
swfdec_event_list_execute (SwfdecEventList *list, SwfdecMovieClip *movie, 
    unsigned int conditions, guint8 key)
{
  unsigned int i;

  g_return_if_fail (list != NULL);

  for (i = 0; i < list->events->len; i++) {
    SwfdecEvent *event = &g_array_index (list->events, SwfdecEvent, i);
    if ((event->conditions & conditions) &&
	event->key == key)
      swfdec_decoder_queue_script (list->dec, movie, event->script);
  }
}

