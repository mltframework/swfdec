
#include <ctype.h>
#include <string.h>
#include <math.h>

#include "swfdec_internal.h"

typedef struct
{
  int type;
  char *string;
  double number;
} ActionVal;

typedef struct
{
  char *name;
  ActionVal *value;
} ActionVar;

struct _SwfdecActionContext
{
  SwfdecDecoder *s;
  SwfdecBits bits;
  GQueue *stack;
  int error;
  ActionVal *registers[4];
  int action;
  char *pc;
  int skip;
  GList *variables;

  int n_constants;
  char **constants;
};

enum
{
  ACTIONVAL_TYPE_UNKNOWN = 0,
  ACTIONVAL_TYPE_BOOLEAN,
  ACTIONVAL_TYPE_NUMBER,
  ACTIONVAL_TYPE_STRING,
};

#define ACTIONVAL_IS_BOOLEAN(val) ((val)->type == ACTIONVAL_TYPE_BOOLEAN)
#define ACTIONVAL_IS_NUMBER(val) ((val)->type == ACTIONVAL_TYPE_NUMBER)
#define ACTIONVAL_IS_STRING(val) ((val)->type == ACTIONVAL_TYPE_STRING)

typedef void SwfdecActionFunc (SwfdecActionContext * context);

static SwfdecActionFunc action_goto_frame;
static SwfdecActionFunc action_get_url;
static SwfdecActionFunc action_next_frame;
static SwfdecActionFunc action_previous_frame;
static SwfdecActionFunc action_play;
static SwfdecActionFunc action_stop;
static SwfdecActionFunc action_toggle_quality;
static SwfdecActionFunc action_stop_sounds;
static SwfdecActionFunc action_wait_for_frame;
static SwfdecActionFunc action_set_target;
static SwfdecActionFunc action_go_to_label;
static SwfdecActionFunc action_push;
static SwfdecActionFunc action_pop;
static SwfdecActionFunc action_binary_op;
static SwfdecActionFunc action_not;
static SwfdecActionFunc action_string_equals;
static SwfdecActionFunc action_string_length;
static SwfdecActionFunc action_string_add;
static SwfdecActionFunc action_string_extract;
static SwfdecActionFunc action_string_less;
static SwfdecActionFunc action_mb_string_length;
static SwfdecActionFunc action_mb_string_extract;
static SwfdecActionFunc action_to_integer;
static SwfdecActionFunc action_char_to_ascii;
static SwfdecActionFunc action_ascii_to_char;
static SwfdecActionFunc action_mb_char_to_ascii;
static SwfdecActionFunc action_mb_ascii_to_char;
static SwfdecActionFunc action_jump;
static SwfdecActionFunc action_if;
static SwfdecActionFunc action_call;
static SwfdecActionFunc action_get_variable;
static SwfdecActionFunc action_set_variable;
static SwfdecActionFunc action_get_url_2;
static SwfdecActionFunc action_goto_frame_2;
static SwfdecActionFunc action_set_target_2;
static SwfdecActionFunc action_get_property;
static SwfdecActionFunc action_set_property;
static SwfdecActionFunc action_clone_sprite;
static SwfdecActionFunc action_remove_sprite;
static SwfdecActionFunc action_start_drag;
static SwfdecActionFunc action_end_drag;
static SwfdecActionFunc action_wait_for_frame_2;
static SwfdecActionFunc action_trace;
static SwfdecActionFunc action_get_time;
static SwfdecActionFunc action_random_number;
static SwfdecActionFunc action_constant_pool;


