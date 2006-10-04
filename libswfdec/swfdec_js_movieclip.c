#include <ctype.h>
#include <string.h>
#include <math.h>

#include <js/jsapi.h>
#include "swfdec_internal.h"
#include "swfdec_js.h"
#include "swfdec_movieclip.h"

static void
movie_clip_finalize (JSContext *cx, JSObject *obj)
{
  SwfdecMovieClip *movie;

  movie = JS_GetPrivate (cx, obj);
  /* since we also finalize the class, not everyone has a private object */
  if (movie) {
    g_assert (movie->jsobj != NULL);

    SWFDEC_LOG ("destroying JSObject %p for movie %p", obj, movie);
    movie->jsobj = NULL;
    g_object_unref (movie);
  }
}

static JSClass movieclip_class = {
    "MovieClip", JSCLASS_NEW_RESOLVE | JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,
    JS_ConvertStub,   movie_clip_finalize,
};

static JSBool
mc_play (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovieClip *movie;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  movie->stopped = FALSE;

  return JS_TRUE;
}

static JSBool
mc_stop (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovieClip *movie;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  movie->stopped = TRUE;

  return JS_TRUE;
}

static JSBool
mc_getBytesLoaded (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecDecoder *dec = JS_GetContextPrivate (cx);

  *rval = INT_TO_JSVAL(MIN (dec->length, dec->loaded));

  return JS_TRUE;
}

static JSBool
mc_getBytesTotal(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecDecoder *dec = JS_GetContextPrivate (cx);

  *rval = INT_TO_JSVAL(dec->length);

  return JS_TRUE;
}

static JSBool
mc_gotoAndPlay (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  int32 frame;
  guint n_frames;
  SwfdecMovieClip *movie;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  
  if (!JS_ValueToInt32 (cx, argv[0], &frame))
    return JS_FALSE;
  /* FIXME: how to handle overflow? */
  n_frames = swfdec_movie_clip_get_n_frames (movie);
  frame = CLAMP (frame, 1, (int) n_frames) - 1;

  movie->next_frame = frame;
  movie->stopped = FALSE;
  return JS_TRUE;
}

static JSBool
mc_gotoAndStop (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  int32 frame;
  guint n_frames;
  SwfdecDecoder *dec;
  SwfdecMovieClip *movie;

  dec = JS_GetContextPrivate (cx);
  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  
  if (!JS_ValueToInt32 (cx, argv[0], &frame))
    return JS_FALSE;
  /* FIXME: how to handle overflow? */
  n_frames = swfdec_movie_clip_get_n_frames (movie);
  frame = CLAMP (frame, 1, (int) n_frames) - 1;

  movie->next_frame = frame;
  movie->stopped = TRUE;
  return JS_TRUE;
}

static JSBool
mc_hitTest (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  SwfdecMovieClip *movie;

  movie = JS_GetPrivate(cx, obj);
  g_assert (movie);
  
  if (argc == 1) {
    SwfdecMovieClip *other;
    if (!JSVAL_IS_OBJECT (argv[0]) ||
	JS_GetClass (JSVAL_TO_OBJECT (argv[0])) != &movieclip_class) {
      g_assert_not_reached ();
      return JS_FALSE;
    }
    other = SWFDEC_MOVIE_CLIP (JS_GetPrivate(cx, JSVAL_TO_OBJECT (argv[0])));
    /* FIXME */
    g_assert (movie->parent == other->parent);
#if 0
    g_print ("%g %g  %g %g --- %g %g  %g %g\n", 
	SWFDEC_OBJECT (movie)->extents.x0, SWFDEC_OBJECT (movie)->extents.y0,
	SWFDEC_OBJECT (movie)->extents.x1, SWFDEC_OBJECT (movie)->extents.y1,
	SWFDEC_OBJECT (other)->extents.x0, SWFDEC_OBJECT (other)->extents.y0,
	SWFDEC_OBJECT (other)->extents.x1, SWFDEC_OBJECT (other)->extents.y1);
#endif
    if (swfdec_rect_intersect (NULL, &SWFDEC_OBJECT (movie)->extents,
	  &SWFDEC_OBJECT (other)->extents)) {
      //g_print ("%s hit %s\n", movie->name ? movie->name : "?", other->name ? other->name : "?");
      *rval = BOOLEAN_TO_JSVAL (JS_TRUE);
    } else {
      *rval = BOOLEAN_TO_JSVAL (JS_FALSE);
    }
  } else if (argc == 3) {
    g_assert_not_reached ();
  } else {
    return JS_FALSE;
  }

  return JS_TRUE;
}

