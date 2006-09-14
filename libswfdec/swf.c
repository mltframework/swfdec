
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <zlib.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "swfdec_internal.h"
#include "swfdec_js.h"

int swf_parse_header1 (SwfdecDecoder * s);
int swf_inflate_init (SwfdecDecoder * s);
int swf_parse_header2 (SwfdecDecoder * s);


void
swfdec_init (void)
{
  static gboolean _inited = FALSE;
  const char *s;

  if (_inited)
    return;

  _inited = TRUE;

  g_type_init ();

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

SwfdecDecoder *
swfdec_decoder_new (void)
{
  SwfdecDecoder *s;

  swfdec_init ();

  s = g_new0 (SwfdecDecoder, 1);

  s->input_queue = swfdec_buffer_queue_new ();

  s->main_sprite = swfdec_object_new (SWFDEC_TYPE_SPRITE);
  s->main_sprite->object.id = 0;
  s->objects = g_list_append (s->objects, s->main_sprite);
  s->main_sprite_seg = swfdec_spriteseg_new ();
  s->main_sprite_seg->id = SWFDEC_OBJECT(s->main_sprite)->id;

  s->flatness = 0.5;

  swfdec_audio_add_stream(s);

  s->cache = swfdec_cache_new();

  swfdec_js_init_decoder (s);

  return s;
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
          s->parse_sprite_seg = s->main_sprite_seg;
          ret = func (s);
          s->parse_sprite = NULL;
          s->parse_sprite_seg = NULL;
          //if(ret != SWF_OK)break;

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
swfdec_decoder_free (SwfdecDecoder * s)
{
  swfdec_js_finish_decoder (s);

  g_list_foreach (s->objects, (GFunc) swfdec_object_unref, NULL);
  g_list_free (s->objects);
  /* s->main_sprite is already freed now */

  if (s->buffer)
    g_free (s->buffer);

  swfdec_buffer_queue_free (s->input_queue);

  swfdec_spriteseg_free (s->main_sprite_seg);

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

  swfdec_cache_free (s->cache);

  g_free (s);

  return SWF_OK;
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
    *rate = s->rate;
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

#if 0
void *
swfdec_decoder_get_sound_chunk (SwfdecDecoder * s, int *length)
{
  GList *g;
  SwfdecSoundBuffer *buffer;
  void *data;

  g = g_list_first (s->sound_buffers);
  if (!g)
    return NULL;

  buffer = (SwfdecSoundBuffer *) g->data;

  s->sound_buffers = g_list_delete_link (s->sound_buffers, g);

  data = buffer->data;
  if (length)
    *length = buffer->len;

  g_free (buffer);

  return data;
}
#endif

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
  SwfdecRect rect;
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

  swfdec_bits_get_rect (&s->b, &rect);
  width = rect.x1;
  height = rect.y1;
  s->parse_width = width;
  s->parse_height = height;
  s->width = floor (width);
  s->height = floor (height);
  s->irect.x0 = 0;
  s->irect.y0 = 0;
  s->irect.x1 = s->width;
  s->irect.y1 = s->height;
  swfdec_bits_syncbits (&s->b);
  s->rate = swfdec_bits_get_u16 (&s->b) / 256.0;
  SWFDEC_LOG ("rate = %g", s->rate);
  s->n_frames = swfdec_bits_get_u16 (&s->b);
  SWFDEC_LOG ("n_frames = %d", s->n_frames);

  n = s->b.ptr - s->b.buffer->data;
  swfdec_buffer_unref (buffer);

  buffer = swfdec_buffer_queue_pull (s->input_queue, n);

  swfdec_sprite_set_n_frames (s->main_sprite, s->n_frames);

  s->state = SWF_STATE_PARSETAG;
  return SWF_CHANGE;
}

SwfdecBuffer *
swfdec_decoder_get_audio (SwfdecDecoder * s)
{
  g_return_val_if_fail (s->current_frame < s->n_frames, NULL);

  if (s->stream_sound_obj) {
    SwfdecBuffer *chunk;

    chunk = s->main_sprite->frames[s->current_frame].sound_chunk;
    if (chunk) {
      SwfdecSound *sound;
      int n;

      sound = SWFDEC_SOUND (s->stream_sound_obj);

      n = chunk->length;
      if (sound->tmpbuflen + n > 2048) {
        n = 2048 - sound->tmpbuflen;
        SWFDEC_WARNING ("clipping audio");
      }
      memcpy (sound->tmpbuf + sound->tmpbuflen, chunk->data, n);
      sound->tmpbuflen += n;
      swfdec_sound_mp3_decode_stream (s, s->stream_sound_obj);
    }
  }

  if (s->main_sprite->frames[s->current_frame].sound_play) {
    SwfdecSound *sound;
    SwfdecSoundChunk *chunk =
      s->main_sprite->frames[s->current_frame].sound_play;

    SWFDEC_DEBUG("chunk %p frame_index %d", chunk, s->current_frame);
    SWFDEC_DEBUG("play sound object=%d start=%d stop=%d stopflag=%d no_restart=%d loop_count=%d",
        chunk->object, chunk->start_sample, chunk->stop_sample,
        chunk->stop, chunk->no_restart, chunk->loop_count);

    sound = SWFDEC_SOUND(swfdec_object_get (s, chunk->object));
    if (sound) {
      swfdec_audio_add_sound (s, sound, chunk->loop_count);
    }
  }

  return swfdec_audio_render (s, 44100/s->rate);
}

#include <signal.h>
/**
 * swfdec_decoder_iterate:
 * @dec: the #SwfdecDecoder to iterate
 * @invalidated: if not NULL, will be set to the area that changed
 *
 * Advances #dec to the next frame. You should make sure to call this function
 * as often per second as swfdec_decoder_get_rate() indicates.
 * After calling this function @invalidated will be set to the area that 
 * changed. This value can be passed to swfdec_player_render() to get an 
 * updated image.
 **/
void
swfdec_decoder_iterate (SwfdecDecoder *dec, SwfdecRect *invalidated)
{
  SwfdecRect invalidate_dontcare;

  g_return_if_fail (dec != NULL);

  if (dec->main_sprite_seg->stopped) {
    SWFDEC_DEBUG ("not iterating, we're stopped");
    return;
  }
  dec->last_frame = dec->current_frame;
  dec->current_frame = dec->next_frame;
  SWFDEC_DEBUG ("iterate, frame_index = %d", dec->current_frame);
  dec->next_frame = -1;
  if (invalidated == NULL)
    invalidated = &invalidate_dontcare;

  g_assert (dec->execute_list == NULL);
  swfdec_object_iterate (dec, SWFDEC_OBJECT (dec->main_sprite), invalidated);
  swfdec_rect_transform (invalidated, invalidated, &dec->main_sprite_seg->transform);

  swfdec_decoder_execute_scripts (dec);

  if (dec->next_frame == -1) {
    if (!dec->main_sprite_seg->stopped) {
      dec->next_frame = dec->current_frame + 1;
      if (dec->next_frame >= dec->n_frames) {
        if (0 /*s->repeat*/) {
          dec->next_frame = 0;
        } else {
          dec->next_frame = dec->n_frames - 1;
        }
      }
    } else {
      dec->next_frame = dec->current_frame;
    }
  }
  if (dec->current_frame == 23) {
    jsval rval;
    char *s = "_root.stop ();";
    JSObject *global = JS_GetGlobalObject (dec->jscx);
    JSScript *script = JS_CompileScript (dec->jscx, global, s, strlen (s), "bla", 1);
    G_BREAKPOINT ();
    JS_ExecuteScript (dec->jscx, global, script, &rval);
  }
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
  g_return_if_fail (dec != NULL);
  g_return_if_fail (cr != NULL);
  if (area == NULL)
    area = &dec->irect;
  SWFDEC_LOG ("%p: starting rendering of frame %d, area %g %g  %g %g", dec, dec->current_frame,
      area->x0, area->y0, area->x1, area->y1);
  if (!swfdec_rect_is_empty (area))
    swfdec_object_render (dec, SWFDEC_OBJECT (dec->main_sprite), cr, 
	&dec->main_sprite_seg->transform, &dec->main_sprite_seg->color_transform, area);
  SWFDEC_LOG ("%p: finished rendering", dec);
}

/**
 * swfdec_decoder_handle_mouse:
 * @dec: a #SwfdecDecoder
 * @x: x coordinate of mouse
 * @y: y coordinate of mouse
 * @button: 1 for pressed, 0 for not pressed
 * @inval: area that was invalidated during the mouse update or NULL
 *
 * Updates the current mouse status. If the mouse has left the area of @dec,
 * you should pass values outside the movie size for @x and @y.
 * The area passed in as @inval takes the area that was updated during the mouse 
 * handling.
 **/
void
swfdec_decoder_handle_mouse (SwfdecDecoder *dec, 
    double x, double y, int button, SwfdecRect *inval)
{
  SwfdecRect invalidate_dontcare;

  g_return_if_fail (dec != NULL);
  g_return_if_fail (button == 0 || button == 1);

  if (inval == NULL)
    inval = &invalidate_dontcare;
  dec->mouse.x = x;
  dec->mouse.y = y;
  dec->mouse.button = button;

  g_assert (dec->execute_list == NULL);
  swfdec_matrix_transform_point_inverse (&dec->main_sprite_seg->transform, &x, &y);
  swfdec_object_handle_mouse (dec, SWFDEC_OBJECT (dec->main_sprite), x, y, button, TRUE, inval);
  swfdec_rect_transform (inval, inval, &dec->main_sprite_seg->transform);
  swfdec_decoder_execute_scripts (dec);
}