#if 0
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
  {0x0a, "add", action_binary_op, 2, 1},
  {0x0b, "subtract", action_binary_op, 2, 1},
  {0x0c, "multiply", action_binary_op, 2, 1},
  {0x0d, "divide", action_binary_op, 2, 1},
  {0x0e, "equal", action_binary_op, 2, 1},
  {0x0f, "less than", action_binary_op, 2, 1},
  {0x10, "and", action_binary_op, 2, 1},
  {0x11, "or", action_binary_op, 2, 1},
  {0x12, "not", action_not, 1, 1},
  {0x13, "string equal", action_string_equals, 2, 1},
  {0x14, "string length", action_string_length, 1, 1},
  {0x15, "substring", NULL, 3, 1},
  {0x17, "pop", action_pop },
  {0x18, "int", action_to_integer, 1, 1},
  {0x1c, "eval", NULL, 1, 1},
  {0x1d, "set variable", action_set_variable, 2, 0},
  {0x20, "set target expr", NULL, 1, 0},
  {0x21, "string concat", NULL, 2, 1},
  {0x22, "get property", action_get_property, 2, 1},
  {0x23, "set property", action_set_property, 3, 0},
  {0x24, "duplicate sprite", action_clone_sprite, 3, 0},
  {0x25, "remove sprite", action_remove_sprite, 1, 0},
  {0x26, "trace", action_trace, 0, 0},
  {0x27, "start drag", action_start_drag, 3, 0},
  {0x28, "stop drag", action_end_drag, 0, 0},
  {0x29, "string compare", NULL, 2, 1},
  {0x30, "random number", action_random_number, 1, 1},
  {0x31, "mb length", action_mb_string_length, 1, 1},
  {0x32, "ord", NULL, 1, 1},
  {0x33, "chr", NULL, 1, 1},
  {0x34, "get time", action_get_time, 0, 1},
  {0x35, "mb substring", NULL, 3, 1},
  {0x36, "mb ord", NULL, 1, 1},
  {0x37, "mb chr", NULL, 1, 1},
  {0x3b, "call function", NULL},
  {0x3a, "delete", NULL},
  {0x3c, "declare local var", NULL},
  {0x3d, "call function", NULL },
  {0x3e, "return from function", NULL},
  {0x3f, "modulo", NULL},
  {0x40, "instantiate", NULL},
  {0x41, "define local 2", NULL },
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
  {0x67, "greater", NULL},
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
#endif

typedef struct _ActionFuncEntry ActionFuncEntry;
struct _ActionFuncEntry
{
  int action;
  SwfdecActionFunc *func;
};
static ActionFuncEntry swf_actions[] = {
  /* version 3 */
  { 0x81, action_goto_frame },
  { 0x83, action_get_url },
  { 0x04, action_next_frame },
  { 0x05, action_previous_frame },
  { 0x06, action_play },
  { 0x07, action_stop },
  { 0x08, action_toggle_quality },
  { 0x09, action_stop_sounds },
  { 0x8a, action_wait_for_frame },
  { 0x8b, action_set_target },
  { 0x8c, action_go_to_label },
  /* version 4 */
  { 0x96, action_push },
  { 0x17, action_pop },
  { 0x0a, action_binary_op /* add */ },
  { 0x0b, action_binary_op /* subtract */ },
  { 0x0c, action_binary_op /* multiply */ },
  { 0x0d, action_binary_op /* divide */ },
  { 0x0e, action_binary_op /* equals */ },
  { 0x0f, action_binary_op /* less */ },
  { 0x10, action_binary_op /* and */ },
  { 0x11, action_binary_op /* or */ },
  { 0x12, action_not },
  { 0x13, action_string_equals },
  { 0x14, action_string_length },
  { 0x21, action_string_add },
  { 0x15, action_string_extract },
  { 0x29, action_string_less },
  { 0x31, action_mb_string_length },
  { 0x35, action_mb_string_extract },
  { 0x18, action_to_integer },
  { 0x32, action_char_to_ascii },
  { 0x33, action_ascii_to_char },
  { 0x36, action_mb_char_to_ascii },
  { 0x37, action_mb_ascii_to_char },
  { 0x99, action_jump },
  { 0x9d, action_if },
  { 0x9e, action_call },
  { 0x1c, action_get_variable },
  { 0x1d, action_set_variable },
  { 0x9a, action_get_url_2 },
  { 0x9f, action_goto_frame_2 },
  { 0x20, action_set_target_2 },
  { 0x22, action_get_property },
  { 0x23, action_set_property },
  { 0x24, action_clone_sprite },
  { 0x25, action_remove_sprite },
  { 0x27, action_start_drag },
  { 0x28, action_end_drag },
  { 0x8d, action_wait_for_frame_2 },
  { 0x26, action_trace },
  { 0x34, action_get_time },
  { 0x30, action_random_number },
  /* version 5 */
  { 0x88, action_constant_pool },
  /* ... */
};

static ActionFuncEntry *
get_action (int action)
{
  int i;

  for (i=0;i<sizeof(swf_actions)/sizeof(swf_actions[0]);i++){
    if (swf_actions[i].action == action) {
      return swf_actions + i;
    }
  }
  return NULL;
}


