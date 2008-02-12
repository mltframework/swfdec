/* Swfdec
 * Copyright (C) 2006-2008 Benjamin Otte <otte@gnome.org>
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

#include <string.h>
#include "swfdec_loader_internal.h"
#include "swfdec_buffer.h"
#include "swfdec_debug.h"
#include "swfdec_stream_target.h"
#include "swfdec_player_internal.h"

/*** gtk-doc ***/

/**
 * SECTION:SwfdecStream
 * @title: SwfdecStream
 * @short_description: object used for input
 *
 * SwfdecStream is the base class used for communication inside Swfdec. If you
 * are a UNIX developer, think of this class as the equivalent to a file 
 * descriptor. #SwfdecLoader and #SwfdecSocket are the subclasses supposed to
 * be used for files or network sockets, respectively.
 *
 * This class provides the functions necessary to implement subclasses of 
 * streams. None of the functions described in this section should be used by
 * anything but subclass implementations. Consider them "protected".
 */

/**
 * SwfdecStream:
 *
 * This is the base object used for providing input. It is abstract, use a 
 * subclass to provide your input. All members are considered private.
 */

/**
 * SwfdecStreamClass:
 * @describe: Provide a string describing your string. Default implementations
 *	      of this function exist for both the #SwfdecLoader and 
 *	      #SwfdecStream subclasses. They return the URL for the stream.
 * @close: Called when Swfdec requests that the stream be closed. After this
 *         function was called, Swfdec will consider the stream finished and
 *         will not ever read data from it again.
 *
 * This is the base class used for providing input. You are supposed to create
 * a subclass that fills in the function pointers mentioned above.
 */

/*** SwfdecStream ***/

typedef enum {
  SWFDEC_STREAM_STATE_CONNECTING = 0, 	/* stream is still in the process of establishing a connection */
  SWFDEC_STREAM_STATE_OPEN,		/* stream is open and data flow is happening */
  SWFDEC_STREAM_STATE_CLOSED,		/* loader has been closed */
  SWFDEC_STREAM_STATE_ERROR		/* loader is in error state */
} SwfdecStreamState;

struct _SwfdecStreamPrivate
{
  SwfdecPlayer *	player;		/* player to queue target notificaions in */
  SwfdecStreamTarget *	target;		/* SwfdecStreamTarget that gets notified about loading progress */
  SwfdecStreamState	state;		/* SwfdecStreamState the stream is currently in */
  SwfdecStreamState	processed_state;/* SwfdecStreamState the target knows about */
  gboolean		queued;		/* TRUE if we have queued an action already */
  char *		error;		/* error message if in error state or NULL */
  SwfdecBufferQueue *	queue;		/* SwfdecBufferQueue managing the input buffers */
};

enum {
  PROP_0,
  PROP_ERROR,
  PROP_OPEN,
  PROP_EOF
};

G_DEFINE_ABSTRACT_TYPE (SwfdecStream, swfdec_stream, G_TYPE_OBJECT)

