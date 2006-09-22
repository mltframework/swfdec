
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zlib.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <liboil/liboil.h>

#include "swfdec.h"
#include "swfdec_audio.h"
#include "swfdec_bits.h"
#include "swfdec_debug.h"
#include "swfdec_decoder.h"
#include "swfdec_js.h"
#include "swfdec_movieclip.h"
#include "swfdec_sound.h"
#include "swfdec_sprite.h"

enum {
  PROP_0,
  PROP_INVALID
};

int swf_parse_header1 (SwfdecDecoder * s);
int swf_inflate_init (SwfdecDecoder * s);
int swf_parse_header2 (SwfdecDecoder * s);

G_DEFINE_TYPE (SwfdecDecoder, swfdec_decoder, G_TYPE_OBJECT)

void
swfdec_init (void)
{
  static gboolean _inited = FALSE;
  const char *s;

  if (_inited)
    return;

  _inited = TRUE;

  g_type_init ();
  oil_init ();

  s = g_getenv ("SWFDEC_DEBUG");
  if (s && s[0]) {
    char *end;
    int level;

    level = strtoul (s, &end, 0);
    if (end[0] == 0) {
      swfdec_debug_set_level (level);
    }
  }
  swfdec_js_init (0);
}

static void
swfdec_decoder_invalidate_cb (SwfdecMovieClip *root, const SwfdecRect *rect, SwfdecDecoder *s)
{
  gboolean was_empty = swfdec_rect_is_empty (&s->invalid);
  SWFDEC_DEBUG ("toplevel invalidation: %g %g  %g %g", rect->x0, rect->y0, rect->x1, rect->y1);
  swfdec_rect_union (&s->invalid, &s->invalid, rect);
  if (was_empty)
    g_object_notify (G_OBJECT (s), "invalid");
}

SwfdecDecoder *
swfdec_decoder_new (void)
{
  SwfdecDecoder *s;

  swfdec_init ();

  s = g_object_new (SWFDEC_TYPE_DECODER, NULL);

  return s;
}

