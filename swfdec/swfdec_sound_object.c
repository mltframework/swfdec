/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include "swfdec_sound_object.h"
#include "swfdec_as_context.h"
#include "swfdec_as_internal.h"
#include "swfdec_as_native_function.h"
#include "swfdec_as_object.h"
#include "swfdec_as_strings.h"
#include "swfdec_debug.h"
#include "swfdec_internal.h"
#include "swfdec_player_internal.h"
#include "swfdec_resource.h"
#include "swfdec_sound.h"

/*** SwfdecSoundObject ***/

G_DEFINE_TYPE (SwfdecSoundObject, swfdec_sound_object, SWFDEC_TYPE_AS_RELAY)

static void
swfdec_sound_object_mark (SwfdecGcObject *object)
{
  SwfdecSoundObject *sound = SWFDEC_SOUND_OBJECT (object);

  if (sound->target != NULL)
    swfdec_as_string_mark (sound->target);

  SWFDEC_GC_OBJECT_CLASS (swfdec_sound_object_parent_class)->mark (object);
}

static void
swfdec_sound_object_dispose (GObject *object)
{
  SwfdecSoundObject *sound = SWFDEC_SOUND_OBJECT (object);

  if (sound->provider) {
    g_object_unref (sound->provider);
    sound->provider = NULL;
  }

  G_OBJECT_CLASS (swfdec_sound_object_parent_class)->dispose (object);
}

static void
swfdec_sound_object_class_init (SwfdecSoundObjectClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  SwfdecGcObjectClass *gc_class = SWFDEC_GC_OBJECT_CLASS (klass);

  object_class->dispose = swfdec_sound_object_dispose;

  gc_class->mark = swfdec_sound_object_mark;
}

static void
swfdec_sound_object_init (SwfdecSoundObject *sound)
{
}

static SwfdecActor *
swfdec_sound_object_get_actor (SwfdecSoundObject *sound)
{
  SwfdecPlayer *player = SWFDEC_PLAYER (swfdec_gc_object_get_context (sound));
  SwfdecMovie *movie;

  movie = swfdec_player_get_movie_from_string (player, 
      sound->target ? sound->target : "");
  if (!SWFDEC_IS_ACTOR (movie))
    return NULL;
  return SWFDEC_ACTOR (movie);
}

static SwfdecSound *
swfdec_sound_object_get_sound (SwfdecSoundObject *sound, const char *name)
{
  SwfdecActor *actor = swfdec_sound_object_get_actor (sound);
  
  if (actor == NULL)
    return NULL;

  return swfdec_resource_get_export (SWFDEC_MOVIE (actor)->resource, name);
}

/*** AS CODE ***/

static SwfdecSoundMatrix *
swfdec_sound_object_get_matrix (SwfdecSoundObject *sound)
{
  if (sound->provider) {
    SwfdecSoundMatrix *ret = swfdec_sound_provider_get_matrix (sound->provider);
    if (ret)
      return ret;
  }

  if (sound->target == NULL) {
    return &SWFDEC_PLAYER (swfdec_gc_object_get_context (sound))->priv->sound_matrix;
  } else {
    SwfdecActor *actor = swfdec_sound_object_get_actor (sound);
    if (actor)
      return &actor->sound_matrix;
  }
  return NULL;
}

SWFDEC_AS_NATIVE (500, 0, swfdec_sound_object_getPan)
void
swfdec_sound_object_getPan (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  const SwfdecSoundMatrix *matrix;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "");

  matrix = swfdec_sound_object_get_matrix (sound);
  if (matrix == NULL)
    return;

  swfdec_as_value_set_integer (cx, ret, swfdec_sound_matrix_get_pan (matrix));
}

SWFDEC_AS_NATIVE (500, 1, swfdec_sound_object_getTransform)
void
swfdec_sound_object_getTransform (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  const SwfdecSoundMatrix *matrix;
  SwfdecAsObject *obj;
  SwfdecAsValue val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "");

  matrix = swfdec_sound_object_get_matrix (sound);
  if (matrix == NULL)
    return;

  obj = swfdec_as_object_new (cx, SWFDEC_AS_STR_Object, NULL);

  swfdec_as_value_set_integer (cx, &val, matrix->ll);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_ll, &val);
  swfdec_as_value_set_integer (cx, &val, matrix->lr);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_lr, &val);
  swfdec_as_value_set_integer (cx, &val, matrix->rl);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_rl, &val);
  swfdec_as_value_set_integer (cx, &val, matrix->rr);
  swfdec_as_object_set_variable (obj, SWFDEC_AS_STR_rr, &val);

  SWFDEC_AS_VALUE_SET_OBJECT (ret, obj);
}