static SwfdecMovieClip *
get_target (SwfdecMovieClip *movie, const char *target)
{
  char *tmp;
  guint len;
  GList *walk;

  //g_print ("get_target: %s\n", target);
  if (target[0] == '\0')
    return movie;

  if (g_str_has_prefix (target, "../")) {
    if (movie->parent == NULL)
      return NULL;
    return get_target (movie->parent, target + 3);
  }
  tmp = strchr (target, '/');
  if (tmp)
    len = tmp - target;
  else
    len = strlen (target);

  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovieClip *cur = walk->data;
    if (cur->content->name && g_ascii_strncasecmp (cur->content->name, target, len) == 0)
      return get_target (cur, target + len + 1);
  }
  return NULL;
}

extern  JSPropertySpec movieclip_props[];

static JSBool
swfdec_js_getProperty (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  uint32 id;
  SwfdecMovieClip *movie;

  if (JSVAL_IS_OBJECT (argv[0])) {
    movie = JS_GetPrivate(cx, JSVAL_TO_OBJECT (argv[0]));
  } else if (JSVAL_IS_STRING (argv[0])) {
    char *str = JS_GetStringBytes (JSVAL_TO_STRING (argv[0]));
    movie = JS_GetPrivate(cx, obj);
    movie = get_target (movie, str);
    if (movie == NULL)
      return JS_FALSE;
  } else {
    return JS_FALSE;
  }
  if (!JS_ValueToECMAUint32 (cx, argv[1], &id))
    return JS_FALSE;

  if (id > 19)
    return JS_FALSE;

  if (movie->jsobj == NULL &&
      !swfdec_js_add_movieclip (movie))
    return JS_FALSE;
  return movieclip_props[id].getter (cx, movie->jsobj, JSVAL_VOID /* FIXME */, rval);
}

static JSBool
swfdec_js_setProperty (JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
  uint32 id;
  SwfdecMovieClip *movie;

  if (JSVAL_IS_OBJECT (argv[0])) {
    movie = JS_GetPrivate(cx, JSVAL_TO_OBJECT (argv[0]));
  } else if (JSVAL_IS_STRING (argv[0])) {
    char *str = JS_GetStringBytes (JSVAL_TO_STRING (argv[0]));
    movie = JS_GetPrivate(cx, obj);
    movie = get_target (movie, str);
    if (movie == NULL)
      return JS_FALSE;
  } else {
    return JS_FALSE;
  }
  if (!JS_ValueToECMAUint32 (cx, argv[1], &id))
    return JS_FALSE;

  if (id > 19)
    return JS_FALSE;

  if (movie->jsobj == NULL &&
      !swfdec_js_add_movieclip (movie))
    return JS_FALSE;
  *rval = argv[2];
  return movieclip_props[id].setter (cx, movie->jsobj, JSVAL_VOID /* FIXME */, rval);
}

static JSFunctionSpec movieclip_methods[] = {
  //{"attachMovie", mc_attachMovie, 4, 0},
  {"getBytesLoaded", mc_getBytesLoaded, 0, 0},
  {"getBytesTotal", mc_getBytesTotal, 0, 0},
  { "getProperty",    	swfdec_js_getProperty,	2, 0, 0 },
  {"gotoAndPlay", mc_gotoAndPlay, 1, 0 },
  {"gotoAndStop", mc_gotoAndStop, 1, 0 },
  {"play", mc_play, 0, 0},
  {"stop", mc_stop, 0, 0},
  {"hitTest", mc_hitTest, 1, 0},
  { "setProperty",    	swfdec_js_setProperty,	3, 0, 0 },
  {NULL}
};

