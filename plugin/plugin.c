
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#define XP_UNIX 1
#define MOZ_X11 1
#include "npapi.h"
#include "npupp.h"
#include "spp.h"



typedef struct
{
  NPP instance;
  Display *display;
  Window window;
  int x;
  int y;
  int width, height;
  int recv_fd, send_fd;
  int player_pid;
  void *buffer_data;
  int buffer_len;
  int buffer_offset;
  pthread_t thread;
  int run_thread;
} Plugin;

void DEBUG (const char *format, ...);

static void packet_write (int fd, int code, int len, void *data);
static NPNetscapeFuncs mozilla_funcs;
static char * get_formats (void);

static int n_helpers;

static void
plugin_fork (Plugin * plugin, int safe)
{
  int fds[4];

  pipe (fds);
  pipe (fds + 2);

  plugin->recv_fd = fds[0];
  plugin->send_fd = fds[3];

  plugin->player_pid = fork ();
  if (plugin->player_pid == 0) {
    char xid_str[20];
    char *argv[20];
    int argc = 0;
    sigset_t sigset;

    memset (&sigset, 0, sizeof(sigset_t));
    sigprocmask (SIG_SETMASK, &sigset, NULL);
    
    sprintf (xid_str, "%ld", plugin->window);

    /* child */
    dup2 (fds[2], 0);
    dup2(fds[1],1);

    argv[argc++] = "swfdec-mozilla-player";
    argv[argc++] = "--xid";
    argv[argc++] = xid_str;
    argv[argc++] = "--plugin";
    if (safe) {
      argv[argc++] = "--safe";
    }
    argv[argc] = NULL;

    execv (BINDIR "/swfdec-mozilla-player", argv);

    DEBUG ("plugin: failed to exec");

    _exit (255);
  }

  close (fds[1]);
  close (fds[2]);
}

static void *
plugin_thread (void *arg)
{
  Plugin *plugin = arg;
  int safe = 0;

  DEBUG ("starting thread");
  while (plugin->run_thread) {
    fd_set read_fds;
    fd_set except_fds;
    struct timeval timeout;
    int max_fd = 0;
    int ret;

    FD_ZERO (&read_fds);
    FD_ZERO (&except_fds);
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    if (plugin->recv_fd > 0) {
      FD_SET (plugin->recv_fd, &read_fds);
      FD_SET (plugin->recv_fd, &except_fds);
      max_fd = plugin->recv_fd;
    }

    ret = select (max_fd + 1, &read_fds, NULL, &except_fds, &timeout);
    if (ret < 0) {
      DEBUG ("select failed %d", errno);
    } else if (ret == 0) {
      DEBUG ("select timeout");
      /* timeout */
    } else {
      if (plugin->recv_fd > 0 && FD_ISSET (plugin->recv_fd, &read_fds)) {
        char buf[100];
        int n;

        DEBUG ("something to read");
        n = read (plugin->recv_fd, buf, 100);
        if (n < 0) {
          DEBUG ("read returned %d (%d)", n, errno);
        } else if (n == 0) {
          /* this means the child closed the descriptor, i.e., died */
          DEBUG ("read returned 0");
          close (plugin->recv_fd);
          plugin->recv_fd = -1;

          if (plugin->run_thread) {
            if (safe == 0) {
              //plugin_fork (plugin, TRUE);
              safe = 1;
            } else {
              /* helper app failed in safe mode.  oops. */
            }
          }
        } else {
          switch (*(int *)buf) {
            case SPP_GO_TO_URL:
              {
                int len;
                
                len = *(int *)(buf + 4);

                DEBUG ("%.*s", len, buf + 8);

                mozilla_funcs.geturl (plugin->instance, buf + 8, "_self");
              }
            default:
              break;
          }
        }

      }
      if (plugin->recv_fd > 0 && FD_ISSET (plugin->recv_fd, &except_fds)) {
        DEBUG ("some exception");
      }

    }
  }
  DEBUG ("stopping thread");

  return NULL;
}


