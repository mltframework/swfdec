
#include <swfdec.h>
#include <swfdec_render.h>
#include <swfdec_buffer.h>

#include <glib.h>

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include <SDL.h>
#include "spp.h"

//#define DEBUG(...) fprintf(stderr,__VA_ARGS__)
#define DEBUG(...)

typedef struct _Packet Packet;
struct _Packet
{
  int code;
  int length;
  void *data;
};
static Packet *packet_get (int fd);
static void packet_free (Packet * packet);
static void packet_write (int fd, int code, int len, const char *s);
static gboolean render_idle_audio (gpointer data);
static gboolean render_idle_noaudio (gpointer data);
static void do_help (void);
static void do_safe (int standalone);
static void new_window (void);
static void sound_setup (void);

/* vars */
static gboolean go = TRUE;
static int render_time;

static SwfdecDecoder *s;

static SDL_Surface *sdl_screen;

static int width;
static int height;

static gboolean enable_sound = TRUE;
static gboolean noskip = FALSE;
static gboolean slow = FALSE;
static gboolean safe = FALSE;
static gboolean hidden = FALSE;
//static unsigned long xid;

static double rate;
static int interval;

static GList *sound_buffers;
static int sound_bytes;
static unsigned char *sound_buf;


int
main (int argc, char *argv[])
{
  int c;
  int index;
  static struct option options[] = {
    {"help", 0, NULL, 'h'},
    {"xid", 1, NULL, 'x'},
    {"no-sound", 0, NULL, 's'},
    {"plugin", 0, NULL, 'p'},
    {"slow", 0, NULL, 'o'},
    {"safe", 0, NULL, 'f'},
    {0},
  };
  int ret;
  int standalone = TRUE;
  int rendering = FALSE;
  int starting = FALSE;
  Packet *packet;
  int fd = 0;
  int send_fd = 1;

  DEBUG ("swf_play: starting player\n");

  while (1) {
    c = getopt_long (argc, argv, "sx:p", options, &index);
    if (c == -1)
      break;

    switch (c) {
      case 's':
        enable_sound = FALSE;
        break;
      case 'x':
        //setenv ("SDL_WINDOWID", optarg, 1);
        //xid = strtol (optarg, NULL, 0);
        break;
      case 'p':
        standalone = FALSE;
        break;
      case 'o':
        slow = TRUE;
        break;
      case 'f':
        safe = TRUE;
        break;
      case 'h':
      default:
        do_help ();
        break;
    }
  }

  if (standalone) {
    if (optind != argc - 1)
      do_help ();
  } else {
    if (optind != argc)
      do_help ();
  }

  SDL_Init (SDL_INIT_NOPARACHUTE | SDL_INIT_AUDIO | SDL_INIT_VIDEO |
      SDL_INIT_TIMER);
  /* SDL thinks it's smart by overriding SIGINT.  It's not */
  signal (SIGINT, SIG_DFL);

  SDL_EventState (SDL_ACTIVEEVENT, SDL_ENABLE);

  if (safe) {
    do_safe(standalone);
  }

  s = swfdec_decoder_new ();

  if (standalone) {
    char *contents;
    gsize length;

    ret = g_file_get_contents (argv[optind], &contents, &length, NULL);
    if (!ret) {
      DEBUG ("error reading file\n");
      exit (1);
    }
    swfdec_decoder_add_data (s, (unsigned char *)contents, length);
    swfdec_decoder_eof (s);
    ret = swfdec_decoder_parse (s);
    if (ret == SWF_ERROR) {
      DEBUG ("error while parsing\n");
      exit (1);
    }
    starting = TRUE;
  }

  while (1) {
    SDL_Event event;
    int did_something = FALSE;

    if (!standalone) {
      packet = packet_get (fd);
      if (packet) {
        switch (packet->code) {
          case SPP_EXIT:
            exit (0);
            break;
          case SPP_DATA:
            swfdec_decoder_add_data (s, packet->data, packet->length);
            packet->data = NULL;
            break;
          case SPP_EOF:
            swfdec_decoder_eof (s);
            starting = TRUE;
            break;
          case SPP_SIZE:
            width = ((int *) packet->data)[0];
            height = ((int *) packet->data)[1];
            swfdec_decoder_set_image_size (s, width, height);
            new_window ();
            break;
          default:
            /* ignore it */
            break;
        }
        packet_free (packet);
        did_something = TRUE;
      }
    }

    //if (standalone) {
      if (SDL_PollEvent (&event)) {
        did_something = TRUE;
        DEBUG ("event %d\n", event.type);
        switch (event.type) {
          case SDL_VIDEORESIZE:
            width = event.resize.w;
            height = event.resize.h;
            swfdec_decoder_set_image_size (s, width, height);
            new_window ();
            break;
          case SDL_MOUSEMOTION:
            swfdec_decoder_set_mouse (s, event.motion.x, event.motion.y,
                event.motion.state);
            break;
          case SDL_MOUSEBUTTONDOWN:
          case SDL_MOUSEBUTTONUP:
            swfdec_decoder_set_mouse (s, event.button.x, event.button.y,
                event.button.state);
            break;
          case SDL_ACTIVEEVENT:
            if (event.active.state & SDL_APPMOUSEFOCUS) {
              if (event.active.gain == 0) {
                swfdec_decoder_set_mouse (s, -1, -1, 0);
              }
            }
            if (event.active.state & SDL_APPACTIVE) {
              hidden = !event.active.gain;
            }
            break;
          default:
            break;
        }
      }
    //}

    if (starting) {
      ret = swfdec_decoder_parse (s);
      DEBUG ("parse ret = %d\n", ret);
      ret = swfdec_decoder_parse (s);
      DEBUG ("parse ret = %d\n", ret);
      ret = swfdec_decoder_parse (s);
      DEBUG ("parse ret = %d\n", ret);
      ret = swfdec_decoder_parse (s);
      DEBUG ("parse ret = %d\n", ret);

      swfdec_decoder_get_rate (s, &rate);
      interval = 1000.0 / rate;

      swfdec_decoder_get_image_size (s, &width, &height);
      DEBUG ("size %dx%d\n", width, height);

      new_window ();

      if (enable_sound)
        sound_setup ();
      render_time = 0;
      starting = FALSE;
      rendering = TRUE;
      did_something = TRUE;
    }

    if (rendering) {
      int now = SDL_GetTicks ();

      if (now >= render_time) {
        char *url;

        if (enable_sound) {
          render_idle_audio (NULL);
        } else {
          render_idle_noaudio (NULL);
        }
        did_something = TRUE;
        url = swfdec_decoder_get_url (s);
        if (url) {
          DEBUG ("sending URL packet\n");
          packet_write (send_fd, SPP_GO_TO_URL, strlen(url), url);
          g_free (url);
        }
      }
    }

    if (!did_something) {
      usleep (10000);
    }
  }

  exit (0);
}