#if 0
static JSBool
mc_attachMovie(JSContext *cx, JSObject *obj, uintN argc, jsval *argv,
    jsval *rval)
{
  SwfdecMovieClip *parent_movie, *child_movie;
  SwfdecSprite *parent_sprite, *attach_sprite;
  JSString *idName, *newName;
  int depth;
  JSObject *initObject, *child_mc;
  SwfdecActionContext *context;

  context = JS_GetContextPrivate (cx);

  idName = JS_ValueToString (cx, argv[0]);
  newName = JS_ValueToString (cx, argv[1]);
  JS_ValueToInt32 (cx, argv[2], &depth);
  JS_ValueToObject (cx, argv[3], &initObject);

  SWFDEC_DEBUG("placing sprite %s as %s at depth %d",
      JS_GetStringBytes (idName), JS_GetStringBytes (newName), depth);

  attach_sprite = SWFDEC_SPRITE(swfdec_exports_lookup (context->s,
    JS_GetStringBytes(idName)));
  if (!attach_sprite) {
    SWFDEC_WARNING("Couldn't find sprite %s", JS_GetStringBytes (idName));
    *rval = JSVAL_VOID;
    return JS_TRUE;
  }

  parent_movie = JS_GetPrivate (cx, obj);
  if (!parent_movie) {
    SWFDEC_WARNING("couldn't get moviement");
    *rval = JSVAL_VOID;
    return JS_TRUE;
  }
  if (parent_movie->id == 0)
    parent_sprite = context->s->main_sprite;
  else
    parent_sprite = SWFDEC_SPRITE(swfdec_object_get (context->s,
        parent_movie->id));

  swfdec_sprite_frame_remove_movie (context->s, &parent_sprite->frames[parent_movie->first_index],
      depth);

  /* FIXME we need a separate list of added moviements */
  child_movie = swfdec_spritemovie_new ();
  child_movie->depth = depth;
  cairo_matrix_init_identity (&child_movie->transform);
  swfdec_color_transform_init_identity (&child_movie->color_transform);

  swfdec_sprite_frame_add_movie (&parent_sprite->frames[parent_movie->first_index],
      child_movie);
  child_mc = movieclip_new (context, child_movie);
  *rval = OBJECT_TO_JSVAL(child_mc);

  JS_SetProperty (cx, obj, JS_GetStringBytes(newName), rval);

  return JS_TRUE;
}
#endif

static JSBool
mc_x_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovieClip *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  d = (movie->x + movie->original_extents.x0) / SWF_SCALE_FACTOR;
  return JS_NewNumberValue (cx, d, vp);
}

static JSBool
mc_x_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovieClip *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  if (!JS_ValueToNumber (cx, *vp, &d))
    return JS_FALSE;
  movie->x = d * SWF_SCALE_FACTOR - movie->original_extents.x0;
  swfdec_movie_clip_update_matrix (movie, TRUE);

  return JS_TRUE;
}

static JSBool
mc_y_get(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovieClip *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  d = (movie->y + movie->original_extents.y0) / SWF_SCALE_FACTOR;
  return JS_NewNumberValue (cx, d, vp);
}

static JSBool
mc_y_set(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovieClip *movie;
  double d;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  if (!JS_ValueToNumber (cx, *vp, &d))
    return JS_FALSE;
  movie->y = d * SWF_SCALE_FACTOR - movie->original_extents.y0;
  swfdec_movie_clip_update_matrix (movie, TRUE);

  return JS_TRUE;
}

static JSBool
mc_currentframe (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovieClip *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  *vp = INT_TO_JSVAL (movie->next_frame + 1);

  return JS_TRUE;
}

static JSBool
mc_framesloaded (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovieClip *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  if (SWFDEC_IS_SPRITE (movie->child)) {
    SwfdecSprite *sprite = SWFDEC_SPRITE (movie->child);
    *vp = INT_TO_JSVAL (sprite->parse_frame);
  } else {
    *vp = INT_TO_JSVAL (swfdec_movie_clip_get_n_frames (movie));
  }

  return JS_TRUE;
}

static JSBool
mc_totalframes (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovieClip *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  *vp = INT_TO_JSVAL (swfdec_movie_clip_get_n_frames (movie));

  return JS_TRUE;
}

static JSBool
mc_parent (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  SwfdecMovieClip *movie;

  movie = JS_GetPrivate (cx, obj);
  g_assert (movie);

  /* FIXME: what do we do if we're the root movie? */
  if (movie->parent)
    movie = movie->parent;

  if (movie->jsobj == NULL)
    swfdec_js_add_movieclip (movie);
  if (movie->jsobj == NULL)
    return JS_FALSE;

  *vp = OBJECT_TO_JSVAL (movie->jsobj);

  return JS_TRUE;
}

/* MovieClip AS standard class */

