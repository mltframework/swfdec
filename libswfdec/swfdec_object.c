
#include "swfdec_internal.h"


SwfdecObject *swfdec_object_new(SwfdecDecoder *s, int id)
{
	SwfdecObject *obj;

	obj = g_new0(SwfdecObject,1);

	obj->id = id;

	s->objects = g_list_append(s->objects, obj);

	return obj;
}

SwfdecObject *swfdec_object_get(SwfdecDecoder *s, int id)
{
	SwfdecObject *object;
	GList *g;

	for(g=g_list_first(s->objects);g;g=g_list_next(g)){
		object = (SwfdecObject *)g->data;
		if(object->id == id)return object;
	}
	SWF_DEBUG(2,"object not found (id==%d)\n",id);

	return NULL;
}