int
tag_func_do_action (SwfdecDecoder * s)
{
  swfdec_sprite_add_action (s->parse_sprite, s->b.buffer,
      s->parse_sprite->parse_frame);

  s->b.ptr += s->b.buffer->length;

  return SWF_OK;
}

static int
pc_is_valid (SwfdecActionContext *context, char *pc)
{
  char *startpc;
  char *endpc;

  startpc = (char *)context->bits.buffer->data;
  endpc = (char *)context->bits.buffer->data + context->bits.buffer->length;
  if (pc >= startpc && pc < endpc) {
    return SWF_OK;
  } else {
    return SWF_ERROR;
  }
}
  

int
swfdec_action_script_execute (SwfdecDecoder * s, SwfdecBuffer * buffer)
{
  int action;
  int len;
  SwfdecBits bits;
  ActionFuncEntry *func_entry;
  SwfdecActionContext *context;

  SWFDEC_DEBUG ("swfdec_action_script_execute %p %p %d", buffer,
      buffer->data, buffer->length);

  if (s->context == NULL) {
    s->context = g_malloc0 (sizeof(SwfdecActionContext));
    s->context->s = s;
    s->context->stack = g_queue_new();
  }
  context = s->context;

  bits.buffer = buffer;
  bits.ptr = buffer->data;
  bits.idx = 0;
  bits.end = buffer->data + buffer->length;

  while (1) {
    action = swfdec_bits_get_u8 (&bits);
    SWFDEC_DEBUG ("executing action 0x%02x", action);
    if (action & 0x80) {
      len = swfdec_bits_get_u16 (&bits);
      context->bits.buffer = bits.buffer;
      context->bits.ptr = bits.ptr;
      context->bits.idx = 0;
      context->bits.end = bits.ptr + len;
    } else {
      len = 0;
    }
    bits.ptr += len;
    context->pc = bits.ptr;
    context->action = action;

    if (context->skip > 0) {
      context->skip--;
      continue;
    }
    if (action == 0)
      break;

    func_entry = get_action (action);
    if (func_entry) {
      func_entry->func (context);
    }else {
      SWFDEC_WARNING ("unknown action 0x%02x, ignoring", action);
      context->error = 1;
    }

    if (len) {
      if (context->bits.ptr < context->bits.end) {
        SWFDEC_ERROR ("action didn't read all data (%d < %d)",
            context->bits.ptr - (bits.ptr - len),
            context->bits.end - (bits.ptr - len));
      }
      if (context->bits.ptr > context->bits.end) {
        SWFDEC_ERROR ("action read past end of buffer (%d > %d)",
            context->bits.ptr - (bits.ptr - len),
            context->bits.end - (bits.ptr - len));
      }
    }

    if (context->pc >= (char *)bits.buffer->data &&
        context->pc < (char *)bits.end) {
      bits.ptr = context->pc;
    } else {
      SWFDEC_ERROR ("jump target outside buffer");
      context->error = 1;
    }

    if (context->error) {
      context->error = 0;
      SWFDEC_ERROR ("action script error");
      //break;
    }

  }

  return SWF_OK;
}

/* action value operations */

static ActionVal *
action_val_new (void)
{
  ActionVal *val;
  val = g_malloc0 (sizeof(ActionVal));

  return val;
}

static void
action_val_free (ActionVal *val)
{
  if (val->string) g_free (val->string);
  g_free (val);
}

static void
action_val_copy (ActionVal *dest, ActionVal *src)
{
  if (dest->string) g_free (dest->string);
  dest->type = src->type;
  dest->number = src->number;
  if (src->string) {
    dest->string = strdup(src->string);
  } else {
    dest->string = NULL;
  }
}

static void
action_val_convert_to_string (ActionVal * val)
{
  if (val->type == ACTIONVAL_TYPE_STRING) return;

  if (val->string) g_free (val->string);

  switch (val->type) {
    case ACTIONVAL_TYPE_UNKNOWN:
      /* probably an error */
      val->string = strdup("");
      break;
    case ACTIONVAL_TYPE_BOOLEAN:
      val->string = g_strdup (val->number ? "true" : "false");
      break;
    case ACTIONVAL_TYPE_NUMBER:
      {
        char s[G_ASCII_DTOSTR_BUF_SIZE];
        val->string = g_strdup(g_ascii_dtostr (s, G_ASCII_DTOSTR_BUF_SIZE,
              val->number));
      }
      break;
    case ACTIONVAL_TYPE_STRING:
      break;
  }
  val->type = ACTIONVAL_TYPE_STRING;
}

