
#include "swfdec_internal.h"


SwfdecObject *swfdec_object_new(SwfdecDecoder *s, int id)
{
	SwfdecObject *obj;

	obj = g_new0(SwfdecObject,1);

	obj->id = id;

	s->objects = g_list_append(s->objects, obj);

	return obj;
}


