
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

  printf ("  reserved = %d\n", reserved);
  printf ("  depth = %d\n", depth);

  if (has_character) {
    character_id = swfdec_bits_get_u16 (bits);
    printf ("  id = %d\n", character_id);
  }
  if (has_matrix) {
    swfdec_bits_get_matrix (bits);
  }
  if (has_color_transform) {
    swfdec_bits_get_color_transform (bits);
    swfdec_bits_syncbits (bits);
  }
  if (has_ratio) {
    ratio = swfdec_bits_get_u16 (bits);
    printf ("  ratio = %d\n", ratio);
  }
  if (has_name) {
    free (swfdec_bits_get_string (bits));
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

  printf ("  id = %d\n", id);
  printf ("  depth = %d\n", depth);

  return SWF_OK;
}

int
tag_func_remove_object_2 (SwfdecDecoder * s)
{
  int depth;

  depth = swfdec_bits_get_u16 (&s->b);

  printf ("  depth = %d\n", depth);

  return SWF_OK;
}
