

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <config.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#define XP_UNIX 1
#define MOZ_X11 1
#include "npapi.h"
#include "npupp.h"

//#define DEBUG(x) printf(x "\n")
#define DEBUG(x)

typedef struct {
	NPP instance;
	Display *display;
	Window window;
	int x;
	int y;
	int width, height;
	int recv_fd, send_fd;
	int player_pid;
}Plugin;


static NPNetscapeFuncs mozilla_funcs;

static void plugin_fork(Plugin *plugin)
{
	int fds[4];

	pipe(fds);
	pipe(fds+2);

	plugin->recv_fd = fds[0];
	plugin->send_fd = fds[3];

	plugin->player_pid = fork();
	if(plugin->player_pid == 0){
		char xid_str[20];
		char width_str[20];
		char height_str[20];
		char *argv[20];
		int argc = 0;

		sprintf(xid_str,"%ld",plugin->window);

		/* child */
		dup2(fds[2],0);
		//dup2(fds[1],1);
		
		argv[argc++] = "swf_play";
		argv[argc++] = "--xid";
		argv[argc++] = xid_str;
		if(plugin->width){
			sprintf(width_str,"%d",plugin->width);
			argv[argc++] = "--width";
			argv[argc++] = width_str;
		}
		if(plugin->height){
			sprintf(height_str,"%d",plugin->height);
			argv[argc++] = "--height";
			argv[argc++] = height_str;
		}
		argv[argc++] = "-";
		argv[argc] = NULL;

		execvp("swf_play",argv);
		execv(BINDIR "swf_play",argv);
		_exit(255);
	}

	close(fds[1]);
	close(fds[2]);
}

static NPError plugin_newp(NPMIMEType mime_type, NPP instance,
	uint16_t mode, int16_t argc, char *argn[], char *argv[],
	NPSavedData *saved)
{
	Plugin * plugin;
	int i;

	DEBUG("plugin_newp");

	if (instance == NULL) return NPERR_INVALID_INSTANCE_ERROR;

	instance->pdata = mozilla_funcs.memalloc(sizeof(Plugin));
	plugin = (Plugin *) instance->pdata;

	if (plugin == NULL) return NPERR_OUT_OF_MEMORY_ERROR;
	memset(plugin, 0, sizeof(Plugin));

	/* mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h) */
	//printf("mode %d\n",mode);
	//printf("mime type: %s\n",pluginType);
	plugin->instance = instance;

	for(i=0;i<argc;i++){
		printf("argv[%d] %s %s\n",i,argn[i],argv[i]);
		if(strcmp(argn[i],"width")==0){
			plugin->width = strtol(argv[i],NULL,0);
		}
		if(strcmp(argn[i],"height")==0){
			plugin->height = strtol(argv[i],NULL,0);
		}
	}

	//plugin_fork(plugin, 0x32);

	return NPERR_NO_ERROR;
}

static NPError plugin_destroy(NPP instance, NPSavedData **save)
{
	Plugin * plugin;

	DEBUG("plugin_destroy");

	if (instance == NULL) return NPERR_INVALID_INSTANCE_ERROR;

	plugin = (Plugin *) instance->pdata;
	if (plugin == NULL) {
		return NPERR_NO_ERROR;
	}

	close(plugin->send_fd);
	close(plugin->recv_fd);

	kill(plugin->player_pid, SIGKILL);
	waitpid(plugin->player_pid,NULL,0);

	mozilla_funcs.memfree(instance->pdata);
	instance->pdata = NULL;

	return NPERR_NO_ERROR;
}

static NPError plugin_set_window(NPP instance, NPWindow* window)
{
	Plugin *plugin;

	DEBUG("plugin_set_window");

	if (instance == NULL) return NPERR_INVALID_INSTANCE_ERROR;

	plugin = (Plugin *) instance->pdata;
	if (plugin == NULL) return NPERR_INVALID_INSTANCE_ERROR;

	if (plugin->window){
		DEBUG("existing window");
		if(plugin->window == (Window) window->window) {
			DEBUG("resize");
			/* Resize event */
			/* Not currently handled */
		}else{
			DEBUG("change");
			printf("ack.  window changed!\n");
		}
	} else {
		NPSetWindowCallbackStruct *ws_info;

		DEBUG("about to fork");

		ws_info = window->ws_info;
		plugin->window = (Window) window->window;
		plugin->display = ws_info->display;

		plugin_fork(plugin);
	}

	DEBUG("leaving plugin_set_window");

	return NPERR_NO_ERROR;
}

static NPError plugin_new_stream(NPP instance, NPMIMEType type,
	const char *window, NPStream** stream_ptr)
{
	DEBUG("plugin_new_stream");

	return NPERR_NO_ERROR;
}