static NPError
plugin_newp (NPMIMEType mime_type, NPP instance,
    uint16_t mode, int16_t argc, char *argn[], char *argv[],
    NPSavedData * saved)
{
  Plugin *plugin;
  int i;

  DEBUG ("plugin_newp instance=%p", instance);

  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  if (n_helpers >= 4) {
    return NPERR_OUT_OF_MEMORY_ERROR;
  }
  n_helpers++;

  instance->pdata = mozilla_funcs.memalloc (sizeof (Plugin));
  plugin = (Plugin *) instance->pdata;

  if (plugin == NULL)
    return NPERR_OUT_OF_MEMORY_ERROR;
  memset (plugin, 0, sizeof (Plugin));

  /* mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h) */
  //DEBUG ("mode %d",mode);
  //DEBUG ("mime type: %s",pluginType);
  plugin->instance = instance;

  for (i = 0; i < argc; i++) {
    //DEBUG ("argv[%d] %s %s",i,argn[i],argv[i]);
    if (strcmp (argn[i], "width") == 0) {
      plugin->width = strtol (argv[i], NULL, 0);
    }
    if (strcmp (argn[i], "height") == 0) {
      plugin->height = strtol (argv[i], NULL, 0);
    }
  }

  plugin->run_thread = TRUE;
  pthread_create (&plugin->thread, NULL, plugin_thread, plugin);

  return NPERR_NO_ERROR;
}

static NPError
plugin_destroy (NPP instance, NPSavedData ** save)
{
  Plugin *plugin;
  int status;

  DEBUG ("plugin_destroy instance=%p", instance);

  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  plugin = (Plugin *) instance->pdata;
  if (plugin == NULL) {
    return NPERR_NO_ERROR;
  }

  n_helpers--;

  close (plugin->send_fd);
  close (plugin->recv_fd);

  if (plugin->player_pid > 0) {
#if 0
    packet_write (plugin->send_fd, SPP_EXIT, 0, NULL);
#endif

    kill (plugin->player_pid, SIGKILL);
    waitpid (plugin->player_pid, &status, 0);
  }
  plugin->run_thread = FALSE;
  pthread_join (plugin->thread, NULL);

  mozilla_funcs.memfree (instance->pdata);
  instance->pdata = NULL;

  return NPERR_NO_ERROR;
}

static NPError
plugin_set_window (NPP instance, NPWindow * window)
{
  Plugin *plugin;

  DEBUG ("plugin_set_window instance=%p", instance);

  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  plugin = (Plugin *) instance->pdata;
  if (plugin == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  if (plugin->window) {
    DEBUG ("existing window");
    if (plugin->window == (Window) window->window) {
      int size[2];

      DEBUG ("resize");
      size[0] = window->width;
      size[1] = window->height;

      packet_write (plugin->send_fd, SPP_SIZE, 8, size);
    } else {
      DEBUG ("change");
      //DEBUG ("ack.  window changed!");
    }
  } else {
    NPSetWindowCallbackStruct *ws_info;

    DEBUG ("about to fork");

    ws_info = window->ws_info;
    plugin->window = (Window) window->window;
    plugin->display = ws_info->display;

    XSelectInput (plugin->display, plugin->window, 0);
#if 0
        (EnterWindowMask | LeaveWindowMask | ButtonPressMask |
         ButtonReleaseMask | PointerMotionMask | ExposureMask ));
#endif

    plugin_fork (plugin, FALSE);

    //fcntl(plugin->send_fd, F_SETFL, O_NONBLOCK);
  }

  DEBUG ("leaving plugin_set_window");

  return NPERR_NO_ERROR;
}

static NPError
plugin_new_stream (NPP instance, NPMIMEType type,
    const char *window, NPStream ** stream_ptr)
{
  DEBUG ("plugin_new_stream instance=%p", instance);

  return NPERR_NO_ERROR;
}

