/* SwfdecAs
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#ifndef _SWFDEC_AS_TYPES_H_
#define _SWFDEC_AS_TYPES_H_

#include <glib.h>

G_BEGIN_DECLS

/* fundamental types */
#define SWFDEC_TYPE_AS_UNDEFINED (0)
#define SWFDEC_TYPE_AS_BOOLEAN	(1)
/* #define SWFDEC_AS_INT	(2)  not defined yet, might be useful as an optimization */
#define SWFDEC_TYPE_AS_NUMBER	(3)
#define SWFDEC_TYPE_AS_STRING	(4)
#define SWFDEC_TYPE_AS_NULL	(5)
#define SWFDEC_TYPE_AS_ASOBJECT	(6)

typedef guint8 SwfdecAsType;
typedef struct _SwfdecAsArray SwfdecAsArray;
typedef struct _SwfdecAsContext SwfdecAsContext;
typedef struct _SwfdecAsFrame SwfdecAsFrame;
typedef struct _SwfdecAsObject SwfdecAsObject;
typedef struct _SwfdecAsStack SwfdecAsStack;
typedef struct _SwfdecAsValue SwfdecAsValue;

/* IMPORTANT: a SwfdecAsValue memset to 0 is a valid undefined value */
struct _SwfdecAsValue {
  SwfdecAsType		type;
  union {
    gboolean		boolean;
    double		number;
    const char *	string;
    SwfdecAsObject *	object;
  } value;
};

#define SWFDEC_IS_AS_VALUE(val) ((val)->type <= SWFDEC_TYPE_AS_OBJECT)

#define SWFDEC_AS_VALUE_IS_UNDEFINED(val) ((val)->type == SWFDEC_TYPE_AS_UNDEFINED)
#define SWFDEC_AS_VALUE_SET_UNDEFINED(val) (val)->type = SWFDEC_TYPE_AS_UNDEFINED

#define SWFDEC_AS_VALUE_IS_BOOLEAN(val) ((val)->type == SWFDEC_TYPE_AS_BOOLEAN)
#define SWFDEC_AS_VALUE_GET_BOOLEAN(val) (g_assert ((val)->type == SWFDEC_TYPE_AS_BOOLEAN), (val)->value.boolean)
#define SWFDEC_AS_VALUE_SET_BOOLEAN(val,b) G_STMT_START { \
  (val)->type = SWFDEC_TYPE_AS_BOOLEAN; \
  (val)->value.boolean = b; \
} G_STMT_END

#define SWFDEC_AS_VALUE_IS_NUMBER(val) ((val)->type == SWFDEC_TYPE_AS_NUMBER)
#define SWFDEC_AS_VALUE_GET_NUMBER(val) (g_assert ((val)->type == SWFDEC_TYPE_AS_NUMBER), (val)->value.number)
#define SWFDEC_AS_VALUE_SET_NUMBER(val,d) G_STMT_START { \
  (val)->type = SWFDEC_TYPE_AS_NUMBER; \
  (val)->value.number = d; \
} G_STMT_END

#define SWFDEC_AS_VALUE_IS_STRING(val) ((val)->type == SWFDEC_TYPE_AS_STRING)
#define SWFDEC_AS_VALUE_GET_STRING(val) (g_assert ((val)->type == SWFDEC_TYPE_AS_STRING), (val)->value.string)
#define SWFDEC_AS_VALUE_SET_STRING(val,s) G_STMT_START { \
  (val)->type = SWFDEC_TYPE_AS_STRING; \
  (val)->value.string = s; \
} G_STMT_END

#define SWFDEC_AS_VALUE_IS_NULL(val) ((val)->type == SWFDEC_TYPE_AS_NULL)
#define SWFDEC_AS_VALUE_SET_NULL(val) (val)->type = SWFDEC_TYPE_AS_NULL