static void
swfdec_decoder_get_property (GObject *object, guint param_id, GValue *value, 
    GParamSpec * pspec)
{
  SwfdecDecoder *dec = SWFDEC_DECODER (object);
  
  switch (param_id) {
    case PROP_INVALID:
      g_value_set_boolean (value, swfdec_rect_is_empty (&dec->invalid));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_decoder_set_property (GObject *object, guint param_id, const GValue *value,
    GParamSpec *pspec)
{
  //SwfdecDecoder *dec = SWFDEC_DECODER (object);

  switch (param_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}

static void
swfdec_decoder_dispose (GObject *object)
{
  SwfdecDecoder * s = SWFDEC_DECODER (object);

  g_array_free (s->audio_events, TRUE);
  g_object_unref (s->root);

  /* this must happen while the JS Context is still alive */
  g_list_foreach (s->characters, (GFunc) swfdec_object_unref, NULL);
  g_list_free (s->characters);
  g_object_unref (s->main_sprite);

  /* make sure all SwfdecObject's are gone before calling this */
  swfdec_js_finish_decoder (s);

  if (s->buffer)
    g_free (s->buffer);

  swfdec_buffer_queue_free (s->input_queue);

  if (s->z) {
    inflateEnd (s->z);
    g_free (s->z);
  }

  if (s->jpegtables) {
    swfdec_buffer_unref (s->jpegtables);
  }

  if (s->tmp_scanline) {
    g_free (s->tmp_scanline);
  }

  g_free (s->password);

  G_OBJECT_CLASS (swfdec_decoder_parent_class)->dispose (object);
}

static void
swfdec_decoder_class_init (SwfdecDecoderClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = swfdec_decoder_get_property;
  object_class->set_property = swfdec_decoder_set_property;
  object_class->dispose = swfdec_decoder_dispose;

  g_object_class_install_property (object_class, PROP_INVALID,
      g_param_spec_boolean ("invalid", "invalid", "decoder contains invalid areas",
	  TRUE, G_PARAM_READABLE));
}

static void
swfdec_decoder_init (SwfdecDecoder *s)
{
  s->input_queue = swfdec_buffer_queue_new ();

  s->main_sprite = swfdec_object_new (s, SWFDEC_TYPE_SPRITE);
  s->main_sprite->object.id = 0;
  s->root = swfdec_object_new (s, SWFDEC_TYPE_MOVIE_CLIP);
  cairo_matrix_scale (&s->root->transform, SWF_SCALE_FACTOR, SWF_SCALE_FACTOR);
  cairo_matrix_scale (&s->root->inverse_transform, 1 / SWF_SCALE_FACTOR, 1 / SWF_SCALE_FACTOR);
  swfdec_movie_clip_set_child (s->root, SWFDEC_OBJECT (s->main_sprite));
  g_signal_connect (s->root, "invalidate", G_CALLBACK (swfdec_decoder_invalidate_cb), s);

  s->audio_events = g_array_new (FALSE, FALSE, sizeof (SwfdecAudioEvent));
  swfdec_js_init_decoder (s);
  swfdec_js_add_movieclip (s->root);
}

int
swfdec_decoder_add_data (SwfdecDecoder * s, const unsigned char *data,
    int length)
{
  SwfdecBuffer *buffer = swfdec_buffer_new ();

  buffer->data = (unsigned char *) data;
  buffer->length = length;

  return swfdec_decoder_add_buffer (s, buffer);
}

int
swfdec_decoder_add_buffer (SwfdecDecoder * s, SwfdecBuffer * buffer)
{
  int ret;

  s->loaded += buffer->length;
  if (s->compressed) {
    int offset = s->z->total_out;

    s->z->next_in = buffer->data;
    s->z->avail_in = buffer->length;
    ret = inflate (s->z, Z_SYNC_FLUSH);
    if (ret < 0) {
      return SWF_ERROR;
    }

    swfdec_buffer_unref (buffer);

    buffer = swfdec_buffer_new_subbuffer (s->uncompressed_buffer, offset,
        s->z->total_out - offset);
    swfdec_buffer_queue_push (s->input_queue, buffer);
  } else {
    swfdec_buffer_queue_push (s->input_queue, buffer);
  }
  /* FIXME: run events for bytes loaded */

  return SWF_OK;
}

void
swfdec_decoder_eof (SwfdecDecoder * s)
{
  if (s->state == SWF_STATE_PARSETAG) {
    s->state = SWF_STATE_EOF;
  }
}

int
swfdec_decoder_parse (SwfdecDecoder * s)
{
  int ret = SWF_OK;
  unsigned char *endptr;
  SwfdecBuffer *buffer;

  while (ret == SWF_OK) {
    s->b = s->parse;

    switch (s->state) {
      case SWF_STATE_INIT1:
        ret = swf_parse_header1 (s);
        break;
      case SWF_STATE_INIT2:
        ret = swf_parse_header2 (s);
        break;
      case SWF_STATE_PARSETAG:
      {
        int header_length;
        int x;
        SwfdecTagFunc *func;
        int tag;
        int tag_len;

        /* we're parsing tags */
        buffer = swfdec_buffer_queue_peek (s->input_queue, 2);
        if (buffer == NULL) {
          return SWF_NEEDBITS;
        }

        s->b.buffer = buffer;
        s->b.ptr = buffer->data;
        s->b.idx = 0;
        s->b.end = buffer->data + buffer->length;


        x = swfdec_bits_get_u16 (&s->b);
        tag = (x >> 6) & 0x3ff;
        SWFDEC_DEBUG ("tag %d %s", tag, swfdec_decoder_get_tag_name (tag));
        tag_len = x & 0x3f;
        if (tag_len == 0x3f) {
          swfdec_buffer_unref (buffer);
          buffer = swfdec_buffer_queue_peek (s->input_queue, 6);
          if (buffer == NULL) {
            return SWF_NEEDBITS;
          }
          s->b.buffer = buffer;
          s->b.ptr = buffer->data;
          s->b.idx = 0;
          s->b.end = buffer->data + buffer->length;

          swfdec_bits_get_u16 (&s->b);
          tag_len = swfdec_bits_get_u32 (&s->b);
          header_length = 6;
        } else {
          header_length = 2;
        }
        swfdec_buffer_unref (buffer);

        SWFDEC_INFO ("parsing at %d, tag %d %s, length %d",
            swfdec_buffer_queue_get_offset (s->input_queue), tag,
            swfdec_decoder_get_tag_name (tag), tag_len);

        if (swfdec_buffer_queue_get_depth (s->input_queue) < tag_len +
            header_length) {
          return SWF_NEEDBITS;
        }

        buffer = swfdec_buffer_queue_pull (s->input_queue, header_length);
        swfdec_buffer_unref (buffer);

        if (tag_len > 0) {
          buffer = swfdec_buffer_queue_pull (s->input_queue, tag_len);
          s->b.buffer = buffer;
          s->b.ptr = buffer->data;
          s->b.idx = 0;
          s->b.end = buffer->data + buffer->length;
          endptr = s->b.ptr + tag_len;
        } else {
          buffer = NULL;
          s->b.buffer = NULL;
          s->b.ptr = NULL;
          s->b.idx = 0;
          s->b.end = NULL;
          endptr = NULL;
        }
        func = swfdec_decoder_get_tag_func (tag);
        if (func == NULL) {
          SWFDEC_WARNING ("tag function not implemented for %d %s",
              tag, swfdec_decoder_get_tag_name (tag));
        } else {
          s->parse_sprite = s->main_sprite;
          ret = func (s);
          s->parse_sprite = NULL;

          swfdec_bits_syncbits (&s->b);
          if (s->b.ptr < endptr) {
            SWFDEC_WARNING
                ("early finish (%d bytes) at %d, tag %d %s, length %d",
                endptr - s->b.ptr,
                swfdec_buffer_queue_get_offset (s->input_queue), tag,
                swfdec_decoder_get_tag_name (tag), tag_len);
            //dumpbits (&s->b);
          }
          if (s->b.ptr > endptr) {
            SWFDEC_WARNING
                ("parse_overrun (%d bytes) at %d, tag %d %s, length %d",
                s->b.ptr - endptr,
                swfdec_buffer_queue_get_offset (s->input_queue), tag,
                swfdec_decoder_get_tag_name (tag), tag_len);
          }
        }

        if (tag == 0) {
          s->state = SWF_STATE_EOF;

          SWFDEC_INFO ("decoded points %d", s->stats_n_points);
        }

        if (buffer)
          swfdec_buffer_unref (buffer);

      }
        break;
      case SWF_STATE_EOF:
        ret = SWF_EOF;

        break;
    }
  }

  return ret;
}

int
swfdec_decoder_get_n_frames (SwfdecDecoder * s, int *n_frames)
{
  if (s->state == SWF_STATE_INIT1 || s->state == SWF_STATE_INIT2) {
    return SWF_ERROR;
  }

  if (n_frames)
    *n_frames = s->n_frames;
  return SWF_OK;
}

int
swfdec_decoder_get_rate (SwfdecDecoder * s, double *rate)
{
  if (s->state == SWF_STATE_INIT1 || s->state == SWF_STATE_INIT2) {
    return SWF_ERROR;
  }

  if (rate)
    *rate = s->rate / 256.0;
  return SWF_OK;
}

int
swfdec_decoder_get_version (SwfdecDecoder *s)
{
  return s->version;
}

int
swfdec_decoder_get_image (SwfdecDecoder * s, unsigned char **image)
{
  if (s->buffer == NULL) {
    return SWF_ERROR;
  }

  if (image)
    *image = s->buffer;
  s->buffer = NULL;

  return SWF_OK;
}

int
swfdec_decoder_peek_image (SwfdecDecoder * s, unsigned char **image)
{
  if (s->buffer == NULL) {
    return SWF_ERROR;
  }

  if (image)
    *image = s->buffer;

  return SWF_OK;
}

int
swfdec_decoder_get_image_size (SwfdecDecoder * s, int *width, int *height)
{
  if (s->state == SWF_STATE_INIT1 || s->state == SWF_STATE_INIT2) {
    return SWF_ERROR;
  }

  if (width)
    *width = s->width;
  if (height)
    *height = s->height;
  return SWF_OK;
}

char *
swfdec_decoder_get_url (SwfdecDecoder * s)
{
  char *url = s->url;

  s->url = NULL;

  return url;
}

static void *
zalloc (void *opaque, unsigned int items, unsigned int size)
{
  return g_malloc (items * size);
}

static void
zfree (void *opaque, void *addr)
{
  g_free (addr);
}

#if 0
static void
dumpbits (SwfdecBits * b)
{
  int i;

  printf ("    ");
  for (i = 0; i < 16; i++) {
    printf ("%02x ", swfdec_bits_get_u8 (b));
  }
  printf ("\n");
}
#endif

int
swf_parse_header1 (SwfdecDecoder * s)
{
  int sig1, sig2, sig3;
  SwfdecBuffer *buffer;

  buffer = swfdec_buffer_queue_pull (s->input_queue, 8);
  if (buffer == NULL) {
    return SWF_NEEDBITS;
  }

  s->b.buffer = buffer;
  s->b.ptr = buffer->data;
  s->b.idx = 0;
  s->b.end = buffer->data + buffer->length;

  sig1 = swfdec_bits_get_u8 (&s->b);
  sig2 = swfdec_bits_get_u8 (&s->b);
  sig3 = swfdec_bits_get_u8 (&s->b);

  s->version = swfdec_bits_get_u8 (&s->b);
  s->length = swfdec_bits_get_u32 (&s->b);

  swfdec_buffer_unref (buffer);

  if ((sig1 != 'F' && sig1 != 'C') || sig2 != 'W' || sig3 != 'S') {
    return SWF_ERROR;
  }

  s->compressed = (sig1 == 'C');
  if (s->compressed) {
    SWFDEC_DEBUG ("compressed");
    swf_inflate_init (s);
  } else {
    SWFDEC_DEBUG ("not compressed");
  }

  s->state = SWF_STATE_INIT2;

  return SWF_OK;
}

int
swf_inflate_init (SwfdecDecoder * s)
{
  SwfdecBuffer *buffer;
  z_stream *z;
  int ret;
  int offset;

  z = g_new0 (z_stream, 1);
  s->z = z;
  z->zalloc = zalloc;
  z->zfree = zfree;
  ret = inflateInit (z);
  SWFDEC_DEBUG ("inflateInit returned %d", ret);

  s->uncompressed_buffer = swfdec_buffer_new_and_alloc (s->length);
  z->next_out = s->uncompressed_buffer->data;
  z->avail_out = s->length;
  z->opaque = NULL;

  offset = z->total_out;
  buffer = swfdec_buffer_queue_pull (s->input_queue,
      swfdec_buffer_queue_get_depth (s->input_queue));
  z->next_in = buffer->data;
  z->avail_in = buffer->length;

  ret = inflate (z, Z_SYNC_FLUSH);
  SWFDEC_DEBUG ("inflate returned %d", ret);

  swfdec_buffer_unref (buffer);

  s->input_queue = swfdec_buffer_queue_new ();

  buffer = swfdec_buffer_new_subbuffer (s->uncompressed_buffer, offset,
      z->total_out - offset);
  swfdec_buffer_queue_push (s->input_queue, buffer);

  return SWF_OK;
}

int
swf_parse_header2 (SwfdecDecoder * s)
{
  double width, height;
  int n;
  SwfdecBuffer *buffer;

  buffer = swfdec_buffer_queue_peek (s->input_queue, 32);
  if (buffer == NULL) {
    return SWF_NEEDBITS;
  }

  s->b.buffer = buffer;
  s->b.ptr = buffer->data;
  s->b.idx = 0;
  s->b.end = buffer->data + buffer->length;

  swfdec_bits_get_rect (&s->b, &s->invalid);
  if (s->invalid.x0 != 0.0 || s->invalid.y0 != 0.0)
    SWFDEC_ERROR ("SWF window doesn't start at 0 0 but at %g %g\n", s->invalid.x0, s->invalid.y0);
  swfdec_rect_scale (&s->invalid, &s->invalid, SWF_SCALE_FACTOR);
  width = s->invalid.x1;
  height = s->invalid.y1;
  s->parse_width = width;
  s->parse_height = height;
  s->width = floor (width);
  s->height = floor (height);
  s->irect.x0 = 0;
  s->irect.y0 = 0;
  s->irect.x1 = s->width;
  s->irect.y1 = s->height;
  swfdec_movie_clip_set_size (s->root, s->width * 20, s->height * 20);
  swfdec_bits_syncbits (&s->b);
  s->rate = swfdec_bits_get_u16 (&s->b);
  s->samples_overhead = 44100 * 256 % s->rate;
  SWFDEC_LOG ("rate = %g", s->rate / 256.0);
  s->n_frames = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG ("n_frames = %d", s->n_frames);

  n = s->b.ptr - s->b.buffer->data;
  swfdec_buffer_unref (buffer);

  buffer = swfdec_buffer_queue_pull (s->input_queue, n);

  swfdec_sprite_set_n_frames (s->main_sprite, s->n_frames);

  s->state = SWF_STATE_PARSETAG;
  return SWF_CHANGE;
}

/**
 * swfdec_decoder_iterate:
 * @dec: the #SwfdecDecoder to iterate
 *
 * Advances #dec to the next frame. You should make sure to call this function
 * as often per second as swfdec_decoder_get_rate() indicates.
 * After calling this function @invalidated will be set to the area that 
 * changed. This value can be passed to swfdec_player_render() to get an 
 * updated image.
 **/
void
swfdec_decoder_iterate (SwfdecDecoder *dec)
{
  g_return_if_fail (dec != NULL);

  g_assert (swfdec_js_script_queue_is_empty (dec));
  g_object_freeze_notify (G_OBJECT (dec));
  /* iterate audio before video so we don't iterate audio clips that get added this frame */
  swfdec_audio_iterate_start (dec);
  swfdec_movie_clip_iterate (dec->root);
  swfdec_decoder_execute_scripts (dec);
  swfdec_audio_iterate_finish (dec);
  g_object_thaw_notify (G_OBJECT (dec));

#if 0
  if (dec->root->current_frame == 2)
    swfdec_js_run (dec, "var foo = 7; with (this) { foo = 3; target = this.foobar; _parent = target + foo; }");
#endif
}

/**
 * swfdec_decoder_render:
 * @dec: a #SwfdecDecoder
 * @cr: #cairo_t to render to
 * @area: #SwfdecRect describing the area to render or NULL for whole area
 *
 * Renders the given @area of the current frame to @cr.
 **/
void
swfdec_decoder_render (SwfdecDecoder *dec, cairo_t *cr, SwfdecRect *area)
{
  static const SwfdecColorTransform trans = { { 1.0, 1.0, 1.0, 1.0 }, { 0.0, 0.0, 0.0, 0.0 } };
  gboolean was_empty;

  g_return_if_fail (dec != NULL);
  g_return_if_fail (cr != NULL);

  if (area == NULL)
    area = &dec->irect;
  if (swfdec_rect_is_empty (area))
    return;
  was_empty = swfdec_rect_is_empty (&dec->invalid);
  SWFDEC_LOG ("%p: starting rendering, area %g %g  %g %g", dec, 
      area->x0, area->y0, area->x1, area->y1);
  swfdec_object_render (SWFDEC_OBJECT (dec->root), cr, &trans, area);
  SWFDEC_LOG ("%p: finished rendering", dec);
  swfdec_rect_subtract (&dec->invalid, &dec->invalid, area);
  if (!was_empty)
    g_object_notify (G_OBJECT (dec), "invalid");
}

/**
 * swfdec_decoder_handle_mouse:
 * @dec: a #SwfdecDecoder
 * @x: x coordinate of mouse
 * @y: y coordinate of mouse
 * @button: 1 for pressed, 0 for not pressed
 *
 * Updates the current mouse status. If the mouse has left the area of @dec,
 * you should pass values outside the movie size for @x and @y.
 * The area passed in as @inval takes the area that was updated during the mouse 
 * handling.
 **/
void
swfdec_decoder_handle_mouse (SwfdecDecoder *dec, 
    double x, double y, int button)
{
  g_return_if_fail (dec != NULL);
  g_return_if_fail (button == 0 || button == 1);

  g_assert (swfdec_js_script_queue_is_empty (dec));
  SWFDEC_LOG ("handling mouse at %g %g %d", x, y, button);
  g_object_freeze_notify (G_OBJECT (dec));
  swfdec_movie_clip_handle_mouse (dec->root, x, y, button);
  swfdec_decoder_execute_scripts (dec);
  g_object_thaw_notify (G_OBJECT (dec));
}

/**
 * swfdec_decoder_get_invalid:
 * @s: a #SwfdecDecoder
 * @rect: pointer to a rectangle to be filled with the invalid area
 *
 * The decoder accumulates the parts that need a redraw. This function gets
 * the rectangle that encloses these parts. Use swfdec_clear_invalid() to clear
 * the accumulated parts.
 **/
void
swfdec_decoder_get_invalid (SwfdecDecoder *s, SwfdecRect *rect)
{
  g_return_if_fail (SWFDEC_IS_DECODER (s));
  g_return_if_fail (rect != NULL);

  *rect = s->invalid;
}

/**
 * swfdec_decoder_clear_invalid:
 * @s: a #SwfdecDecoder
 *
 * Clears the list of areas that need a redraw.
 **/
void
swfdec_decoder_clear_invalid (SwfdecDecoder *s)
{
  gboolean was_empty;
  g_return_if_fail (SWFDEC_IS_DECODER (s));

  was_empty = swfdec_rect_is_empty (&s->invalid);
  swfdec_rect_init_empty (&s->invalid);
  if (!was_empty)
    g_object_notify (G_OBJECT (s), "invalid");
}