enum {
  PROP_X,
  PROP_Y,
  PROP_XSCALE,
  PROP_YSCALE,
  PROP_CURRENTFRAME,
  PROP_TOTALFRAMES,
  PROP_ALPHA,
  PROP_VISIBLE,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_ROTATION,
  PROP_FRAMESLOADED,
  PROP_NAME,
  PROP_DROPTARGET,
  PROP_URL,
  PROP_HIGHQUALITY,
  PROP_FOCUSRECT,
  PROP_SOUNDBUFTIME,
  PROP_QUALITY,
  PROP_XMOUSE,
  PROP_YMOUSE
};

static JSBool
not_reached (JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
  g_assert_not_reached ();
}

#define MC_PROP_ATTRS (JSPROP_PERMANENT|JSPROP_SHARED)
JSPropertySpec movieclip_props[] = {
  {"_x",	    PROP_X,		MC_PROP_ATTRS,			  mc_x_get,	    mc_x_set },
  {"_y",	    PROP_Y,		MC_PROP_ATTRS,			  mc_y_get,	    mc_y_set },
  {"_xscale",	    PROP_XSCALE,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_yscale",	    PROP_YSCALE,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_currentframe", PROP_CURRENTFRAME,	MC_PROP_ATTRS | JSPROP_READONLY,  mc_currentframe,  NULL },
  {"_totalframes",  PROP_TOTALFRAMES,	MC_PROP_ATTRS | JSPROP_READONLY,  mc_totalframes,   NULL },
  {"_alpha",	    PROP_ALPHA,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_visble",	    PROP_VISIBLE,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_width",	    PROP_WIDTH,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_height",	    PROP_HEIGHT,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_rotation",	    PROP_ROTATION,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_framesloaded", PROP_FRAMESLOADED,	MC_PROP_ATTRS | JSPROP_READONLY,  mc_framesloaded,  NULL },
  {"_name",	    PROP_NAME,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_droptarget",   PROP_DROPTARGET,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_url",	    PROP_URL,		MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_highquality",  PROP_HIGHQUALITY,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_focusrect",    PROP_FOCUSRECT,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_soundbuftime", PROP_SOUNDBUFTIME,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_xmouse",	    PROP_XMOUSE,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_ymouse",	    PROP_YMOUSE,	MC_PROP_ATTRS,			  not_reached,	    not_reached },
  {"_parent",	    -1,			MC_PROP_ATTRS | JSPROP_READONLY,  mc_parent,	    NULL},
  {NULL}
};

#if 0
JSObject *
movieclip_find (SwfdecActionContext *context,
    SwfdecMovieClip *movie)
{
  GList *g;
  struct mc_list_entry *listentry;

  for (g = g_list_first (context->movielist); g; g = g_list_next (g)) {
    listentry = (struct mc_list_entry *)g->data;

    if (listentry->movie == movie)
      return listentry->mc;
  }

  return NULL;
}

static void
swfdec_native_ASSetPropFlags (SwfdecActionContext *context, int num_args,
  ActionVal *_this)
{
  ActionVal *a;
  ActionVal *b;
  ActionVal *c;
  ActionVal *d;
  int allowFalse = 0;
  int flags;

  a = stack_pop (context); /* obj */
  action_val_convert_to_object (a);
  b = stack_pop (context); /* property list */
  c = stack_pop (context); /* flags */
  action_val_convert_to_number (c);
  if (num_args >= 4) {
    d = stack_pop (context); /* allowFalse */
    action_val_convert_to_boolean (d);
    allowFalse = d->number;
    action_val_free (d);
  }

  flags = (int)c->number & 0x7;
  /* The flags appear to be 0x1 for DontEnum, 0x2 for DontDelete, and 0x4 for
   * DontWrite, though the tables I found on the web are poorly written.
   */

  if (ACTIONVAL_IS_NULL(b)) {
    GList *g;

    SWFDEC_DEBUG("%d args", num_args);

    for (g = g_list_first (a->obj->properties); g; g = g_list_next (g)) {
      ScriptObjectProperty *prop = g->data;
      if (allowFalse) {
        prop->flags = flags;
      } else {
        prop->flags |= flags;
      }
    }
  } else {
    action_val_convert_to_string (b);
    SWFDEC_WARNING("ASSetPropFlags not implemented (properties %s, flags 0x%x)",
      b->string, (int)c->number);
  }

  action_val_free (a);
  action_val_free (b);
  action_val_free (c);

  a = action_val_new ();
  a->type = ACTIONVAL_TYPE_UNDEF;
  stack_push (context, a);
}
#endif

/**
 * swfdec_js_add_movieclip_class:
 * @dec: a @SwfdecDecoder
 *
 * Adds the movieclip class to the JS Context of @dec.
 **/
void
swfdec_js_add_movieclip_class (SwfdecDecoder *dec)
{
  JSObject *global = JS_GetGlobalObject (dec->jscx);

  g_assert (dec->jsmovie == NULL);
  dec->jsmovie = JS_InitClass (dec->jscx, global, NULL,
      &movieclip_class, NULL, 0, movieclip_props, movieclip_methods,
      NULL, NULL);
}

void
swfdec_js_movie_clip_add_property (SwfdecMovieClip *movie)
{
  jsval val;

  g_assert (movie->parent);
  g_assert (movie->parent->jsobj);
  if (movie->jsobj == NULL) {
    if (!swfdec_js_add_movieclip (movie))
      return;
  }
  val = OBJECT_TO_JSVAL (movie->jsobj);
  JS_SetProperty (SWFDEC_OBJECT (movie)->decoder->jscx, movie->parent->jsobj,
      movie->content->name, &val);
}

void
swfdec_js_movie_clip_remove_property (SwfdecMovieClip *movie)
{
  g_assert (movie->parent);
  g_assert (movie->parent->jsobj);
  g_assert (movie->jsobj);

  JS_DeleteProperty (SWFDEC_OBJECT (movie)->decoder->jscx, movie->parent->jsobj,
      movie->content->name);
}

/**
 * swfdec_js_add_movieclip:
 * @movie: a #SwfdecMovieClip
 *
 * Ensures that a JSObject for the given @movie exists.
 **/
gboolean
swfdec_js_add_movieclip (SwfdecMovieClip *movie)
{
  JSObject *global;
  JSContext *cx;
  GList *walk;

  g_return_val_if_fail (SWFDEC_IS_MOVIE_CLIP (movie), FALSE);
  g_return_val_if_fail (movie->jsobj == NULL, FALSE);

  cx = SWFDEC_OBJECT (movie)->decoder->jscx;
  global = JS_GetGlobalObject (cx);

  movie->jsobj = JS_NewObject (cx, &movieclip_class, NULL, NULL);
  if (movie->jsobj == NULL) {
    SWFDEC_ERROR ("failed to create JS object for movie %p", movie);
    return FALSE;
  }
  SWFDEC_LOG ("created JSObject %p for movie %p", movie->jsobj, movie);
  g_object_ref (movie);
  JS_SetPrivate (cx, movie->jsobj, movie);
  /* add all children */
  for (walk = movie->list; walk; walk = walk->next) {
    SwfdecMovieClip *child = walk->data;
    if (child->content->name)
      swfdec_js_movie_clip_add_property (child);
  }
  /* special case */
  if (movie == SWFDEC_OBJECT (movie)->decoder->root) {
    jsval val = OBJECT_TO_JSVAL (movie->jsobj);
    if (!JS_SetProperty(cx, global, "_root", &val)) {
      SWFDEC_ERROR ("failed to set root object");
      return FALSE;
    }
  }
  return TRUE;
}

#if 0
void
action_register_sprite_movie (SwfdecDecoder * s, SwfdecMovieClip *movie)
{
  SwfdecActionContext *context;
  JSObject *mc;
  JSBool ok;
  jsval val;

  SWFDEC_DEBUG ("Placing MovieClip %s", movie->name ? movie->name : "(no name)");

  if (s->context == NULL)
    swfdec_init_context (s);
  context = s->context;
#if SWFDEC_ACTIONS_DEBUG_GC
  JS_GC(context->jscx);
#endif

  mc = movieclip_new (context, movie);
  val = OBJECT_TO_JSVAL(mc);

  if (movie->name) {
    JSObject *parentclip;
    char *parentname;

    parentclip = movieclip_find (context, s->parse_sprite_movie);
    parentname = name_object (context, parentclip);
    SWFDEC_INFO("%s is a child of %s", movie->name, parentname);
    g_free (parentname);

    /* FIXME: This helps sbemail out a bit, but I'm guessing it's wrong.  There
     * are still some scope issues, it seems -- for example, a clip is created
     * while parsing _root, but is then accessed as a member of another movie
     * clip which is also a child of _root.
     */
    ok = JS_SetProperty (context->jscx, context->global, movie->name, &val);
    ok &= JS_SetProperty (context->jscx, parentclip, movie->name, &val);
    if (!ok)
      SWFDEC_WARNING("Failed to register %s", movie->name);
  }
}
#endif
