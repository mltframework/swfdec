
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "swfdec_internal.h"

typedef struct
{
  void *SwfdecDecoder;
} SwfdecActionContext;

typedef struct
{
  int type;
  char *s;
  double num;
} SwfdecActionVal;

enum
{
  ACTIONVAL_TYPE_UNKNOWN = 0,
  ACTIONVAL_TYPE_BOOLEAN,
  ACTIONVAL_TYPE_INTEGER,
  ACTIONVAL_TYPE_DOUBLE,
  ACTIONVAL_TYPE_STRING,
};

#define ACTIONVAL_IS_BOOLEAN(val) ((val)->type == ACTIONVAL_TYPE_BOOLEAN)
#define ACTIONVAL_IS_INTEGER(val) ((val)->type == ACTIONVAL_TYPE_INTEGER)
#define ACTIONVAL_IS_DOUBLE(val) ((val)->type == ACTIONVAL_TYPE_DOUBLE)
#define ACTIONVAL_IS_STRING(val) ((val)->type == ACTIONVAL_TYPE_STRING)
#define ACTIONVAL_IS_NUM(val) (ACTIONVAL_IS_BOOLEAN(val) || \
		ACTIONVAL_IS_INTEGER(val) || ACTIONVAL_IS_DOUBLE(val))

typedef int SwfdecActionFunc (SwfdecActionContext * context,
    SwfdecActionVal * dest, SwfdecActionVal * src1, SwfdecActionVal * src2);

static SwfdecActionFunc
    action_add,
    action_subtract,
    action_multiply,
    action_divide,
    action_equal,
    action_less_than,
    action_logical_and,
    action_logical_or,
    action_logical_not,
    action_string_equal,
    action_string_length, action_substring, action_int, action_string_concat;

struct action_struct
{
  int action;
  char *name;
  SwfdecActionFunc *func;
  int n_src;
  int n_dest;
};
static struct action_struct actions[] = {
  {0x04, "next frame", NULL, 0, 0},
  {0x05, "prev frame", NULL, 0, 0},
  {0x06, "play", NULL, 0, 0},
  {0x07, "stop", NULL, 0, 0},
  {0x08, "toggle quality", NULL, 0, 0},
  {0x09, "stop sounds", NULL, 0, 0},
  {0x0a, "add", action_add, 2, 1},
  {0x0b, "subtract", action_subtract, 2, 1},
  {0x0c, "multiply", action_multiply, 2, 1},
  {0x0d, "divide", action_divide, 2, 1},
  {0x0e, "equal", action_equal, 2, 1},
  {0x0f, "less than", action_less_than, 2, 1},
  {0x10, "and", action_logical_and, 2, 1},
  {0x11, "or", action_logical_or, 2, 1},
  {0x12, "not", action_logical_not, 1, 1},
  {0x13, "string equal", action_string_equal, 2, 1},
  {0x14, "string length", action_string_length, 1, 1},
  {0x15, "substring", action_substring, 3, 1},
//      { 0x17, "unknown",              NULL },
  {0x18, "int", action_int, 1, 1},
  {0x1c, "eval", NULL, 1, 1},
  {0x1d, "set variable", NULL, 2, 0},
  {0x20, "set target expr", NULL, 1, 0},
  {0x21, "string concat", action_string_concat, 2, 1},
  {0x22, "get property", NULL, 2, 1},
  {0x23, "set property", NULL, 3, 0},
  {0x24, "duplicate clip", NULL, 3, 0},
  {0x25, "remove clip", NULL, 1, 0},
  {0x26, "trace", NULL, 0, 0},
  {0x27, "start drag movie", NULL, 3, 0},       /* 3 or 7 */
  {0x28, "stop drag movie", NULL, 0, 0},
  {0x29, "string compare", NULL, 2, 1},
  {0x30, "random", NULL, 1, 1},
  {0x31, "mb length", NULL, 1, 1},
  {0x32, "ord", NULL, 1, 1},
  {0x33, "chr", NULL, 1, 1},
  {0x34, "get timer", NULL, 0, 1},
  {0x35, "mb substring", NULL, 3, 1},
  {0x36, "mb ord", NULL, 1, 1},
  {0x37, "mb chr", NULL, 1, 1},
  {0x3b, "call function", NULL},
  {0x3a, "delete", NULL},
  {0x3c, "declare local var", NULL},
//      { 0x3d, "unknown",              NULL },
  {0x3e, "return from function", NULL},
  {0x3f, "modulo", NULL},
  {0x40, "instantiate", NULL},
//      { 0x41, "unknown",              NULL },
  {0x44, "typeof", NULL},
  {0x46, "unknown", NULL},
  {0x47, "add", NULL},
  {0x48, "less than", NULL},
  {0x49, "equals", NULL},
  {0x4c, "make boolean", NULL},
  {0x4e, "get member", NULL},
  {0x4f, "set member", NULL},
  {0x50, "increment", NULL},
  {0x52, "call method", NULL},
  {0x60, "bitwise and", NULL},
  {0x61, "bitwise or", NULL},
  {0x62, "bitwise xor", NULL},
  {0x63, "shift left", NULL},
  {0x64, "shift right", NULL},
  {0x65, "rotate right", NULL},
//      { 0x67, "unknown",              NULL },
  {0x81, "goto frame", NULL},
  {0x83, "get url", NULL},
  {0x87, "get next dict member", NULL},
  {0x88, "declare dictionary", NULL},
  {0x8a, "wait for frame", NULL},
//      { 0x8b, "unknown",              NULL },
//      { 0x8c, "unknown",              NULL },
  {0x8d, "wait for frame expr", NULL, 1, 0},
  {0x94, "with", NULL},
  {0x96, "push data", NULL},
  {0x99, "branch", NULL, 0, 0},
  {0x9a, "get url2", NULL, 2, 0},
  {0x9b, "declare function", NULL},
  {0x9d, "branch if", NULL, 1, 0},
  {0x9e, "call frame", NULL, 1, 0},
  {0x9f, "goto expr", NULL, 1, 0},
//      { 0xb7, "unknown",              NULL },
/* in 1394C148d01.swf */
  {0x8b, "unknown", NULL},
  {0x8c, "unknown", NULL},
};
static int n_actions = sizeof (actions) / sizeof (actions[0]);