#define SWFDEC_AS_VALUE_IS_OBJECT(val) ((val)->type == SWFDEC_TYPE_AS_ASOBJECT)
#define SWFDEC_AS_VALUE_GET_OBJECT(val) (g_assert ((val)->type == SWFDEC_TYPE_AS_ASOBJECT), (val)->value.object)
#define SWFDEC_AS_VALUE_SET_OBJECT(val,o) G_STMT_START { \
  (val)->type = SWFDEC_TYPE_AS_ASOBJECT; \
  (val)->value.object = o; \
} G_STMT_END


/* List of static strings that are required all the time */
extern const char *swfdec_as_strings[];
#define SWFDEC_AS_STR_EMPTY (swfdec_as_strings[0] + 1)
#define SWFDEC_AS_STR_PROTO (swfdec_as_strings[1] + 1)
#define SWFDEC_AS_STR_THIS (swfdec_as_strings[2] + 1)
#define SWFDEC_AS_STR_CODE (swfdec_as_strings[3] + 1)
#define SWFDEC_AS_STR_LEVEL (swfdec_as_strings[4] + 1)
#define SWFDEC_AS_STR_DESCRIPTION (swfdec_as_strings[5] + 1)
#define SWFDEC_AS_STR_STATUS (swfdec_as_strings[6] + 1)
#define SWFDEC_AS_STR_SUCCESS (swfdec_as_strings[7] + 1)
#define SWFDEC_AS_STR_NET_CONNECTION_CONNECT_SUCCESS (swfdec_as_strings[8] + 1)
#define SWFDEC_AS_STR_ON_LOAD (swfdec_as_strings[9] + 1)
#define SWFDEC_AS_STR_ON_ENTER_FRAME (swfdec_as_strings[10] + 1)
#define SWFDEC_AS_STR_ON_UNLOAD (swfdec_as_strings[11] + 1)
#define SWFDEC_AS_STR_ON_MOUSE_MOVE (swfdec_as_strings[12] + 1)
#define SWFDEC_AS_STR_ON_MOUSE_DOWN (swfdec_as_strings[13] + 1)
#define SWFDEC_AS_STR_ON_MOUSE_UP (swfdec_as_strings[14] + 1)
#define SWFDEC_AS_STR_ON_KEY_UP (swfdec_as_strings[15] + 1)
#define SWFDEC_AS_STR_ON_KEY_DOWN (swfdec_as_strings[16] + 1)
#define SWFDEC_AS_STR_ON_DATA (swfdec_as_strings[17] + 1)
#define SWFDEC_AS_STR_ON_PRESS (swfdec_as_strings[18] + 1)
#define SWFDEC_AS_STR_ON_RELEASE (swfdec_as_strings[19] + 1)
#define SWFDEC_AS_STR_ON_RELEASE_OUTSIDE (swfdec_as_strings[20] + 1)
#define SWFDEC_AS_STR_ON_ROLL_OVER (swfdec_as_strings[21] + 1)
#define SWFDEC_AS_STR_ON_ROLL_OUT (swfdec_as_strings[22] + 1)
#define SWFDEC_AS_STR_ON_DRAG_OVER (swfdec_as_strings[23] + 1)
#define SWFDEC_AS_STR_ON_DRAG_OUT (swfdec_as_strings[24] + 1)
#define SWFDEC_AS_STR_ON_CONSTRUCT (swfdec_as_strings[25] + 1)
#define SWFDEC_AS_STR_ON_STATUS (swfdec_as_strings[26] + 1)
#define SWFDEC_AS_STR_ERROR (swfdec_as_strings[27] + 1)
#define SWFDEC_AS_STR_NETSTREAM_BUFFER_EMPTY (swfdec_as_strings[28] + 1)
#define SWFDEC_AS_STR_NETSTREAM_BUFFER_FULL (swfdec_as_strings[29] + 1)
#define SWFDEC_AS_STR_NETSTREAM_BUFFER_FLUSH (swfdec_as_strings[30] + 1)
#define SWFDEC_AS_STR_NETSTREAM_PLAY_START (swfdec_as_strings[31] + 1)
#define SWFDEC_AS_STR_NETSTREAM_PLAY_STOP (swfdec_as_strings[32] + 1)
#define SWFDEC_AS_STR_NETSTREAM_PLAY_STREAMNOTFOUND (swfdec_as_strings[33] + 1)

