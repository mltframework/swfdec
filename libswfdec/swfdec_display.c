
#include "swfdec_internal.h"

int
tag_func_place_object_2 (SwfdecDecoder * s)
{
  SwfdecBits *bits = &s->b;
  int reserved;
  int has_unknown;
  int has_name;
  int has_ratio;
  int has_color_transform;
  int has_matrix;
  int has_character;
  int move;
  int depth;
  int character_id;
  int ratio;

  reserved = swfdec_bits_getbit (bits);
  has_unknown = swfdec_bits_getbit (bits);
  has_name = swfdec_bits_getbit (bits);
  has_ratio = swfdec_bits_getbit (bits);
  has_color_transform = swfdec_bits_getbit (bits);
  has_matrix = swfdec_bits_getbit (bits);
  has_character = swfdec_bits_getbit (bits);
  move = swfdec_bits_getbit (bits);
  depth = swfdec_bits_get_u16 (bits);

  SWFDEC_DEBUG ("  reserved = %d\n", reserved);
  SWFDEC_DEBUG ("  depth = %d\n", depth);

  if (has_character) {
    character_id = swfdec_bits_get_u16 (bits);
    SWFDEC_DEBUG ("  id = %d\n", character_id);
  }
  if (has_matrix) {
    SwfdecTransform trans;

    swfdec_bits_get_transform (bits, &trans);
  }
  if (has_color_transform) {
    SwfdecColorTransform ct;

    swfdec_bits_get_color_transform (bits, &ct);
    swfdec_bits_syncbits (bits);
  }
  if (has_ratio) {
    ratio = swfdec_bits_get_u16 (bits);
    SWFDEC_DEBUG ("  ratio = %d\n", ratio);
  }
  if (has_name) {
    g_free (swfdec_bits_get_string (bits));
  }

  return SWF_OK;
}

int
tag_func_remove_object (SwfdecDecoder * s)
{
  int id;
  int depth;

  id = swfdec_bits_get_u16 (&s->b);
  depth = swfdec_bits_get_u16 (&s->b);

  SWFDEC_DEBUG ("  id = %d\n", id);
  SWFDEC_DEBUG ("  depth = %d\n", depth);

  return SWF_OK;
}

int
tag_func_remove_object_2 (SwfdecDecoder * s)
{
  int depth;

  depth = swfdec_bits_get_u16 (&s->b);

  SWFDEC_DEBUG ("  depth = %d\n", depth);

  return SWF_OK;
}
