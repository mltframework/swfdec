
#ifndef __SWFDEC_OBJECT_H__
#define __SWFDEC_OBJECT_H__

#include "swfdec_types.h"


struct swfdec_object_struct
{
  int id;
  int type;

  double trans[6];

  int number;

  void *priv;
};

enum
{
  SWF_OBJECT_SHAPE,
  SWF_OBJECT_TEXT,
  SWF_OBJECT_FONT,
  SWF_OBJECT_SPRITE,
  SWF_OBJECT_BUTTON,
  SWF_OBJECT_SOUND,
  SWF_OBJECT_IMAGE,
};

SwfdecObject *swfdec_object_new (SwfdecDecoder * s, int id);
void swfdec_object_free (SwfdecObject * object);
SwfdecObject *swfdec_object_get (SwfdecDecoder * s, int id);

#endif