/* all existing actions */
typedef enum {
  SWFDEC_AS_ACTION_NEXT_FRAME = 0x04,
  SWFDEC_AS_ACTION_PREVIOUS_FRAME = 0x05,
  SWFDEC_AS_ACTION_PLAY = 0x06,
  SWFDEC_AS_ACTION_STOP = 0x07,
  SWFDEC_AS_ACTION_TOGGLE_QUALITY = 0x08,
  SWFDEC_AS_ACTION_STOP_SOUNDS = 0x09,
  SWFDEC_AS_ACTION_ADD = 0x0A,
  SWFDEC_AS_ACTION_SUBTRACT = 0x0B,
  SWFDEC_AS_ACTION_MULTIPLY = 0x0C,
  SWFDEC_AS_ACTION_DIVIDE = 0x0D,
  SWFDEC_AS_ACTION_EQUALS = 0x0E,
  SWFDEC_AS_ACTION_LESS = 0x0F,
  SWFDEC_AS_ACTION_AND = 0x10,
  SWFDEC_AS_ACTION_OR = 0x11,
  SWFDEC_AS_ACTION_NOT = 0x12,
  SWFDEC_AS_ACTION_STRING_EQUALS = 0x13,
  SWFDEC_AS_ACTION_STRING_LENGTH = 0x14,
  SWFDEC_AS_ACTION_STRING_EXTRACT = 0x15,
  SWFDEC_AS_ACTION_POP = 0x17,
  SWFDEC_AS_ACTION_TO_INTEGER = 0x18,
  SWFDEC_AS_ACTION_GET_VARIABLE = 0x1C,
  SWFDEC_AS_ACTION_SET_VARIABLE = 0x1D,
  SWFDEC_AS_ACTION_SET_TARGET2 = 0x20,
  SWFDEC_AS_ACTION_STRING_ADD = 0x21,
  SWFDEC_AS_ACTION_GET_PROPERTY = 0x22,
  SWFDEC_AS_ACTION_SET_PROPERTY = 0x23,
  SWFDEC_AS_ACTION_CLONE_SPRITE = 0x24,
  SWFDEC_AS_ACTION_REMOVE_SPRITE = 0x25,
  SWFDEC_AS_ACTION_TRACE = 0x26,
  SWFDEC_AS_ACTION_START_DRAG = 0x27,
  SWFDEC_AS_ACTION_END_DRAG = 0x28,
  SWFDEC_AS_ACTION_STRING_LESS = 0x29,
  SWFDEC_AS_ACTION_THROW = 0x2A,
  SWFDEC_AS_ACTION_CAST = 0x2B,
  SWFDEC_AS_ACTION_IMPLEMENTS = 0x2C,
  SWFDEC_AS_ACTION_RANDOM = 0x30,
  SWFDEC_AS_ACTION_MB_STRING_LENGTH = 0x31,
  SWFDEC_AS_ACTION_CHAR_TO_ASCII = 0x32,
  SWFDEC_AS_ACTION_ASCII_TO_CHAR = 0x33,
  SWFDEC_AS_ACTION_DELETE = 0x3A,
  SWFDEC_AS_ACTION_DELETE2 = 0x3B,
  SWFDEC_AS_ACTION_DEFINE_LOCAL = 0x3C,
  SWFDEC_AS_ACTION_CALL_FUNCTION = 0x3D,
  SWFDEC_AS_ACTION_RETURN = 0x3E,
  SWFDEC_AS_ACTION_MODULO = 0x3F,
  SWFDEC_AS_ACTION_NEW_OBJECT = 0x40,
  SWFDEC_AS_ACTION_DEFINE_LOCAL2 = 0x41,
  SWFDEC_AS_ACTION_INIT_ARRAY = 0x42,
  SWFDEC_AS_ACTION_INIT_OBJECT = 0x43,
  SWFDEC_AS_ACTION_TYPE_OF = 0x44,
  SWFDEC_AS_ACTION_TARGET_PATH = 0x45,
  SWFDEC_AS_ACTION_ENUMERATE = 0x46,
  SWFDEC_AS_ACTION_ADD2 = 0x47,
  SWFDEC_AS_ACTION_LESS2 = 0x48,
  SWFDEC_AS_ACTION_EQUALS2 = 0x49,
  SWFDEC_AS_ACTION_TO_NUMBER = 0x4A,
  SWFDEC_AS_ACTION_TO_STRING = 0x4B,
  SWFDEC_AS_ACTION_PUSH_DUPLICATE = 0x4C,
  SWFDEC_AS_ACTION_SWAP = 0x4D,
  SWFDEC_AS_ACTION_GET_MEMBER = 0x4E,
  SWFDEC_AS_ACTION_SET_MEMBER = 0x4F,
  SWFDEC_AS_ACTION_INCREMENT = 0x50,
  SWFDEC_AS_ACTION_DECREMENT = 0x51,
  SWFDEC_AS_ACTION_CALL_METHOD = 0x52,
  SWFDEC_AS_ACTION_NEW_METHOD = 0x53,
  SWFDEC_AS_ACTION_INSTANCE_OF = 0x54,
  SWFDEC_AS_ACTION_ENUMERATE2 = 0x55,
  SWFDEC_AS_ACTION_BIT_AND = 0x60,
  SWFDEC_AS_ACTION_BIT_OR = 0x61,
  SWFDEC_AS_ACTION_BIT_XOR = 0x62,
  SWFDEC_AS_ACTION_BIT_LSHIFT = 0x63,
  SWFDEC_AS_ACTION_BIT_RSHIFT = 0x64,
  SWFDEC_AS_ACTION_BIT_URSHIFT = 0x65,
  SWFDEC_AS_ACTION_STRICT_EQUALS = 0x66,
  SWFDEC_AS_ACTION_GREATER = 0x67,
  SWFDEC_AS_ACTION_STRING_FREATER = 0x68,
  SWFDEC_AS_ACTION_EXTENDS = 0x69,
  SWFDEC_AS_ACTION_GOTO_FRAME = 0x81,
  SWFDEC_AS_ACTION_GET_URL = 0x83,
  SWFDEC_AS_ACTION_STORE_REGISTER = 0x87,
  SWFDEC_AS_ACTION_CONSTANT_POOL = 0x88,
  SWFDEC_AS_ACTION_WAIT_FOR_FRAME = 0x8A,
  SWFDEC_AS_ACTION_SET_TARGET = 0x8B,
  SWFDEC_AS_ACTION_GOTO_LABEL = 0x8C,
  SWFDEC_AS_ACTION_WAIT_FOR_FRAME2 = 0x8D,
  SWFDEC_AS_ACTION_DEFINE_FUNCTION2 = 0x8E,
  SWFDEC_AS_ACTION_TRY = 0x8F,
  SWFDEC_AS_ACTION_WITH = 0x94,
  SWFDEC_AS_ACTION_PUSH = 0x96,
  SWFDEC_AS_ACTION_JUMP = 0x99,
  SWFDEC_AS_ACTION_GET_URL2 = 0x9A,
  SWFDEC_AS_ACTION_DEFINE_FUNCTION = 0x9B,
  SWFDEC_AS_ACTION_IF = 0x9D,
  SWFDEC_AS_ACTION_CALL = 0x9E,
  SWFDEC_AS_ACTION_GOTO_FRAME2 = 0x9F
} SwfdecAsAction;

const char *	swfdec_as_value_to_string	(SwfdecAsContext *	context,
						 const SwfdecAsValue *	value);


G_END_DECLS
#endif