#if 0
static void
hexdump (unsigned char *data, int len)
{
  int i;
  int j;

  while (len > 0) {
    printf ("    ");
    j = (len < 16) ? len : 16;
    for (i = 0; i < j; i++) {
      printf ("%02x ", data[i]);
    }
    for (; i < 16; i++) {
      printf ("   ");
    }
    for (i = 0; i < j; i++) {
      printf ("%c", isprint (data[i]) ? data[i] : '.');
    }
    printf ("\n");
    data += j;
    len -= j;
  }
}
#endif

void
get_actions (SwfdecDecoder * s, SwfdecBits * bits)
{
  int action;
  int len;

  //int index;
  //int skip_count;
  int i;

  SWFDEC_LOG ("get_actions");

  while (1) {
    action = swfdec_bits_get_u8 (bits);
    if (action == 0)
      break;

    if (action & 0x80) {
      len = swfdec_bits_get_u16 (bits);
    } else {
      len = 0;
    }

    for (i = 0; i < n_actions; i++) {
      if (actions[i].action == action) {
        SWFDEC_LOG ("  [%02x] %s", action, actions[i].name);
        break;
      }
    }
    if (i == n_actions) {
      SWFDEC_WARNING ("  [%02x] unknown action", action);
    }
#if 0
    if (s->debug < 1) {
      hexdump (bits->ptr, len);
    }
#endif

    bits->ptr += len;
  }
}

int
tag_func_do_action (SwfdecDecoder * s)
{
  get_actions (s, &s->b);
  swfdec_sprite_add_action (s->parse_sprite, s->b.buffer,
      s->parse_sprite->parse_frame);

  return SWF_OK;
}


int
swfdec_action_script_execute (SwfdecDecoder * s, SwfdecBuffer * buffer)
{
  int action;
  int len;
  SwfdecBits bits;

  //int index;
  //int skip_count;
  int i;
  int skip = 0;

  SWFDEC_LOG ("swfdec_action_script_execute %p %p %d", buffer,
      buffer->data, buffer->length);

  bits.buffer = buffer;
  bits.ptr = buffer->data;
  bits.idx = 0;
  bits.end = buffer->data + buffer->length;

  while (1) {
    action = swfdec_bits_get_u8 (&bits);
    if (action == 0)
      break;

    if (action & 0x80) {
      len = swfdec_bits_get_u16 (&bits);
    } else {
      len = 0;
    }

    for (i = 0; i < n_actions; i++) {
      if (actions[i].action == action) {
        SWFDEC_INFO ("  [%02x] %s", action, actions[i].name);
        break;
      }
    }
    if (i == n_actions) {
      SWFDEC_WARNING ("  [%02x] unknown action", action);
    }

    if (skip > 0) {
      skip--;
    } else {
      if (action == 0x07) {     /* stop */
        /* FIXME */
        //s->stopped = TRUE;
      }
      if (action == 0x81) {     /* goto frame */
        int frame;

        frame = swfdec_bits_get_u16 (&bits);
        SWFDEC_INFO ("goto frame %d\n", frame);
        bits.ptr -= 2;

        s->render->frame_index = frame - 1;
      }
      if (action == 0x06) {     /* play */
        s->stopped = FALSE;
      }
      if (action == 0x8a) {     /* wait for frame */
        int frame;

        frame = swfdec_bits_get_u16 (&bits);
        skip = swfdec_bits_get_u8 (&bits);
        SWFDEC_INFO ("wait for frame %d\n", frame);
        bits.ptr -= 3;

        if (TRUE) {
          /* frame is always loaded */
          skip = 0;
        }
      }
    }

    bits.ptr += len;
  }

  return SWF_OK;
}