static NPError
plugin_destroy_stream (NPP instance, NPStream * stream, NPError reason)
{
  Plugin *plugin;

  DEBUG ("plugin_destroy_stream instance=%p", instance);

  if (instance == NULL)
    return 0;
  plugin = (Plugin *) instance->pdata;

  if (plugin == NULL)
    return 0;

  if (!plugin->player_pid)
    return 0;

  packet_write (plugin->send_fd, SPP_EOF, 0, NULL);

  return NPERR_NO_ERROR;
}

static int32
plugin_write_ready (NPP instance, NPStream * stream)
{
  //DEBUG ("plugin_write_ready");

  /* This is arbitrary */
  return 4096;
}

static int32
plugin_write (NPP instance, NPStream * stream, int32 offset,
    int32 len, void *buffer)
{
  Plugin *plugin;

  DEBUG ("plugin_write instance=%p", instance);

  if (instance == NULL)
    return 0;
  plugin = (Plugin *) instance->pdata;

  if (plugin == NULL)
    return 0;

  if (!plugin->player_pid)
    return 0;

  packet_write (plugin->send_fd, SPP_DATA, len, buffer);

  return len;
}

static void
plugin_stream_as_file (NPP instance, NPStream * stream, const char *fname)
{
  Plugin *plugin;

  DEBUG ("plugin_stream_as_file instance=%p", instance);

  if (instance == NULL)
    return;
  plugin = (Plugin *) instance->pdata;

  if (plugin == NULL)
    return;

  //printf("plugin_stream_as_file\n");
}

static int
plugin_event (NPP instance, void *event)
{
  DEBUG ("plugin_event instance=%p", instance);

  return TRUE;
}

static NPError
plugin_set_value (NPP instance, NPNVariable var, void *value)
{
  DEBUG ("plugin_set_value instance=%p", instance);

  return NPERR_NO_ERROR;
}

/* exported functions */

NPError
NP_GetValue (void *future, NPPVariable variable, void *value)
{
  NPError err = NPERR_NO_ERROR;

  switch (variable) {
    case NPPVpluginNameString:
      *((char **) value) = "Shockwave Flash";
      break;
    case NPPVpluginDescriptionString:
      *((char **) value) =
          "Shockwave Flash 4.0 animation viewer handled by swfdec-"
          VERSION
          ".  Plays SWF animations, commonly known as Macromedia&reg; "
          "Flash&reg;.<br><br>"
          "This is alpha software.  It will probably behave in many "
          "situations, but may also ride your motorcycle, drink all "
          "your milk, or use your computer to browse porn.  Comments, "
          "feature requests, and patches are welcome.<br><br>"
          "See <a href=\"http://www.schleef.org/swfdec/\">"
          "http://www.schleef.org/swfdec/</a> for information.<br><br>"
          "Flash, Shockwave, and Macromedia are trademarks of "
          "Macromedia, Inc. The swfdec software and its contributors "
          "are not affiliated with Macromedia, Inc.";
      break;
    default:
      err = NPERR_GENERIC_ERROR;
  }
  return err;
}

char *
NP_GetMIMEDescription (void)
{
  return get_formats();
}

