/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Mozilla Public
 * License Version 1.1 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of
 * the License at http://www.mozilla.org/MPL/
 * 
 * Software distributed under the License is distributed on an "AS
 * IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * rights and limitations under the License.
 * 
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation. All
 * Rights Reserved.
 *
 * Contributor(s): 
 * Stephen Mak <smak@sun.com>
 */

/*
 * npshell.c
 *
 * Netscape Client Plugin API
 * - Function that need to be implemented by plugin developers
 *
 * This file defines a "shell" plugin that plugin developers can use
 * as the basis for a real plugin.  This shell just provides empty
 * implementations of all functions that the plugin can implement
 * that will be called by Netscape (the NPP_xxx methods defined in 
 * npapi.h). 
 *
 * dp Suresh <dp@netscape.com>
 * updated 5/1998 <pollmann@netscape.com>
 * updated 9/2000 <smak@sun.com>
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "npapi.h"
//#include "strings.h"
//#include "plstr.h"

#include "config.h"

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

typedef struct {
	NPP instance;
	Display *display;
	Window window;
	int x;
	int y;
	int width, height;
	int recv_fd, send_fd;
	int player_pid;
}PluginInstance;


/***********************************************************************
 *
 * Implementations of plugin API functions
 *
 ***********************************************************************/

char*
NPP_GetMIMEDescription(void)
{
    return("application/x-shockwave-flash:swf:Shockwave Flash");
}

NPError
NPP_GetValue(NPP instance, NPPVariable variable, void *value)
{
    NPError err = NPERR_NO_ERROR;

    switch (variable) {
        case NPPVpluginNameString:
            *((char **)value) = "swfdec-0.1.0 Macromedia Flash decoder";
            break;
        case NPPVpluginDescriptionString:
            *((char **)value) =
"swfdec-0.1.0 - SWF (Macromedia Flash) movie decoding library\n"
"This is alpha software.  See <a href=\"https://cvs.comedi.org/\">"
"https://cvs.comedi.org/</a> for information.\n";
            break;
        default:
            err = NPERR_GENERIC_ERROR;
    }
    return err;
}

NPError
NPP_Initialize(void)
{
    return NPERR_NO_ERROR;
}

jref
NPP_GetJavaClass()
{
    return NULL;
}

void
NPP_Shutdown(void)
{
}

static void pfork(PluginInstance *this)
{
	int fds[4];

	pipe(fds);
	pipe(fds+2);

	this->recv_fd = fds[0];
	this->send_fd = fds[3];

	this->player_pid = fork();
	if(this->player_pid == 0){
		char xid_str[20];
		char width_str[20];
		char height_str[20];
		char *argv[20];
		int argc = 0;

		sprintf(xid_str,"%ld",this->window);

		/* child */
		dup2(fds[2],0);
		//dup2(fds[1],1);
		
		argv[argc++] = "swf_play";
		argv[argc++] = "--xid";
		argv[argc++] = xid_str;
#if 0
		if(this->width){
			sprintf(width_str,"%d",this->width);
			argv[argc++] = "--width";
			argv[argc++] = width_str;
		}
		if(this->height){
			sprintf(height_str,"%d",this->height);
			argv[argc++] = "--height";
			argv[argc++] = height_str;
		}
#endif
		argv[argc++] = "-";
		argv[argc] = NULL;

		execvp("swf_play",argv);
		execv(BINDIR "swf_play",argv);
		_exit(255);
	}

	close(fds[1]);
	close(fds[2]);
}