static int
action_promote (SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  if (ACTIONVAL_IS_DOUBLE (src1))
    return ACTIONVAL_TYPE_DOUBLE;
  if (ACTIONVAL_IS_DOUBLE (src2))
    return ACTIONVAL_TYPE_DOUBLE;

  if (ACTIONVAL_IS_INTEGER (src1))
    return ACTIONVAL_TYPE_INTEGER;
  if (ACTIONVAL_IS_INTEGER (src2))
    return ACTIONVAL_TYPE_INTEGER;

  return ACTIONVAL_TYPE_BOOLEAN;
}

static SwfdecActionVal *
action_pop (SwfdecActionContext * context)
{
  /* FIXME unimplemented */
  return NULL;
}

static int
action_add (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_NUM (src1) && ACTIONVAL_IS_NUM (src2)) {
    dest->type = action_promote (src1, src2);
    dest->num = src1->num + src2->num;
  } else if (ACTIONVAL_IS_STRING (src1) && ACTIONVAL_IS_STRING (src2)) {
    dest->type = ACTIONVAL_TYPE_STRING;
    dest->s = g_malloc (strlen (src1->s) + strlen (src2->s) + 1);
    strcpy (dest->s, src1->s);
    strcat (dest->s, src2->s);
  } else {
    SWFDEC_DEBUG ("incompatible types");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_subtract (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_NUM (src1) && ACTIONVAL_IS_NUM (src2)) {
    dest->type = action_promote (src1, src2);
    dest->num = src1->num - src2->num;
  } else {
    SWFDEC_DEBUG ("incompatible types");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_multiply (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_NUM (src1) && ACTIONVAL_IS_NUM (src2)) {
    dest->type = action_promote (src1, src2);
    dest->num = src1->num * src2->num;
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_divide (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_NUM (src1) && ACTIONVAL_IS_NUM (src2)) {
    dest->type = action_promote (src1, src2);
    dest->num = src1->num / src2->num;
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_equal (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_NUM (src1) && ACTIONVAL_IS_NUM (src2)) {
    dest->type = ACTIONVAL_TYPE_BOOLEAN;
    dest->num = (src1->num == src2->num);
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_less_than (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_NUM (src1) && ACTIONVAL_IS_NUM (src2)) {
    dest->type = ACTIONVAL_TYPE_BOOLEAN;
    dest->num = (src1->num < src2->num);
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_logical_and (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_BOOLEAN (src1) && ACTIONVAL_IS_BOOLEAN (src2)) {
    dest->type = ACTIONVAL_TYPE_BOOLEAN;
    dest->num = (src1->num && src2->num);
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_logical_or (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_BOOLEAN (src1) && ACTIONVAL_IS_BOOLEAN (src2)) {
    dest->type = ACTIONVAL_TYPE_BOOLEAN;
    dest->num = (src1->num || src2->num);
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_logical_not (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2 == NULL);

  if (ACTIONVAL_IS_BOOLEAN (src1)) {
    dest->type = ACTIONVAL_TYPE_BOOLEAN;
    dest->num = !src1->num;
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_string_equal (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_STRING (src1) && ACTIONVAL_IS_STRING (src2)) {
    dest->type = ACTIONVAL_TYPE_BOOLEAN;
    dest->num = (strcmp (src1->s, src2->s) == 0);
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_string_length (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2 == NULL);

  if (ACTIONVAL_IS_STRING (src1)) {
    dest->type = ACTIONVAL_TYPE_INTEGER;
    dest->num = strlen (src1->s);
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_substring (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  unsigned int len;
  unsigned int slen;
  unsigned int offset;
  SwfdecActionVal *src3;

  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  src3 = action_pop (context);
  g_assert (src3);

  if (ACTIONVAL_IS_STRING (src1) && ACTIONVAL_IS_NUM (src2) &&
      ACTIONVAL_IS_NUM (src3)) {
    dest->type = ACTIONVAL_TYPE_STRING;
    slen = strlen (src1->s);

    offset = src2->num;
    if (offset > slen)
      offset = slen;

    len = src3->num;
    if (len > slen - offset)
      len = slen - offset;

    dest->s = g_malloc (len + 1);
    memcpy (dest->s, src1->s + offset, len);
    dest->s[len] = 0;
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_int (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2 == NULL);

  if (ACTIONVAL_IS_NUM (src1)) {
    dest->type = ACTIONVAL_TYPE_INTEGER;
    dest->num = floor (src1->num);
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}

static int
action_string_concat (SwfdecActionContext * context, SwfdecActionVal * dest,
    SwfdecActionVal * src1, SwfdecActionVal * src2)
{
  g_assert (dest);
  g_assert (src1);
  g_assert (src2);

  if (ACTIONVAL_IS_STRING (src1) && ACTIONVAL_IS_STRING (src2)) {
    dest->type = ACTIONVAL_TYPE_STRING;
    dest->s = g_malloc (strlen (src1->s) + strlen (src2->s) + 1);
    strcpy (dest->s, src1->s);
    strcat (dest->s, src2->s);
  } else {
    SWFDEC_DEBUG ("incompatible types\n");
    return SWF_ERROR;
  }

  return SWF_OK;
}