static NPError plugin_destroy_stream(NPP instance, NPStream* stream,
	NPError reason)
{
	DEBUG("plugin_destroy_stream");

	return NPERR_NO_ERROR;
}

static int32 plugin_write_ready(NPP instance, NPStream *stream)
{
	/* This is arbitrary */

	DEBUG("plugin_write_ready");

	return 4096;
}

static int32 plugin_write(NPP instance, NPStream *stream, int32 offset,
	int32 len, void *buffer)
{
	Plugin *plugin;

	DEBUG("plugin_write");

	if (instance == NULL) return 0;
	plugin = (Plugin *) instance->pdata;

	if (plugin == NULL) return 0;

	if(!plugin->player_pid) return 0;

	write(plugin->send_fd, buffer, len);

	return len;
}

static void plugin_stream_as_file(NPP instance, NPStream *stream,
	const char* fname)
{
	Plugin *plugin;

	DEBUG("plugin_stream_as_file");

	if (instance == NULL) return;
	plugin = (Plugin *) instance->pdata;

	if (plugin == NULL) return;

	printf("plugin_stream_as_file\n");
}

/* exported functions */

NPError NP_GetValue(void *future, NPPVariable variable, void *value)
{
    NPError err = NPERR_NO_ERROR;

    switch (variable) {
        case NPPVpluginNameString:
            *((char **)value) = "Shockwave Flash";
            break;
        case NPPVpluginDescriptionString:
            *((char **)value) =
"Shockwave Flash 6.0 animation viewer handled by Swfdec-" VERSION ".  "
"Plays SWF animations, commonly known as Macromedia&reg; Flash&reg;.<br><br>"
"This is alpha software.  It will probably behave in many situations, but "
"may also ride your motorcycle, drink all your milk, or use your computer "
"to browse porn.  Comments, feature requests, and patches are welcome.<br><br>"
"See <a href=\"http://swfdec.sourceforge.net/\">"
"http://swfdec.sourceforge.net/</a> for information.<br><br>"
"Flash, Shockwave, and Macromedia are trademarks of Macromedia, Inc.  Swfdec "
"is not affiliated with Macromedia, Inc.";
            break;
        default:
            err = NPERR_GENERIC_ERROR;
    }
    return err;
}

char *NP_GetMIMEDescription(void)
{
    return("application/x-shockwave-flash:swf:Shockwave Flash");
}

NPError NP_Initialize(NPNetscapeFuncs * moz_funcs, NPPluginFuncs *
	plugin_funcs)
{
	printf("NP_Initialize\n");

	if(moz_funcs == NULL || plugin_funcs == NULL)
		return NPERR_INVALID_FUNCTABLE_ERROR;

	if((moz_funcs->version >> 8) > NP_VERSION_MAJOR)
		return NPERR_INCOMPATIBLE_VERSION_ERROR;
	if(moz_funcs->size < sizeof(NPNetscapeFuncs))
		return NPERR_INVALID_FUNCTABLE_ERROR;
	if(plugin_funcs->size < sizeof(NPPluginFuncs))
		return NPERR_INVALID_FUNCTABLE_ERROR;

	memcpy(&mozilla_funcs, moz_funcs, sizeof(NPNetscapeFuncs));

	plugin_funcs->version = (NP_VERSION_MAJOR<<8) + NP_VERSION_MINOR;
	plugin_funcs->size = sizeof(NPPluginFuncs);
	plugin_funcs->newp = NewNPP_NewProc(plugin_newp);
	plugin_funcs->destroy = NewNPP_DestroyProc(plugin_destroy);
	plugin_funcs->setwindow = NewNPP_SetWindowProc(plugin_set_window);
	plugin_funcs->newstream = NewNPP_NewStreamProc(plugin_new_stream);
	plugin_funcs->destroystream = NewNPP_DestroyStreamProc(plugin_destroy_stream);
	plugin_funcs->writeready = NewNPP_WriteReadyProc(plugin_write_ready);
	plugin_funcs->asfile = NewNPP_StreamAsFileProc(plugin_stream_as_file);
	plugin_funcs->write = NewNPP_WriteProc(plugin_write);

	return NPERR_NO_ERROR;
}

NPError NP_Shutdown(void)
{
	return NPERR_NO_ERROR;
}

#if 0

NPError 
NPP_DestroyStream(NPP instance, NPStream *stream, NPError reason)
{
    PluginInstance* This;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    This = (PluginInstance*) instance->pdata;

    close(This->send_fd);

	printf("NPP_DestroyStream\n");

    return NPERR_NO_ERROR;
}




void NPP_URLNotify(NPP instance, const char* url, NPReason reason,
	void* notifyData)
{
    /***** Insert NPP_URLNotify code here *****\
    PluginInstance* This;
    if (instance != NULL)
        This = (PluginInstance*) instance->pdata;
    \*********************************************/
}


#endif

