
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mad.h>

#include "swf.h"
#include "bits.h"
#include "tags.h"
#include "proto.h"


struct action_struct {
	int action;
	char * name;
	int (*func)(bits_t *bits);
};
static struct action_struct actions[] = {
	{ 0x04, "next frame",		NULL },
	{ 0x05, "prev frame",		NULL },
	{ 0x06, "play",			NULL },
	{ 0x06, "play",			NULL },
	{ 0x07, "stop",			NULL },
	{ 0x08, "toggle quality",	NULL },
	{ 0x08, "toggle quality",	NULL },
	{ 0x09, "stop sounds",		NULL },
	{ 0x0a, "add",			NULL },
	{ 0x0b, "subtract",		NULL },
	{ 0x0c, "multiply",		NULL },
	{ 0x0d, "divide",		NULL },
	{ 0x0e, "equal",		NULL },
	{ 0x0f, "less than",		NULL },
	{ 0x10, "and",			NULL },
	{ 0x11, "or",			NULL },
	{ 0x12, "not",			NULL },
	{ 0x13, "string equal",		NULL },
	{ 0x14, "string length",	NULL },
	{ 0x15, "substring",		NULL },
//	{ 0x17, "unknown",		NULL },
	{ 0x18, "int",			NULL },
	{ 0x1c, "eval",			NULL },
	{ 0x1d, "set variable",		NULL },
	{ 0x20, "set target expr",	NULL },
	{ 0x21, "string concat",	NULL },
	{ 0x22, "get property",		NULL },
	{ 0x23, "set property",		NULL },
	{ 0x24, "duplicate clip",	NULL },
	{ 0x25, "remove clip",		NULL },
	{ 0x26, "trace",		NULL },
	{ 0x27, "start drag movie",	NULL },
	{ 0x28, "stop drag movie",	NULL },
	{ 0x29, "string compare",	NULL },
	{ 0x30, "random",		NULL },
	{ 0x31, "mb length",		NULL },
	{ 0x32, "ord",			NULL },
	{ 0x33, "chr",			NULL },
	{ 0x34, "get timer",		NULL },
	{ 0x35, "mb substring",		NULL },
	{ 0x36, "mb ord",		NULL },
	{ 0x37, "mb chr",		NULL },
	{ 0x3b, "call function",	NULL },
	{ 0x3a, "delete",		NULL },
	{ 0x3c, "declare local var",	NULL },
//	{ 0x3d, "unknown",		NULL },
	{ 0x3e, "return from function",	NULL },
	{ 0x3f, "modulo",		NULL },
	{ 0x40, "instantiate",		NULL },
//	{ 0x41, "unknown",		NULL },
	{ 0x44, "typeof",		NULL },
	{ 0x46, "unknown",		NULL },
	{ 0x47, "add",			NULL },
	{ 0x48, "less than",		NULL },
	{ 0x49, "equals",		NULL },
	{ 0x4c, "make boolean",		NULL },
	{ 0x4e, "get member",		NULL },
	{ 0x4f, "set member",		NULL },
	{ 0x50, "increment",		NULL },
	{ 0x52, "call method",		NULL },
	{ 0x60, "bitwise and",		NULL },
	{ 0x61, "bitwise or",		NULL },
	{ 0x62, "bitwise xor",		NULL },
	{ 0x63, "shift left",		NULL },
	{ 0x64, "shift right",		NULL },
	{ 0x65, "rotate right",		NULL },
//	{ 0x67, "unknown",		NULL },
	{ 0x81, "goto frame",		NULL },
	{ 0x83, "get url",		NULL },
	{ 0x87, "get next dict member",	NULL },
	{ 0x88, "declare dictionary",	NULL },
	{ 0x8a, "wait for frame",	NULL },
//	{ 0x8b, "unknown",		NULL },
//	{ 0x8c, "unknown",		NULL },
	{ 0x8d, "wait for frame expr",	NULL },
	{ 0x94, "with",			NULL },
	{ 0x96, "push data",		NULL },
	{ 0x99, "branch",		NULL },
	{ 0x9a, "get url2",		NULL },
	{ 0x9b, "declare function",	NULL },
	{ 0x9d, "branch if",		NULL },
	{ 0x9e, "call frame",		NULL },
	{ 0x9f, "goto expr",		NULL },
//	{ 0xb7, "unknown",		NULL },
/* in 1394C148d01.swf */
	{ 0x8b, "unknown",		NULL },
	{ 0x8c, "unknown",		NULL },
};
static int n_actions = sizeof(actions)/sizeof(actions[0]);

void get_actions(swf_state_t *s,bits_t *bits)
{
	int action;
	int len;
	//int index;
	//int skip_count;
	int i;

	SWF_DEBUG(0,"get_actions\n");

	while(1){
		action = get_u8(bits);
		if(action==0)break;

		if(action&0x80){
			len = get_u16(bits);
		}else{
			len = 0;
		}

		for(i=0;i<n_actions;i++){
			if(actions[i].action == action){
				if(s->debug<1){
					printf("  [%02x] %s\n",action, actions[i].name);
				}
				break;
			}
		}
		if(i==n_actions){
			if(s->debug<3){
				printf("  [%02x] *** unknown action\n",action);
			}
		}
		bits->ptr += len;
	}
}

int tag_func_do_action(swf_state_t *s)
{
	get_actions(s,&s->b);

	return SWF_OK;
}