static void
do_help (void)
{
  g_print ("swf_play [--xid|-x XID] [--no-sound|-s] [--slow] file.swf\n");
  g_print ("swf_play [--xid|-x XID] [--no-sound|-s] [--slow] [--plugin|-p]\n");
  g_print ("swf_play [--help]\n");
  exit (1);
}

static void 
do_safe (int standalone)
{
  Packet *packet;
  int fd = 0;

  width = 100;
  height = 100;
  new_window ();
  SDL_FillRect (sdl_screen, NULL, 0x80808000);
  SDL_UpdateRect (sdl_screen, 0, 0, width, height);

  while (1) {
    int did_something = FALSE;
    SDL_Event event;

    if (!standalone) {
      packet = packet_get (fd);
      if (packet) {
        switch (packet->code) {
          case SPP_EXIT:
            exit (0);
            break;
          case SPP_DATA:
            swfdec_decoder_add_data (s, packet->data, packet->length);
            packet->data = NULL;
            break;
          case SPP_EOF:
            swfdec_decoder_eof (s);
            //starting = TRUE;
            break;
          case SPP_SIZE:
            width = ((int *) packet->data)[0];
            height = ((int *) packet->data)[1];
            new_window ();
            SDL_FillRect (sdl_screen, NULL, 0x80808000);
            SDL_UpdateRect (sdl_screen, 0, 0, width, height);
            break;
          default:
            /* ignore it */
            break;
        }
        packet_free (packet);
        did_something = TRUE;
      }
    }

    if (SDL_PollEvent (&event)) {
      did_something = TRUE;
      switch (event.type) {
        case SDL_VIDEORESIZE:
          width = event.resize.w;
          height = event.resize.h;
          new_window ();
          SDL_FillRect (sdl_screen, NULL, 0x80808000);
          SDL_UpdateRect (sdl_screen, 0, 0, width, height);
          break;
        case SDL_MOUSEMOTION:
          swfdec_decoder_set_mouse (s, event.motion.x, event.motion.y,
              event.motion.state);
          break;
        case SDL_MOUSEBUTTONDOWN:
        case SDL_MOUSEBUTTONUP:
          swfdec_decoder_set_mouse (s, event.button.x, event.button.y,
              event.button.state);
          break;
        default:
          break;
      }
    }

    if (!did_something) {
      usleep (10000);
    }
  }
}