static void
action_val_convert_to_number (ActionVal * val)
{
  switch (val->type) {
    case ACTIONVAL_TYPE_UNKNOWN:
      /* probably an error */
      val->number = 0.0;
      break;
    case ACTIONVAL_TYPE_BOOLEAN:
      break;
    case ACTIONVAL_TYPE_NUMBER:
      break;
    case ACTIONVAL_TYPE_STRING:
      val->number = g_ascii_strtod (val->string, NULL);
      g_free (val->string);
      val->string = NULL;
      break;
  }
  val->type = ACTIONVAL_TYPE_NUMBER;
}

static void
action_val_convert_to_boolean (ActionVal * val)
{
  switch (val->type) {
    case ACTIONVAL_TYPE_UNKNOWN:
      /* probably an error */
      val->number = 0.0;
      break;
    case ACTIONVAL_TYPE_BOOLEAN:
      break;
    case ACTIONVAL_TYPE_NUMBER:
      val->number = (val->number != 0);
      break;
    case ACTIONVAL_TYPE_STRING:
      val->number = (strlen(val->string) > 0);
      g_free (val->string);
      val->string = NULL;
      break;
  }
  val->type = ACTIONVAL_TYPE_NUMBER;
}

/* variable ops */

static ActionVar *
variable_get (SwfdecActionContext *context, const char *name)
{
  GList *g;
  ActionVar *var;

  for(g=g_list_first(context->variables);g;g=g_list_next(g)){
    var = g->data;
    if (strcmp(name,var->name)==0) {
      return var;
    }
  }

  var = g_malloc(sizeof(ActionVar));
  var->name = strdup(name);
  var->value = action_val_new();
  context->variables = g_list_append(context->variables, var);

  return var;
}

/* stack ops */

static ActionVal *
stack_pop (SwfdecActionContext *context)
{
  ActionVal *val;

  val = g_queue_pop_head (context->stack);
  if (val == NULL) {
    SWFDEC_ERROR("stack underrun");
    context->error = 1;
    val = action_val_new ();
  }

  return val;
}

static void
stack_push (SwfdecActionContext *context, ActionVal *value)
{
  g_queue_push_head (context->stack, value);
}

/* actions */

static void
action_goto_frame (SwfdecActionContext * context)
{
  int frame;

  frame = swfdec_bits_get_u16 (&context->bits);

  /* FIXME hack */
  context->s->render->frame_index = frame - 1;
  if (context->s->render->frame_index < 0) {
    context->s->render->frame_index = 0;
  }
}

static void
action_get_url (SwfdecActionContext * context)
{
  char *url;
  char *target;

  url = swfdec_bits_get_string (&context->bits);
  target = swfdec_bits_get_string (&context->bits);

  SWFDEC_DEBUG ("get_url %s %s", url, target);

  if (context->s->url) g_free (context->s->url);
  context->s->url = url;
  
  g_free(target);
}

static void
action_next_frame (SwfdecActionContext * context)
{
  context->s->render->frame_index++;
}

static void
action_previous_frame (SwfdecActionContext * context)
{
  context->s->render->frame_index--;
}

static void
action_play (SwfdecActionContext * context)
{
  context->s->stopped = FALSE;
}

static void
action_stop (SwfdecActionContext * context)
{
  context->s->stopped = TRUE;
}

static void
action_toggle_quality (SwfdecActionContext * context)
{
  /* ignored */
}

static void
action_stop_sounds (SwfdecActionContext * context)
{
  /* FIXME */
  SWFDEC_WARNING ("stop_sounds unimplemented");
}

static void
action_wait_for_frame (SwfdecActionContext * context)
{
  int frame;
  int skip;

  frame = swfdec_bits_get_u16 (&context->bits);
  skip = swfdec_bits_get_u8 (&context->bits);

  if (TRUE) {
    /* frame is always loaded */
    context->skip = 0;
  } else {
    context->skip = skip;
  }
}

static void
action_set_target (SwfdecActionContext * context)
{
  char *target;

  target = swfdec_bits_get_string (&context->bits);

  SWFDEC_WARNING ("set_target unimplemented");

  g_free(target);
}