SWFDEC_AS_NATIVE (500, 2, swfdec_sound_object_getVolume)
void
swfdec_sound_object_getVolume (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  const SwfdecSoundMatrix *matrix;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "");

  matrix = swfdec_sound_object_get_matrix (sound);
  if (matrix == NULL)
    return;

  swfdec_as_value_set_integer (cx, ret, matrix->volume);
}

SWFDEC_AS_NATIVE (500, 3, swfdec_sound_object_setPan)
void
swfdec_sound_object_setPan (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  SwfdecSoundMatrix *matrix;
  int pan;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "i", &pan);

  matrix = swfdec_sound_object_get_matrix (sound);
  if (matrix == NULL)
    return;

  swfdec_sound_matrix_set_pan (matrix, pan);
}

SWFDEC_AS_NATIVE (500, 4, swfdec_sound_object_setTransform)
void
swfdec_sound_object_setTransform (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  SwfdecSoundMatrix *matrix;
  SwfdecAsObject *trans;
  SwfdecAsValue *val;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "o", &trans);

  matrix = swfdec_sound_object_get_matrix (sound);
  if (matrix == NULL)
    return;

  /* ll */
  val = swfdec_as_object_peek_variable (trans, SWFDEC_AS_STR_ll);
  if (val) {
    matrix->ll = swfdec_as_value_to_integer (cx, val);
  } else if (swfdec_as_object_has_variable (trans, SWFDEC_AS_STR_ll) == trans) {
    matrix->ll = 0;
  }
  /* lr */
  val = swfdec_as_object_peek_variable (trans, SWFDEC_AS_STR_lr);
  if (val) {
    matrix->lr = swfdec_as_value_to_integer (cx, val);
  } else if (swfdec_as_object_has_variable (trans, SWFDEC_AS_STR_lr) == trans) {
    matrix->lr = 0;
  }
  /* rr */
  val = swfdec_as_object_peek_variable (trans, SWFDEC_AS_STR_rr);
  if (val) {
    matrix->rr = swfdec_as_value_to_integer (cx, val);
  } else if (swfdec_as_object_has_variable (trans, SWFDEC_AS_STR_rr) == trans) {
    matrix->rr = 0;
  }
  /* rl */
  val = swfdec_as_object_peek_variable (trans, SWFDEC_AS_STR_rl);
  if (val) {
    matrix->rl = swfdec_as_value_to_integer (cx, val);
  } else if (swfdec_as_object_has_variable (trans, SWFDEC_AS_STR_rl) == trans) {
    matrix->rl = 0;
  }
}

SWFDEC_AS_NATIVE (500, 5, swfdec_sound_object_setVolume)
void
swfdec_sound_object_setVolume (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  SwfdecSoundMatrix *matrix;
  int volume;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "i", &volume);

  matrix = swfdec_sound_object_get_matrix (sound);
  if (matrix == NULL)
    return;

  matrix->volume = volume;
}

SWFDEC_AS_NATIVE (500, 9, swfdec_sound_object_getDuration)
void
swfdec_sound_object_getDuration (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Sound.getDuration");
}

SWFDEC_AS_NATIVE (500, 10, swfdec_sound_object_setDuration)
void
swfdec_sound_object_setDuration (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Sound.setDuration");
}

SWFDEC_AS_NATIVE (500, 11, swfdec_sound_object_getPosition)
void
swfdec_sound_object_getPosition (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Sound.getPosition");
}

SWFDEC_AS_NATIVE (500, 12, swfdec_sound_object_setPosition)
void
swfdec_sound_object_setPosition (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Sound.setPosition");
}

SWFDEC_AS_NATIVE (500, 13, swfdec_sound_object_loadSound)
void
swfdec_sound_object_loadSound (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  SwfdecActor *actor;
  const char *url;
  gboolean stream;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "sb", &url, &stream);
  actor = swfdec_sound_object_get_actor (sound);
  if (actor == NULL)
    return;

  if (sound->provider)
    g_object_unref (sound->provider);
  sound->provider = SWFDEC_SOUND_PROVIDER (swfdec_load_sound_new (object, url));
  if (stream)
    swfdec_sound_provider_start (sound->provider, actor, 0, 1);
}