static void
new_window (void)
{
  sdl_screen = SDL_SetVideoMode (width, height, 32, SDL_SWSURFACE|SDL_RESIZABLE);
  if (sdl_screen == NULL) {
    DEBUG ("SDL_SetVideoMode failed\n");
  }
}

static void
fill_audio (void *udata, Uint8 * stream, int len)
{
  GList *g;
  SwfdecBuffer *buffer;
  int n;
  int offset = 0;

  while (1) {
    g = g_list_first (sound_buffers);
    if (!g)
      break;

    buffer = (SwfdecBuffer *) g->data;
    if (buffer->length < len - offset) {
      n = buffer->length;
      memcpy (sound_buf + offset, buffer->data, n);
      swfdec_buffer_unref (buffer);
      sound_buffers = g_list_delete_link (sound_buffers, g);
    } else {
      SwfdecBuffer *subbuffer;

      n = len - offset;
      memcpy (sound_buf + offset, buffer->data, n);
      subbuffer = swfdec_buffer_new_subbuffer (buffer, n, buffer->length - n);
      g->data = subbuffer;
      swfdec_buffer_unref (buffer);
    }

    sound_bytes -= n;
    offset += n;

    if (offset >= len)
      break;
  }

  if (offset < len) {
    memset (sound_buf + offset, 0, len - offset);
  }

  SDL_MixAudio (stream, sound_buf, len, SDL_MIX_MAXVOLUME);
}

static void
sound_setup (void)
{
  SDL_AudioSpec wanted;

  if (slow) {
  wanted.freq = 22050;
  }else{
  wanted.freq = 44100;
  }
#if G_BYTE_ORDER == 4321
  wanted.format = AUDIO_S16MSB;
#else
  wanted.format = AUDIO_S16LSB;
#endif
  wanted.channels = 2;
  wanted.samples = 1024;
  wanted.callback = fill_audio;
  wanted.userdata = NULL;

  sound_buf = malloc (1024 * 2 * 2);

  if (SDL_OpenAudio (&wanted, NULL) < 0) {
    DEBUG ("Couldn't open audio: %s, disabling\n", SDL_GetError ());
    enable_sound = FALSE;
  }

  SDL_PauseAudio (0);
}