static void
swfdec_stream_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecStreamPrivate *stream = SWFDEC_STREAM (object)->priv;
  
  switch (param_id) {
    case PROP_ERROR:
      g_value_set_string (value, stream->error);
      break;
    case PROP_OPEN:
      g_value_set_boolean (value, stream->state == SWFDEC_STREAM_STATE_OPEN);
      break;
    case PROP_EOF:
      g_value_set_boolean (value, stream->state == SWFDEC_STREAM_STATE_CLOSED);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_stream_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  //SwfdecStream *stream = SWFDEC_STREAM (object);

  switch (param_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_stream_dispose (GObject *object)
{
  SwfdecStreamPrivate *stream = SWFDEC_STREAM (object)->priv;

  /* targets are supposed to keep a reference around */
  g_assert (stream->target == NULL);
  if (stream->queue) {
    swfdec_buffer_queue_unref (stream->queue);
    stream->queue = NULL;
  }
  g_free (stream->error);
  stream->error = NULL;

  G_OBJECT_CLASS (swfdec_stream_parent_class)->dispose (object);
}

static void
swfdec_stream_class_init (SwfdecStreamClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (SwfdecStreamPrivate));

  object_class->dispose = swfdec_stream_dispose;
  object_class->get_property = swfdec_stream_get_property;
  object_class->set_property = swfdec_stream_set_property;

  g_object_class_install_property (object_class, PROP_ERROR,
      g_param_spec_string ("error", "error", "NULL when no error or string describing error",
	  NULL, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_OPEN,
      g_param_spec_boolean ("open", "open", "TRUE while data is flowing",
	  FALSE, G_PARAM_READABLE));
  g_object_class_install_property (object_class, PROP_EOF,
      g_param_spec_boolean ("eof", "eof", "TRUE when all data has been transmitted",
	  FALSE, G_PARAM_READABLE));
}

static void
swfdec_stream_init (SwfdecStream *stream)
{
  SwfdecStreamPrivate *priv;

  stream->priv = priv = G_TYPE_INSTANCE_GET_PRIVATE (stream, SWFDEC_TYPE_STREAM, SwfdecStreamPrivate);

  priv->queue = swfdec_buffer_queue_new ();
}

/*** INTERNAL API ***/

SwfdecBufferQueue *
swfdec_stream_get_queue (SwfdecStream *stream)
{
  g_return_val_if_fail (SWFDEC_IS_STREAM (stream), NULL);

  return stream->priv->queue;
}

static void swfdec_stream_queue_processing (SwfdecStream *stream);

static void
swfdec_stream_process (gpointer streamp, gpointer unused)
{
  SwfdecStream *stream = streamp;
  SwfdecStreamPrivate *priv = stream->priv;

  g_assert (priv->target != NULL);

  priv->queued = FALSE;
  if (priv->state == priv->processed_state &&
      priv->state != SWFDEC_STREAM_STATE_OPEN)
    return;
  g_assert (priv->processed_state != SWFDEC_STREAM_STATE_CLOSED);
  g_object_ref (stream);
  if (priv->state == SWFDEC_STREAM_STATE_ERROR) {
    swfdec_stream_target_error (priv->target, stream);
  } else {
    while (priv->state != priv->processed_state) {
      if (priv->processed_state == SWFDEC_STREAM_STATE_CONNECTING) {
	priv->processed_state = SWFDEC_STREAM_STATE_OPEN;
	swfdec_stream_target_open (priv->target, stream);
      } else if (priv->processed_state == SWFDEC_STREAM_STATE_OPEN) {
	if (swfdec_stream_target_parse (priv->target, stream)) {
	  g_print ("requeue!\n");
	  swfdec_stream_queue_processing (stream);
	  goto out;
	} else {
	  priv->processed_state = SWFDEC_STREAM_STATE_CLOSED;
	  swfdec_stream_target_close (priv->target, stream);
	}
      }
    }
    if (priv->processed_state == SWFDEC_STREAM_STATE_OPEN) {
      if (swfdec_stream_target_parse (priv->target, stream)) {
	g_print ("requeue!\n");
	swfdec_stream_queue_processing (stream);
      }
    }
  }
out:
  g_object_unref (stream);
}

static void
swfdec_stream_queue_processing (SwfdecStream *stream)
{
  SwfdecStreamPrivate *priv = stream->priv;

  if (priv->queued)
    return;
  priv->queued = TRUE;
  if (priv->target) {
    g_assert (priv->player);
    swfdec_player_add_external_action (priv->player, stream,
	swfdec_stream_process, NULL);
  }
}

void
swfdec_stream_close (SwfdecStream *stream)
{
  SwfdecStreamPrivate *priv;
  SwfdecStreamClass *klass;

  g_return_if_fail (SWFDEC_IS_STREAM (stream));

  priv = stream->priv;
  if (priv->state == SWFDEC_STREAM_STATE_ERROR &&
      priv->state == SWFDEC_STREAM_STATE_CLOSED)
    return;

  klass = SWFDEC_STREAM_GET_CLASS (stream);

  if (klass->close)
    klass->close (stream);
  priv->state = SWFDEC_STREAM_STATE_CLOSED;
  priv->processed_state = SWFDEC_STREAM_STATE_CLOSED;
}

void
swfdec_stream_set_target (SwfdecStream *stream, SwfdecStreamTarget *target)
{
  SwfdecStreamPrivate *priv;

  g_return_if_fail (SWFDEC_IS_STREAM (stream));
  if (target != NULL) {
    g_return_if_fail (stream->priv->processed_state == SWFDEC_STREAM_STATE_CONNECTING);
    g_return_if_fail (SWFDEC_IS_STREAM_TARGET (target));
  }

  priv = stream->priv;
  if (priv->target) {
    swfdec_player_remove_all_external_actions (priv->player, stream);
  }
  priv->queued = FALSE;
  priv->target = target;
  if (target) {
    priv->player = swfdec_stream_target_get_player (target);
    if (priv->state != SWFDEC_STREAM_STATE_CONNECTING)
      swfdec_stream_queue_processing (stream);
  } else {
    priv->player = NULL;
  }
}

/** PUBLIC API ***/

/**
 * swfdec_stream_describe:
 * @stream: a #SwfdecStream
 *
 * Describes the stream in a simple string. This is mostly useful for debugging
 * purposes.
 *
 * Returns: a constant string describing the stream
 **/
const char *
swfdec_stream_describe (SwfdecStream *stream)
{
  SwfdecStreamClass *klass;

  g_return_val_if_fail (SWFDEC_IS_STREAM (stream), NULL);

  klass = SWFDEC_STREAM_GET_CLASS (stream);
  g_return_val_if_fail (klass->describe, NULL);

  return klass->describe (stream);
}

/**
 * swfdec_stream_error:
 * @stream: a #SwfdecStream
 * @error: a printf-style string describing the error
 * @...: arguments for the @error string
 *
 * Moves the stream in the error state if it wasn't before. A stream that is in
 * the error state will not process any more data. Also, internal error 
 * handling scripts may be executed.
 **/
void
swfdec_stream_error (SwfdecStream *stream, const char *error, ...)
{
  va_list args;

  g_return_if_fail (SWFDEC_IS_STREAM (stream));
  g_return_if_fail (error != NULL);

  va_start (args, error);
  swfdec_stream_errorv (stream, error, args);
  va_end (args);
}

/**
 * swfdec_stream_errorv:
 * @stream: a #SwfdecStream
 * @error: a printf-style error string
 * @args: arguments for @error
 *
 * This function is the va_list alternative to swfdec_stream_error(). See that
 * function for details.
 **/
void
swfdec_stream_errorv (SwfdecStream *stream, const char *error, va_list args)
{
  SwfdecStreamPrivate *priv;
  char *real_error;

  g_return_if_fail (SWFDEC_IS_STREAM (stream));
  g_return_if_fail (error != NULL);

  real_error = g_strdup_vprintf (error, args);
  priv = stream->priv;
  if (priv->error) {
    SWFDEC_ERROR ("another error in stream for %s: %s", 
	swfdec_stream_describe (stream), real_error);
    g_free (real_error);
    return;
  }

  SWFDEC_ERROR ("error in stream for %s: %s", 
      swfdec_stream_describe (stream), real_error);
  priv->state = SWFDEC_STREAM_STATE_ERROR;
  priv->error = real_error;
  swfdec_stream_queue_processing (stream);
}

/**
 * swfdec_stream_open:
 * @stream: a #SwfdecStream
 *
 * Call this function when your stream opened the resulting file. For HTTP this
 * is when having received the headers. You must call this function before 
 * swfdec_stream_push() can be called.
 **/
void
swfdec_stream_open (SwfdecStream *stream)
{
  g_return_if_fail (SWFDEC_IS_STREAM (stream));
  g_return_if_fail (stream->priv->state == SWFDEC_STREAM_STATE_CONNECTING);

  stream->priv->state = SWFDEC_STREAM_STATE_OPEN;
  g_object_notify (G_OBJECT (stream), "open");
  swfdec_stream_queue_processing (stream);
}

/**
 * swfdec_stream_push:
 * @stream: a #SwfdecStream
 * @buffer: new data to make available. The stream takes the reference
 *          to the buffer.
 *
 * Makes the data in @buffer available to @stream and processes it. The @stream
 * must be open.
 **/
void
swfdec_stream_push (SwfdecStream *stream, SwfdecBuffer *buffer)
{
  g_return_if_fail (SWFDEC_IS_STREAM (stream));
  g_return_if_fail (stream->priv->state == SWFDEC_STREAM_STATE_OPEN);
  g_return_if_fail (buffer != NULL);

  swfdec_buffer_queue_push (stream->priv->queue, buffer);
  /* FIXME */
  if (SWFDEC_IS_LOADER (stream))
    g_object_notify (G_OBJECT (stream), "loaded");
  swfdec_stream_queue_processing (stream);
}

/**
 * swfdec_stream_eof:
 * @stream: a #SwfdecStream
 *
 * Indicates to @stream that no more data will follow. The stream must be open.
 **/
void
swfdec_stream_eof (SwfdecStream *stream)
{
  g_return_if_fail (SWFDEC_IS_STREAM (stream));
  g_return_if_fail (stream->priv->state == SWFDEC_STREAM_STATE_OPEN);

  stream->priv->state = SWFDEC_STREAM_STATE_CLOSED;
  g_object_notify (G_OBJECT (stream), "open");
  g_object_notify (G_OBJECT (stream), "eof");
  swfdec_stream_queue_processing (stream);
}