static void
action_go_to_label (SwfdecActionContext * context)
{
  char *label;

  label = swfdec_bits_get_string (&context->bits);

  SWFDEC_WARNING ("go_to_label unimplemented");

  g_free(label);
}

static void
action_push (SwfdecActionContext * context)
{
  ActionVal *value;
  int type;

  while (context->bits.ptr < context->bits.end) {
  value = action_val_new();

  type = swfdec_bits_get_u8 (&context->bits);
  switch (type) {
    case 0: /* string */
      value->type = ACTIONVAL_TYPE_STRING;
      value->string = swfdec_bits_get_string (&context->bits);
      break;
    case 1: /* float */
      {
        union {
          float f;
          guint32 i;
        } x;

        value->type = ACTIONVAL_TYPE_NUMBER;
        /* FIXME check endianness */
        x.i = swfdec_bits_get_u32 (&context->bits);
        value->number = x.f;
      }
      break;
    case 2: /* null */
      /* FIXME what to do here */
      break;
    case 3: /* undefined */
      break;
    case 4: /* register number */
      {
        int reg;
        reg = swfdec_bits_get_u8 (&context->bits);
        /* FIXME there can be 256 registers */
        if (reg >=0 && reg < 4) {
          action_val_copy (value, context->registers[reg]);
        }
      }
      break;
    case 5: /* boolean */
      value->type = ACTIONVAL_TYPE_BOOLEAN;
      value->number = swfdec_bits_get_u8 (&context->bits);
      break;
    case 6: /* double */
      {
        union {
          double f;
          guint32 i[2];
        } x;

        value->type = ACTIONVAL_TYPE_NUMBER;
        /* FIXME check endianness */
        x.i[0] = swfdec_bits_get_u32 (&context->bits);
        x.i[1] = swfdec_bits_get_u32 (&context->bits);
        value->number = x.f;
      }
      break;
    case 7: /* int32 */
      value->type = ACTIONVAL_TYPE_NUMBER;
      value->number = swfdec_bits_get_u32 (&context->bits);
      break;
    case 8: /* constant8 */
      {
        int i;
        i = swfdec_bits_get_u8 (&context->bits);
        if (i < context->n_constants) {
          value->type = ACTIONVAL_TYPE_STRING;
          value->string = g_strdup (context->constants[i]);
        } else {
          SWFDEC_ERROR ("request for constant outside constant pool");
          context->error = 1;
        }
      }
      break;
    case 9: /* constant16 */
      {
        int i;
        i = swfdec_bits_get_u16 (&context->bits);
        if (i < context->n_constants) {
          value->type = ACTIONVAL_TYPE_STRING;
          value->string = g_strdup (context->constants[i]);
        } else {
          SWFDEC_ERROR ("request for constant outside constant pool");
          context->error = 1;
        }
      }
      break;
    default:
      SWFDEC_ERROR("illegal type");
  }

  stack_push (context, value);
  }
}

static void
action_pop (SwfdecActionContext * context)
{
  action_val_free (stack_pop (context));
}

static void
action_binary_op (SwfdecActionContext * context)
{
  ActionVal *a;
  ActionVal *b;

  a = stack_pop (context);
  action_val_convert_to_number (a);
  b = stack_pop (context);
  action_val_convert_to_number (b);

  switch (context->action) {
    case 0x0a:
      b->number += a->number;
      break;
    case 0x0b:
      b->number -= a->number;
      break;
    case 0x0c:
      b->number *= a->number;
      break;
    case 0x0d:
      b->number /= a->number;
      break;
    case 0x0e:
      b->number = (b->number == a->number);
      break;
    case 0x0f:
      b->number = (b->number < a->number);
      break;
    case 0x10:
      b->number = (b->number && a->number);
      break;
    case 0x11:
      b->number = (b->number || a->number);
      break;
    default:
      SWFDEC_ERROR ("should not be reached");
      break;
  }
  action_val_free (a);
  stack_push (context, b);
}

static void
action_not (SwfdecActionContext * context)
{
  ActionVal *a;

  a = stack_pop (context);
  action_val_convert_to_number (a);
  a->number = (!a->number);

  stack_push (context, a);
}

static void
action_string_equals (SwfdecActionContext * context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;

  a = stack_pop (context);
  action_val_convert_to_string (a);
  b = stack_pop (context);
  action_val_convert_to_string (b);
  c = action_val_new ();

  c->type = ACTIONVAL_TYPE_BOOLEAN;
  c->number = (strcmp (a->string, b->string) == 0);

  action_val_free (a);
  action_val_free (b);
  stack_push (context, c);
}

