
#include <swfdec.h>
#include <swfdec_render.h>
#include <swfdec_buffer.h>

#include <glib.h>

#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>

#include <SDL.h>

#define g_print printf

gboolean debug = FALSE;
int slow = 0;

gboolean go = TRUE;
int render_time;

SwfdecDecoder *s;
unsigned char *image;
unsigned char *image4;

SDL_Surface *sdl_screen;

guint input_idle_id;
guint render_idle_id;

int width;
int height;
int fast = FALSE;
int enable_sound = TRUE;
int quit_at_eof = FALSE;

double rate;
int interval;
struct timeval image_time;

static void do_help(void);
static void new_window(void);

static void sound_setup(void);

void convert_image (unsigned char *dest, unsigned char *src, int width,
    int height);

static gboolean render_idle_audio (gpointer data);
static gboolean render_idle_noaudio (gpointer data);

int main(int argc, char *argv[])
{
	int c;
	int index;
	static struct option options[] = {
		{ "width", 1, NULL, 'w' },
		{ "height", 1, NULL, 'h' },
		{ "fast", 0, NULL, 'f' },
		{ "no-sound", 0, NULL, 's' },
		{ "quit", 0, NULL, 'q' },
		{ 0 },
	};
        char *contents;
        int length;
        int ret;

        SDL_Init (SDL_INIT_NOPARACHUTE|SDL_INIT_AUDIO|SDL_INIT_VIDEO|SDL_INIT_TIMER);

	s = swfdec_decoder_new();

	while(1){
		c = getopt_long(argc, argv, "qsfx:w:h:", options, &index);
		if(c==-1)break;

		switch(c){
		case 'w':
			width = strtoul(optarg, NULL, 0);
			printf("width set to %d\n",width);
			break;
		case 'h':
			height = strtoul(optarg, NULL, 0);
			printf("height set to %d\n",height);
			break;
		case 'f':
			fast = TRUE;
			break;
		case 's':
			enable_sound = FALSE;
			break;
		case 'q':
			quit_at_eof = TRUE;
			break;
		default:
			do_help();
			break;
		}
	}

	if(optind != argc-1)do_help();

	if(width){
		swfdec_decoder_set_image_size(s,width,height);
	}

        ret = g_file_get_contents (argv[optind], &contents, &length, NULL);
        if (!ret) {
          g_print("error reading file\n");
          exit(1);
        }
        swfdec_decoder_add_data (s, contents, length);

        while (1) {
          ret = swfdec_decoder_parse (s);
          if (ret == SWF_NEEDBITS ) {
            swfdec_decoder_eof (s);
          }
          if (ret == SWF_ERROR) {
            g_print("error while parsing\n");
            exit(1);
          }
          if (ret == SWF_EOF) {
            break;
          }
        }

        swfdec_decoder_get_rate (s, &rate);
        interval = 1000.0/rate;

        swfdec_decoder_get_image_size (s, &width, &height);

	new_window();

	if(enable_sound)sound_setup();

        render_time = 0;
        while (go) {
          int now = SDL_GetTicks ();
          if (now >= render_time) {
            if (enable_sound) {
              render_idle_audio (NULL);
            } else {
              render_idle_noaudio (NULL);
            }
          }
        }

	exit(0);
}

static void do_help(void)
{
	fprintf(stderr,"swf_play file.swf\n");
	exit(1);
}

static void new_window(void)
{
  sdl_screen = SDL_SetVideoMode(width, height, 32, SDL_SWSURFACE);
  if (sdl_screen == NULL) {
    g_print("SDL_SetVideoMode failed\n");
  }
}

GList *sound_buffers;
int sound_bytes;
unsigned char *sound_buf;

static void fill_audio(void *udata, Uint8 *stream, int len)
{
	GList *g;
	SwfdecBuffer *buffer;
	int n;
	int offset = 0;

	while(1){
		g = g_list_first(sound_buffers);
		if(!g)break;

		buffer = (SwfdecBuffer *)g->data;
                if (buffer->length < len - offset) {
                  n = buffer->length;
		  memcpy(sound_buf + offset, buffer->data, n);
                  swfdec_buffer_unref (buffer);
                  sound_buffers = g_list_delete_link (sound_buffers, g);
                } else {
                  SwfdecBuffer *subbuffer;

                  n = len - offset;
		  memcpy(sound_buf + offset, buffer->data, n);
                  subbuffer = swfdec_buffer_new_subbuffer (buffer, n,
                      buffer->length - n);
                  g->data = subbuffer;
                  swfdec_buffer_unref (buffer);
                }

		sound_bytes -= n;
		offset += n;

		if(offset >= len)break;
	}

	if(offset<len){
		memset(sound_buf+offset,0,len-offset);
	}

	SDL_MixAudio(stream, sound_buf, len, SDL_MIX_MAXVOLUME);
}

static void sound_setup(void)
{
	SDL_AudioSpec wanted;

	wanted.freq = 44100;
#if G_BYTE_ORDER == 4321
	wanted.format = AUDIO_S16MSB;
#else
	wanted.format = AUDIO_S16LSB;
#endif
	wanted.channels = 2;
	wanted.samples = 1024;
	wanted.callback = fill_audio;
	wanted.userdata = NULL;

	sound_buf = malloc(1024*2*2);

	if ( SDL_OpenAudio(&wanted, NULL) < 0 ) {
		fprintf(stderr, "Couldn't open audio: %s, disabling\n", SDL_GetError());
		enable_sound = FALSE;
	}

	SDL_PauseAudio(0);
}


static gboolean render_idle_audio(gpointer data)
{
        SwfdecBuffer *video_buffer;
        SwfdecBuffer *audio_buffer;
        gboolean ret;

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
        if (slow) {
          swfdec_buffer_ref (audio_buffer);
          sound_buffers = g_list_append (sound_buffers, audio_buffer);
	  sound_bytes += audio_buffer->length;
        }

printf("%d\n", sound_bytes);
        if (sound_bytes > 20000) {
          video_buffer = swfdec_render_get_image (s);
        } else {
          video_buffer = NULL;
        }
        if (video_buffer) {
          int ret;
          SDL_Surface *surface;

#define RED_MASK 0x00ff0000
#define GREEN_MASK 0x0000ff00
#define BLUE_MASK 0x000000ff
#define ALPHA_MASK 0xff000000
          surface = SDL_CreateRGBSurfaceFrom (video_buffer->data, width,
              height, 32, width * 4,
              RED_MASK, GREEN_MASK, BLUE_MASK, ALPHA_MASK);
          SDL_SetAlpha (surface, 0, SDL_ALPHA_OPAQUE);
          ret = SDL_BlitSurface (surface, NULL, sdl_screen, NULL);
          if (ret < 0) {
            g_print("SDL_BlitSurface failed\n");
          }
          SDL_UpdateRect (sdl_screen, 0, 0, width, height);
          swfdec_buffer_unref (video_buffer);
          SDL_FreeSurface (surface);
        }

          render_time = SDL_GetTicks () + 10;

	return FALSE;
}

static gboolean render_idle_noaudio(gpointer data)
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
              height, 32, width * 4, 0xff000000, 0x00ff0000, 0x0000ff00,
              0x000000ff);
          SDL_BlitSurface (surface, NULL, sdl_screen, NULL);
          swfdec_buffer_unref (video_buffer);
          SDL_FreeSurface (surface);
        }

          render_time = SDL_GetTicks () + 10;

	return FALSE;
}

