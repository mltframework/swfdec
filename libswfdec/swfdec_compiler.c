#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#include "swfdec_compiler.h"
#include "swfdec_bits.h"
#include "swfdec_decoder.h"
#include "swfdec_sprite.h"

typedef struct {
  guint action;
  const char *name;
} SwfdecActionSpec;

static const SwfdecActionSpec * swfdec_action_find (guint action);

/**
 * swfdec_compile:
 * @s: a #SwfdecDecoder during parsing
 *
 * continues parsing the current file and compiles the ActionScript commands 
 * encountered into a script for later execution.
 *
 * Returns: A new JSScript or NULL on failure
 **/
JSScript *
swfdec_compile (SwfdecDecoder *s)
{
  unsigned int action, len;
  const SwfdecActionSpec *current;
  SwfdecBits *bits;

  g_return_val_if_fail (s != NULL, NULL);

  bits = &s->b;
  g_print ("Creating new script for frame %d\n", s->parse_sprite->parse_frame);
  while ((action = swfdec_bits_get_u8 (bits))) {
    if (action & 0x80) {
      len = swfdec_bits_get_u16 (bits);
    } else {
      len = 0;
    }
    current = swfdec_action_find (action);
    if (current) {
      g_print ("  %s\n", current->name);
    } else {
      g_print ("  unknown action 0x%02X\n", action);
      swfdec_bits_getbits (bits, len);
    }
  }
  return NULL;
}

/* must be sorted! */
SwfdecActionSpec actions[] = {
  /* version 3 */
  { 0x81, "ActionGoToFrame" },
  { 0x83, "ActionGetURL" },
  { 0x04, "ActionNextFrame" },
  { 0x05, "ActionPreviousFrame" },
  { 0x06, "ActionPlay" },
  { 0x07, "ActionStop" },
  { 0x08, "ActionToggleQuality" },
  { 0x09, "ActionStopSounds" },
  { 0x8a, "ActionWaitForFrame" },
  { 0x8b, "ActionSetTarget" },
  { 0x8c, "ActionGoToLabel" },
};

static const SwfdecActionSpec *
swfdec_action_find (guint action)
{
  return bsearch (&action, actions, G_N_ELEMENTS (actions), 
      sizeof (SwfdecActionSpec), g_int_equal);
}