static void
action_string_length (SwfdecActionContext * context)
{
  ActionVal *a;
  ActionVal *d;

  a = stack_pop (context);
  action_val_convert_to_string (a);
  d = action_val_new ();

  d->type = ACTIONVAL_TYPE_NUMBER;
  d->number = strlen (a->string);

  action_val_free (a);
  stack_push (context, d);
}

static void
action_string_add (SwfdecActionContext * context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;

  a = stack_pop (context);
  action_val_convert_to_string (a);
  b = stack_pop (context);
  action_val_convert_to_string (b);
  c = action_val_new ();

  c->type = ACTIONVAL_TYPE_STRING;
  c->string = g_strconcat (b->string, a->string, NULL);

  action_val_free (a);
  action_val_free (b);
  stack_push (context, c);
}

static void
action_string_extract (SwfdecActionContext * context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;
  ActionVal *d;
  int n;
  int count;
  int index;

  a = stack_pop (context);
  action_val_convert_to_number (a);
  b = stack_pop (context);
  action_val_convert_to_number (b);
  c = stack_pop (context);
  action_val_convert_to_string (c);

  d = action_val_new ();
  d->type = ACTIONVAL_TYPE_STRING;
  n = strlen (c->string);
  count = a->number;
  index = b->number;
  if (count < 0) count = 0;
  if (index < 0) index = 0;
  if (index > n) index = n;
  if (count > n - index) count = n - index;

  d->string = g_strndup (c->string + index, count);

  action_val_free (a);
  action_val_free (b);
  action_val_free (c);
  stack_push (context, d);
}

static void
action_string_less (SwfdecActionContext * context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *d;

  a = stack_pop (context);
  action_val_convert_to_string (a);
  b = stack_pop (context);
  action_val_convert_to_string (b);
  d = action_val_new ();

  d->type = ACTIONVAL_TYPE_BOOLEAN;
  d->number = (strcmp (b->string, a->string) < 0);

  action_val_free (a);
  action_val_free (b);
  stack_push (context, d);
}

static void
action_mb_string_length (SwfdecActionContext * context)
{
  ActionVal *a;
  ActionVal *d;

  a = stack_pop (context);
  action_val_convert_to_string (a);
  d = action_val_new ();

  d->type = ACTIONVAL_TYPE_NUMBER;
  d->number = g_utf8_strlen (a->string, 0);

  action_val_free (a);
  stack_push (context, d);
}

static void
action_mb_string_extract (SwfdecActionContext * context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;
  ActionVal *d;
  int len;
  int count;
  int index;
  char *start;
  char *end;

  a = stack_pop (context);
  action_val_convert_to_number (a);
  b = stack_pop (context);
  action_val_convert_to_number (b);
  c = stack_pop (context);
  action_val_convert_to_string (c);

  d = action_val_new ();
  d->type = ACTIONVAL_TYPE_STRING;
  len = g_utf8_strlen (c->string, 0);
  count = a->number;
  index = b->number;
  if (count < 0) count = 0;
  if (index < 0) index = 0;
  if (index > len) index = len;
  if (count > len - index) count = len - index;

  start = g_utf8_offset_to_pointer (c->string, index);
  end = g_utf8_offset_to_pointer (c->string, index + count);
  d->string = g_strndup (start, end - start);

  action_val_free (a);
  action_val_free (b);
  action_val_free (c);
  stack_push (context, d);
}

static void
action_to_integer (SwfdecActionContext * context)
{
  ActionVal *a;

  a = stack_pop (context);
  action_val_convert_to_number (a);

  a->number = (int)(a->number);

  stack_push (context, a);
}

static void
action_char_to_ascii (SwfdecActionContext * context)
{
  ActionVal *a;

  a = stack_pop (context);
  action_val_convert_to_string (a);

  a->number = a->string[0];
  g_free (a->string);
  a->type = ACTIONVAL_TYPE_NUMBER;

  stack_push (context, a);
}

static void
action_ascii_to_char (SwfdecActionContext * context)
{
  ActionVal *a;
  char s[2];

  a = stack_pop (context);
  action_val_convert_to_number (a);

  s[0] = a->number;
  s[1] = 0;

  a->type = ACTIONVAL_TYPE_STRING;
  a->string = strdup(s);

  stack_push (context, a);
}