static gboolean
render_idle_audio (gpointer data)
{
  SwfdecBuffer *video_buffer;
  SwfdecBuffer *audio_buffer;
  gboolean ret;

  if (hidden) {
    int now = SDL_GetTicks ();

    render_time = now + 10;
    return FALSE;
  }

  if (sound_bytes >= 40000) {
    int now = SDL_GetTicks ();

    render_time = now + 10;
    return FALSE;
  }

  ret = swfdec_render_iterate (s);
  if (!ret) {
    go = FALSE;
    return FALSE;
  }

  audio_buffer = swfdec_render_get_audio (s);

  if (audio_buffer == NULL) {
    /* error */
    go = FALSE;
  }

  sound_buffers = g_list_append (sound_buffers, audio_buffer);
  sound_bytes += audio_buffer->length;
#if 0
  if (slow) {
    swfdec_buffer_ref (audio_buffer);
    sound_buffers = g_list_append (sound_buffers, audio_buffer);
    sound_bytes += audio_buffer->length;
  }
#endif

  if (sound_bytes > 20000 || noskip) {
    video_buffer = swfdec_render_get_image (s);
  } else {
    video_buffer = NULL;
    DEBUG ("video_buffer == NULL\n");
  }
  if (video_buffer) {
    int ret;
    SDL_Surface *surface;

    if (video_buffer->length != width * height * 4) {
      DEBUG ("video buffer wrong size (%d should be %d)\n",
          video_buffer->length, width * height * 4);
    }
#if G_BYTE_ORDER == 4321
#define RED_MASK 0x0000ff00
#define GREEN_MASK 0x00ff0000
#define BLUE_MASK 0xff000000
#define ALPHA_MASK 0x000000ff
#else
#define RED_MASK 0x00ff0000
#define GREEN_MASK 0x0000ff00
#define BLUE_MASK 0x000000ff
#define ALPHA_MASK 0xff000000
#endif
    surface = SDL_CreateRGBSurfaceFrom (video_buffer->data, width,
        height, 32, width * 4, RED_MASK, GREEN_MASK, BLUE_MASK, ALPHA_MASK);
    SDL_SetAlpha (surface, 0, SDL_ALPHA_OPAQUE);
    ret = SDL_BlitSurface (surface, NULL, sdl_screen, NULL);
    if (ret < 0) {
      DEBUG ("SDL_BlitSurface failed\n");
    }
    SDL_UpdateRect (sdl_screen, 0, 0, width, height);
    swfdec_buffer_unref (video_buffer);
    SDL_FreeSurface (surface);
  }

  render_time = SDL_GetTicks () + 10;

  return FALSE;
}

static gboolean
render_idle_noaudio (gpointer data)
{
  SwfdecBuffer *video_buffer;
  SwfdecBuffer *audio_buffer;

  swfdec_render_iterate (s);

  video_buffer = swfdec_render_get_image (s);
  audio_buffer = swfdec_render_get_audio (s);

  swfdec_buffer_unref (audio_buffer);
  if (video_buffer) {
    SDL_Surface *surface;

    surface = SDL_CreateRGBSurfaceFrom (video_buffer->data, width,
        height, 32, width * 4, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
    SDL_BlitSurface (surface, NULL, sdl_screen, NULL);
    swfdec_buffer_unref (video_buffer);
    SDL_FreeSurface (surface);
  }

  render_time = SDL_GetTicks () + 10;

  return FALSE;
}

static gboolean
fd_is_ready (int fd)
{
  fd_set readfds;
  struct timeval tv = { 0 };

  FD_ZERO (&readfds);
  FD_SET (fd, &readfds);

  select (fd + 1, &readfds, NULL, NULL, &tv);
  if (!FD_ISSET (fd, &readfds)) {
    return FALSE;
  }
  return TRUE;
}

static Packet *
packet_get (int fd)
{
  Packet *packet;

  if (!fd_is_ready (fd)) {
    return NULL;
  }

  packet = malloc (sizeof (Packet));
  read (fd, &packet->code, 4);
  read (fd, &packet->length, 4);
  if (packet->length > 0) {
    packet->data = malloc (packet->length);
    read (fd, packet->data, packet->length);
  }

  DEBUG ("swf_play: packet code=%d length=%d\n", packet->code, packet->length);

  return packet;
}

static void
packet_free (Packet * packet)
{
  if (packet->data)
    free (packet->data);
  free (packet);
}

static void
packet_write (int fd, int code, int len, const char *s)
{
  char *buf;

  buf = g_malloc (len + 8);
  memcpy (buf, &code, 4);
  memcpy (buf + 4, &len, 4);
  memcpy (buf + 8, s, len);
  write (fd, buf, len + 8);

  g_free (buf);
}