SWFDEC_AS_NATIVE (500, 14, swfdec_sound_object_getBytesLoaded)
void
swfdec_sound_object_getBytesLoaded (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Sound.getBytesLoaded");
}

SWFDEC_AS_NATIVE (500, 15, swfdec_sound_object_getBytesTotal)
void
swfdec_sound_object_getBytesTotal (SwfdecAsContext *cx, SwfdecAsObject *object,
    guint argc, SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Sound.getBytesTotal");
}

SWFDEC_AS_NATIVE (500, 18, swfdec_sound_object_get_checkPolicyFile)
void
swfdec_sound_object_get_checkPolicyFile (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Sound.checkPolicyFile (get)");
}

SWFDEC_AS_NATIVE (500, 19, swfdec_sound_object_set_checkPolicyFile)
void
swfdec_sound_object_set_checkPolicyFile (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Sound.checkPolicyFile (set)");
}

SWFDEC_AS_NATIVE (500, 16, swfdec_sound_object_areSoundsInaccessible)
void
swfdec_sound_object_areSoundsInaccessible (SwfdecAsContext *cx,
    SwfdecAsObject *object, guint argc, SwfdecAsValue *argv,
    SwfdecAsValue *ret)
{
  SWFDEC_STUB ("Sound.areSoundsInaccessible");
}

SWFDEC_AS_NATIVE (500, 7, swfdec_sound_object_attachSound)
void
swfdec_sound_object_attachSound (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  const char *name;
  SwfdecSound *new;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "s", &name);

  new = swfdec_sound_object_get_sound (sound, name);
  if (new) {
    if (sound->provider)
      g_object_unref (sound->provider);
    sound->provider = g_object_ref (new);
  }
}

SWFDEC_AS_NATIVE (500, 8, swfdec_sound_object_start)
void
swfdec_sound_object_start (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  SwfdecActor *actor;
  double offset = 0;
  int loops = 1;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "|ni", &offset, &loops);
  actor = swfdec_sound_object_get_actor (sound);
  if (actor == NULL)
    return;

  if (sound->provider == NULL) {
    SWFDEC_INFO ("no sound attached when calling Sound.start()");
    return;
  }
  if (loops <= 0)
    loops = 1;
  if (offset < 0 || !isfinite (offset))
    offset = 0;

  swfdec_sound_provider_start (sound->provider, actor, offset * 44100, loops);
}

SWFDEC_AS_NATIVE (500, 6, swfdec_sound_object_stop)
void
swfdec_sound_object_stop (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
  const char *name = NULL;
  SwfdecSound *stopme;
  SwfdecActor *actor;

  SWFDEC_AS_CHECK (SWFDEC_TYPE_SOUND_OBJECT, &sound, "|s", &name);
  actor = swfdec_sound_object_get_actor (sound);
  if (actor == NULL)
    return;

  if (name) {
    stopme = swfdec_sound_object_get_sound (sound, name);
    if (stopme == NULL)
      return;
    if (sound->provider == NULL || SWFDEC_IS_SOUND (sound->provider))
      swfdec_sound_provider_stop (SWFDEC_SOUND_PROVIDER (stopme), actor);
  } else if (sound->provider) {
    swfdec_sound_provider_stop (sound->provider, actor);
  }
}

SWFDEC_AS_NATIVE (500, 17, swfdec_sound_object_construct)
void
swfdec_sound_object_construct (SwfdecAsContext *cx, SwfdecAsObject *object, guint argc, 
    SwfdecAsValue *argv, SwfdecAsValue *ret)
{
  SwfdecSoundObject *sound;
    
  if (!swfdec_as_context_is_constructing (cx))
    return;

  sound = g_object_new (SWFDEC_TYPE_SOUND_OBJECT, "context", cx, NULL);
  swfdec_as_object_set_relay (object, SWFDEC_AS_RELAY (sound));

  if (argc == 0 || SWFDEC_AS_VALUE_IS_UNDEFINED (argv[0])) {
    sound->target = NULL;
  } else {
    sound->target = swfdec_as_value_to_string (cx, argv[0]);
  }

  SWFDEC_AS_VALUE_SET_OBJECT (ret, object);
}