static void
action_mb_char_to_ascii (SwfdecActionContext * context)
{
  ActionVal *a;

  a = stack_pop (context);
  action_val_convert_to_string (a);

  a->number = g_utf8_get_char (a->string);
  g_free (a->string);
  a->type = ACTIONVAL_TYPE_NUMBER;

  stack_push (context, a);
}

static void
action_mb_ascii_to_char (SwfdecActionContext * context)
{
  ActionVal *a;
  char s[6];
  int len;

  a = stack_pop (context);
  action_val_convert_to_number (a);

  len = g_unichar_to_utf8 (a->number, s);

  a->type = ACTIONVAL_TYPE_STRING;
  a->string = g_strndup(s,len);

  stack_push (context, a);
}

static void
action_jump (SwfdecActionContext *context)
{
  int offset;

  offset = swfdec_bits_get_s16 (&context->bits);

  if (pc_is_valid (context, context->pc + offset) == SWF_OK) {
    context->pc += offset;
  } else {
    SWFDEC_ERROR ("bad jump of %d", offset);
  }
}

static void
action_if (SwfdecActionContext *context)
{
  ActionVal *a;
  int offset;

  offset = swfdec_bits_get_s16 (&context->bits);

  a = stack_pop (context);
  action_val_convert_to_boolean (a);

  if (a->number) {
    if (pc_is_valid (context, context->pc + offset) == SWF_OK) {
      context->pc += offset;
    } else {
      SWFDEC_ERROR ("bad branch of %d", offset);
    }
  }
}

static void
action_call (SwfdecActionContext *context)
{
  ActionVal *a;

  a = stack_pop (context);
  SWFDEC_WARNING ("action call unimplemented");

  action_val_free (a);
}

static void
action_get_variable (SwfdecActionContext *context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVar *var;

  a = stack_pop (context);
  action_val_convert_to_string (a);

  var = variable_get (context, a->string);

  b = action_val_new ();
  action_val_copy (b, var->value);

  stack_push (context, b);
  action_val_free (a);
}

static void
action_set_variable (SwfdecActionContext *context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVar *var;

  a = stack_pop (context);
  b = stack_pop (context);
  action_val_convert_to_string (b);

  var = variable_get (context, b->string);
  action_val_copy (var->value, a);

  action_val_free (a);
  action_val_free (b);
}

static void
action_get_url_2 (SwfdecActionContext *context)
{
  ActionVal *a;
  ActionVal *b;
  int send_vars;
  int reserved;
  int load_target;
  int load_vars;

  send_vars = swfdec_bits_getbits (&context->bits, 2);
  reserved = swfdec_bits_getbits (&context->bits, 4);
  load_target = swfdec_bits_getbits (&context->bits, 1);
  load_vars = swfdec_bits_getbits (&context->bits, 1);

  a = stack_pop (context);
  action_val_convert_to_string (a);
  b = stack_pop (context);
  action_val_convert_to_string (b);

  SWFDEC_WARNING ("action get_url_2 unimplemented");

  action_val_free (a);
  action_val_free (b);
}

static void
action_goto_frame_2 (SwfdecActionContext *context)
{
  ActionVal *a;
  int reserved;
  int scene_bias;
  int play;

  reserved = swfdec_bits_getbits (&context->bits, 6);
  scene_bias = swfdec_bits_getbits (&context->bits, 1);
  play = swfdec_bits_getbits (&context->bits, 1);

  a = stack_pop (context);
  action_val_convert_to_string (a);

  SWFDEC_WARNING ("action goto_frame_2 unimplemented");

  action_val_free (a);
}

static void
action_set_target_2 (SwfdecActionContext *context)
{
  ActionVal *a;

  a = stack_pop (context);
  action_val_convert_to_string (a);

  SWFDEC_WARNING ("action set_target_2 unimplemented");

  action_val_free (a);
}