NPError
NP_Initialize (NPNetscapeFuncs * moz_funcs, NPPluginFuncs * plugin_funcs)
{
  DEBUG ("NP_Initialize");

  if (moz_funcs == NULL || plugin_funcs == NULL)
    return NPERR_INVALID_FUNCTABLE_ERROR;

  if ((moz_funcs->version >> 8) > NP_VERSION_MAJOR)
    return NPERR_INCOMPATIBLE_VERSION_ERROR;
  if (moz_funcs->size < sizeof (NPNetscapeFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;
  if (plugin_funcs->size < sizeof (NPPluginFuncs))
    return NPERR_INVALID_FUNCTABLE_ERROR;

  memcpy (&mozilla_funcs, moz_funcs, sizeof (NPNetscapeFuncs));

  plugin_funcs->size = sizeof (NPPluginFuncs);
  plugin_funcs->version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
  plugin_funcs->newp = NewNPP_NewProc (plugin_newp);
  plugin_funcs->destroy = NewNPP_DestroyProc (plugin_destroy);
  plugin_funcs->setwindow = NewNPP_SetWindowProc (plugin_set_window);
  plugin_funcs->newstream = NewNPP_NewStreamProc (plugin_new_stream);
  plugin_funcs->destroystream =
      NewNPP_DestroyStreamProc (plugin_destroy_stream);
  plugin_funcs->asfile = NewNPP_StreamAsFileProc (plugin_stream_as_file);
  plugin_funcs->writeready = NewNPP_WriteReadyProc (plugin_write_ready);
  plugin_funcs->write = NewNPP_WriteProc (plugin_write);
  plugin_funcs->print = NULL;
  plugin_funcs->event = NewNPP_HandleEventProc (plugin_event);
  plugin_funcs->urlnotify = NULL;
  plugin_funcs->javaClass = NULL;
  plugin_funcs->getvalue = NULL;
  plugin_funcs->setvalue = NewNPP_SetValueProc (plugin_set_value);

  return NPERR_NO_ERROR;
}

NPError
NP_Shutdown (void)
{
  return NPERR_NO_ERROR;
}

#if 0

NPError
NPP_DestroyStream (NPP instance, NPStream * stream, NPError reason)
{
  PluginInstance *This;

  if (instance == NULL)
    return NPERR_INVALID_INSTANCE_ERROR;

  This = (PluginInstance *) instance->pdata;

  close (This->send_fd);

  //printf("NPP_DestroyStream\n");

  return NPERR_NO_ERROR;
}




void
NPP_URLNotify (NPP instance, const char *url, NPReason reason, void *notifyData)
{
    /***** Insert NPP_URLNotify code here *****\
    PluginInstance* This;
    if (instance != NULL)
        This = (PluginInstance*) instance->pdata;
    \*********************************************/
}


#endif

static void
packet_write (int fd, int code, int len, void *data)
{
  write (fd, &code, 4);
  write (fd, &len, 4);
  if (len > 0 && data) {
    write (fd, data, len);
  }
}




void DEBUG (const char *format, ...)
{
#if 0
  va_list varargs;
  char s[100];

  va_start (varargs, format);
  vsnprintf (s, 99, format, varargs);
  va_end (varargs);
  fprintf(stderr, "swfdec plugin: %s\n", s);
#endif
}





static char *
get_formats (void)
{
  static char *formats = NULL;
  int fds[4];
  int alloc_length;
  int length;
  int player_pid;
  int status = 0;
  int ret;

  if (formats) {
    return formats;
  }

  pipe (fds);
  pipe (fds + 2);

  /*
   * fds[0]  parent recv
   * fds[1]  child send
   * fds[2]  child recv
   * fds[3]  parent send
   */
  //recv_fd = fds[0];
  //send_fd = fds[3];

  player_pid = fork ();
  if (player_pid == 0) {
    char *argv[20];
    int argc = 0;

    /* child */
    dup2 (fds[2], 0);
    dup2 (fds[1], 1);

    argv[argc++] = "swfdec-mozilla-player";
    argv[argc++] = "--print-formats";
    argv[argc] = NULL;

    execv (BINDIR "swfdec-mozilla-player", argv);
    _exit (255);
  }
  close (fds[1]);
  close (fds[2]);

  alloc_length = 1024;
  formats = malloc (alloc_length);
  length = 0;
  while (1) {

    if (length == alloc_length - 1) {
      alloc_length += 1024;
      formats = realloc (formats, alloc_length);
    }

    ret = read (fds[0], formats + length, alloc_length - 1 - length);
    if (ret < 0) {
      goto error;
    }
    length += ret;

    if (ret == 0)
      break;
  }

  ret = waitpid (player_pid, &status, WNOHANG);

  if (ret == 0 || (WIFEXITED (status) && WEXITSTATUS (status) == 0)) {
    formats[length] = 0;

    close (fds[0]);
    close (fds[3]);

    return formats;
  }
error:
  close (fds[0]);
  close (fds[3]);
  free (formats);
  formats = NULL;

  return NULL;
}