NPError 
NPP_New(NPMIMEType pluginType,
    NPP instance,
    uint16 mode,
    int16 argc,
    char* argn[],
    char* argv[],
    NPSavedData* saved)
{
	PluginInstance* This;
	int i;

	if (instance == NULL)
		return NPERR_INVALID_INSTANCE_ERROR;

	instance->pdata = NPN_MemAlloc(sizeof(PluginInstance));
	This = (PluginInstance*) instance->pdata;

	if (This == NULL) return NPERR_OUT_OF_MEMORY_ERROR;

	memset(This, 0, sizeof(PluginInstance));

	/* mode is NP_EMBED, NP_FULL, or NP_BACKGROUND (see npapi.h) */
	//printf("mode %d\n",mode);
	//printf("mime type: %s\n",pluginType);
	This->instance = instance;

	for(i=0;i<argc;i++){
		printf("argv[%d] %s %s\n",i,argn[i],argv[i]);
		if(strcmp(argn[i],"width")==0){
			This->width = strtol(argv[i],NULL,0);
		}
		if(strcmp(argn[i],"height")==0){
			This->height = strtol(argv[i],NULL,0);
		}
	}

	//pfork(This, 0x32);

	return NPERR_NO_ERROR;
}

NPError 
NPP_Destroy(NPP instance, NPSavedData** save)
{
    PluginInstance* This;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    This = (PluginInstance*) instance->pdata;

    if (This != NULL) {
	    kill(This->player_pid, SIGKILL);
	    waitpid(This->player_pid,NULL,0);

        NPN_MemFree(instance->pdata);
        instance->pdata = NULL;
    }

    printf("NPP_Destroy\n");

    return NPERR_NO_ERROR;
}


NPError 
NPP_SetWindow(NPP instance, NPWindow* window)
{
    PluginInstance* This;
    NPSetWindowCallbackStruct *ws_info;

    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    This = (PluginInstance*) instance->pdata;

    if (This == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    ws_info = (NPSetWindowCallbackStruct *)window->ws_info;

    if (This->window == (Window) window->window) {
        /* The page with the plugin is being resized.
           Save any UI information because the next time
           around expect a SetWindow with a new window
           id.
        */
        //fprintf(stderr, "Nullplugin: plugin received window resize.\n");
        //fprintf(stderr, "Window=(%i)\n", (int)window->window);
        if (window) {
           //fprintf(stderr, "W=(%i) H=(%i)\n", (int)window->width, (int)window->height);
        }
        return NPERR_NO_ERROR;
    } else {
	    if(This->window){
		    fprintf(stderr,"ack.  window changed!\n");
	    }

      This->window = (Window) window->window;
      This->display = ws_info->display;

      //printf("window %dx%d at %d %d\n", window->width, window->height, window->x, window->y);

#if 0
      This->x = window->x;
      This->y = window->y;
      This->width = window->width;
      This->height = window->height;
      This->visual = ws_info->visual;
      This->depth = ws_info->depth;
      This->colormap = ws_info->colormap;
      makePixmap(This);
      makeWidget(This);
#endif

	pfork(This);

    }
    return NPERR_NO_ERROR;
}


int16
NPP_HandleEvent(NPP instance, void* event)
{
	NPEvent *ev = event;

	printf("NPP_HandleEvent %d\n",ev->type);

	return 0;
}

NPError 
NPP_NewStream(NPP instance,
          NPMIMEType type,
          NPStream *stream, 
          NPBool seekable,
          uint16 *stype)
{
    if (instance == NULL)
        return NPERR_INVALID_INSTANCE_ERROR;

    printf("NPP_NewStream\n");

    return NPERR_NO_ERROR;
}

int32 
NPP_WriteReady(NPP instance, NPStream *stream)
{
    return 4096;
}


int32 NPP_Write(NPP instance, NPStream *stream, int32 offset, int32 len,
	void *buffer)
{
	PluginInstance* this;

	if(!instance)
		return 0;
	
	this = (PluginInstance*) instance->pdata;

	if(!this->player_pid)
		return 0;

	write(this->send_fd, buffer, len);
	return len;
}


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


void 
NPP_StreamAsFile(NPP instance, NPStream *stream, const char* fname)
{
    /***** Insert NPP_StreamAsFile code here *****\
    PluginInstance* This;
    if (instance != NULL)
        This = (PluginInstance*) instance->pdata;
    \*********************************************/
	printf("NPP_StreamAsFile\n");
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


void NPP_Print(NPP instance, NPPrint* printInfo)
{
}