static void
action_get_property (SwfdecActionContext *context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *d;
  int index;

  a = stack_pop (context);
  action_val_convert_to_number (a);
  b = stack_pop (context);
  action_val_convert_to_string (b);

  d = action_val_new();
  d->type = ACTIONVAL_TYPE_NUMBER;

  index = a->number;
  if (index >= 0 && index <= 21) {
    switch (index) {
      case 5:
        d->number = context->s->n_frames; /* total frames */
        break;
      case 8:
        d->number = context->s->width; /* width */
        break;
      case 9:
        d->number = context->s->height; /* height */
        break;
      case 12:
        d->number = context->s->n_frames; /* frames loaded */
        break;
      case 16:
        d->number = 1; /* high quality */
        break;
      case 19:
        d->number = 1; /* quality */
        break;
      case 20:
        d->number = context->s->mouse_x; /* mouse x */
        break;
      case 21:
        d->number = context->s->mouse_y; /* mouse y */
        break;
      default:
        /* FIXME need to get the property here */
        SWFDEC_WARNING ("get property unimplemented (index = %d)", index);
        break;
    }
  } else {
    SWFDEC_ERROR("property index out of range");
    context->error = 1;
  }

  action_val_free (a);
  action_val_free (b);
  stack_push (context, d);
}

static void
action_set_property (SwfdecActionContext *context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;
  int index;

  a = stack_pop (context);
  b = stack_pop (context);
  action_val_convert_to_number (b);
  c = stack_pop (context);
  action_val_convert_to_string (c);

  index = b->number;
  if (index >= 0 && index <= 21) {
    SWFDEC_WARNING ("set property unimplemented");
    /* FIXME need to set the property here */
  } else {
    SWFDEC_ERROR("property index out of range");
    context->error = 1;
  }

  action_val_free (a);
  action_val_free (b);
  action_val_free (c);
}

static void
action_clone_sprite (SwfdecActionContext *context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;

  a = stack_pop (context);
  action_val_convert_to_number (a);
  b = stack_pop (context);
  action_val_convert_to_string (b);
  c = stack_pop (context);
  action_val_convert_to_string (c);

  SWFDEC_WARNING ("clone sprite unimplemented");

  action_val_free (a);
  action_val_free (b);
  action_val_free (c);
}

static void
action_remove_sprite (SwfdecActionContext *context)
{
  ActionVal *a;

  a = stack_pop (context);
  action_val_convert_to_string (a);

  SWFDEC_WARNING ("remove sprite unimplemented");

  action_val_free (a);
}

static void
action_start_drag (SwfdecActionContext *context)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;

  a = stack_pop (context);
  action_val_convert_to_string (a);
  b = stack_pop (context);
  action_val_convert_to_boolean (b);
  c = stack_pop (context);
  action_val_convert_to_boolean (c);
  if (c->number) {
    action_val_free (stack_pop (context));
    action_val_free (stack_pop (context));
    action_val_free (stack_pop (context));
    action_val_free (stack_pop (context));
  }

  SWFDEC_WARNING ("start drag unimplemented");

  action_val_free (a);
  action_val_free (b);
  action_val_free (c);
}

static void
action_end_drag (SwfdecActionContext *context)
{
  SWFDEC_WARNING ("end drag unimplemented");
}

static void
action_wait_for_frame_2 (SwfdecActionContext *context)
{
  ActionVal *a;

  a = stack_pop (context);
  action_val_convert_to_number (a);

  /* FIXME frame has always been loaded */
  if (1) {
    context->skip = swfdec_bits_get_u8 (&context->bits);
  }

  action_val_free (a);
}

static void
action_trace (SwfdecActionContext *context)
{
  ActionVal *a;

  a = stack_pop (context);

  action_val_free (a);
}

static void
action_get_time (SwfdecActionContext *context)
{
  ActionVal *a;

  a = action_val_new ();
  a->type = ACTIONVAL_TYPE_NUMBER;
  /* FIXME implement */
  //a->number = swfdec_decoder_get_time ();
  a->number = 0;

  stack_push (context, a);
}

static void
action_random_number (SwfdecActionContext *context)
{
  ActionVal *a;

  a = stack_pop (context);
  action_val_convert_to_number (a);

  a->number = g_random_int_range (0, a->number);

  stack_push (context, a);
}

static void
action_constant_pool (SwfdecActionContext *context)
{
  int n;
  int i;

  n = swfdec_bits_get_u16 (&context->bits);

  if (context->constants) {
    for (i=0;i<context->n_constants;i++){
      g_free(context->constants[i]);
    }
    g_free (context->constants);
  }
  context->n_constants = n;
  context->constants = g_malloc(sizeof(char *) * n);

  for(i=0;i<n;i++){
    context->constants[i] = swfdec_bits_get_string (&context->bits);
  }
}

