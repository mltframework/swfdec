
#ifndef __SWFDEC_ACTIONS_H__
#define __SWFDEC_ACTIONS_H__

#include <swfdec_decoder.h>
#include <swfdec_buffer.h>

#include "js/jsapi.h"

typedef struct
{
  int n_constants;
  char **constants;
  int refcount;
} ConstantPool;

typedef JSBool SwfdecNativeMethod (SwfdecActionContext * context, int num_args);

typedef struct
{
  char *name;
  unsigned char *pc;
  SwfdecNativeMethod *meth;
  int num_params, register_count, code_size;
  char preload_parent, preload_root, suppress_super, preload_super;
  char suppress_args, preload_args, suppress_this, preload_this;
  char preload_global, is_function2;
  unsigned char *param_regs;
  SwfdecBuffer *buffer;
  ConstantPool *constantpool;
} ScriptFunction;

typedef struct
{
  unsigned char *returnpc; /* pc to return to. */
  SwfdecBuffer *returnbuffer; /* buffer for returnpc, for next pc_is_valid() */
  ConstantPool *returnpool; /* constant pool of previous frame */
  SwfdecBits bits;	/* bits corresponding to this frame */
  jsval *returnargv;
  int returnargc;
  /* endpc marks the end of this function body, when an automatic return should
   * happen (apparently).
   */
  unsigned char *endpc;
  char done;		/* Set when executing an ActionReturn or an end-marker */

  /* registers are only valid if is_function2 is set */
  unsigned char is_function2;
  JSObject *registers;	/* Array of 256 registers for the frame. */
} CallFrame;

struct _SwfdecActionContext
{
  SwfdecDecoder *s;
  SwfdecBits bits;
  int error;
  JSObject *registers;	/* Array of 4 global registers */
  int action;
  unsigned char *pc;
  int skip;

  GQueue *call_stack;
  int call_depth;

  JSObject *stack;
  int stack_top;

  jsval *tag_argv;
  int tag_argc;

  ConstantPool *constantpool;

  JSRuntime *jsrt;
  JSContext *jscx;
  JSObject *global;		/* The _global object */
  JSObject *root;		/* The current _root object */

  GList *seglist;
};

typedef void SwfdecActionFunc (SwfdecActionContext * context);

typedef struct
{
  int action;
  SwfdecActionFunc *func;
  char *name;
} ActionFuncEntry;

/* Turns on forcing of GC at various points, which is useful in debugging JS
 * misuse.
 */
#define SWFDEC_ACTIONS_DEBUG_GC 0

/* action_script_call relies on a number of spare entries in argv which are
 * rooted for the GC.  3 are for the TAG_[ABC] used in tag funcs, and one is the
 * array containing the registers.
 */
#define FUNCTION_TEMPORARIES		4

/* actions.c */
void swfdec_init_context (SwfdecDecoder * s);
int swfdec_action_script_execute (SwfdecDecoder * s, SwfdecBuffer * buffer);
int tag_func_do_init_action (SwfdecDecoder * s);
int tag_func_do_action (SwfdecDecoder * s);
void action_register_sprite_seg (SwfdecDecoder * s, SwfdecSpriteSegment *seg);
void constant_pool_ref (ConstantPool *pool);
void constant_pool_unref (ConstantPool *pool);
jsval stack_pop (SwfdecActionContext *context);
void stack_push (SwfdecActionContext *context, jsval value);
void stack_show (SwfdecActionContext *context);
void stack_show_value (SwfdecActionContext *context, jsval val);
JSBool action_script_call(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, 
	jsval *rval);
int pc_is_valid (SwfdecActionContext *context, unsigned char *pc);
JSObject *jsval_as_object (SwfdecActionContext *context, jsval val);
char *name_object (SwfdecActionContext *context, JSObject *obj);

/* actions_tags.c */
ActionFuncEntry *get_action (int action);

/* actions_builtin.c */
void swfdec_init_context_builtins (SwfdecActionContext *context);
JSObject *movieclip_find (SwfdecActionContext *context,
    SwfdecSpriteSegment *seg);

#endif
