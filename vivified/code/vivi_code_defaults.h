/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
 *                    Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef DEFAULT_BINARY
#define DEFAULT_BINARY(CapsName, underscore_name, operator_name, bytecode, precedence)
#endif

DEFAULT_BINARY (Add,		add,		"add",	SWFDEC_AS_ACTION_ADD,		VIVI_PRECEDENCE_ADD)
DEFAULT_BINARY (Subtract,	subtract,	"-",	SWFDEC_AS_ACTION_SUBTRACT,	VIVI_PRECEDENCE_ADD)
DEFAULT_BINARY (Multiply,	multiply,	"*",	SWFDEC_AS_ACTION_MULTIPLY,	VIVI_PRECEDENCE_MULTIPLY)
DEFAULT_BINARY (Divide,		divide,		"/",	SWFDEC_AS_ACTION_DIVIDE,	VIVI_PRECEDENCE_MULTIPLY)
DEFAULT_BINARY (Modulo,		modulo,		"%",	SWFDEC_AS_ACTION_MODULO,	VIVI_PRECEDENCE_MULTIPLY)
DEFAULT_BINARY (Equals,		equals,		"==",	SWFDEC_AS_ACTION_EQUALS,	VIVI_PRECEDENCE_EQUALITY)
DEFAULT_BINARY (Less,		less,		"<",	SWFDEC_AS_ACTION_LESS,		VIVI_PRECEDENCE_RELATIONAL)
DEFAULT_BINARY (LogicalAnd,	logical_and,	"and",	SWFDEC_AS_ACTION_AND,		VIVI_PRECEDENCE_AND)
DEFAULT_BINARY (LogicalOr,	logical_or,	"or",	SWFDEC_AS_ACTION_OR,		VIVI_PRECEDENCE_OR)
DEFAULT_BINARY (StringEquals,	string_equals,	"eq",	SWFDEC_AS_ACTION_STRING_EQUALS,	VIVI_PRECEDENCE_EQUALITY)
DEFAULT_BINARY (StringLess,	string_less,	"lt",	SWFDEC_AS_ACTION_STRING_LESS,	VIVI_PRECEDENCE_RELATIONAL)
DEFAULT_BINARY (Add2,		add2,	  	"+",	SWFDEC_AS_ACTION_ADD2,		VIVI_PRECEDENCE_ADD)
DEFAULT_BINARY (Less2,		less2,		"<",	SWFDEC_AS_ACTION_LESS2,		VIVI_PRECEDENCE_RELATIONAL)
DEFAULT_BINARY (Equals2,	equals2,	"==",	SWFDEC_AS_ACTION_EQUALS2,	VIVI_PRECEDENCE_EQUALITY)
DEFAULT_BINARY (BitwiseAnd,	bitwise_and,	"&",	SWFDEC_AS_ACTION_BIT_AND,	VIVI_PRECEDENCE_BINARY_AND)
DEFAULT_BINARY (BitwiseOr,	bitwise_or,	"|",	SWFDEC_AS_ACTION_BIT_OR,	VIVI_PRECEDENCE_BINARY_OR)
DEFAULT_BINARY (BitwiseXor,	bitwise_xor,	"^",	SWFDEC_AS_ACTION_BIT_XOR,	VIVI_PRECEDENCE_BINARY_XOR)
DEFAULT_BINARY (LeftShift,	left_shift,	"<<",	SWFDEC_AS_ACTION_BIT_LSHIFT,	VIVI_PRECEDENCE_SHIFT)
DEFAULT_BINARY (RightShift,	right_shift,	">>",	SWFDEC_AS_ACTION_BIT_RSHIFT,	VIVI_PRECEDENCE_SHIFT)
DEFAULT_BINARY (UnsignedRightShift,unsigned_right_shift,">>>",SWFDEC_AS_ACTION_BIT_URSHIFT,VIVI_PRECEDENCE_SHIFT)
DEFAULT_BINARY (StrictEquals,	strict_equals,	"===",	SWFDEC_AS_ACTION_STRICT_EQUALS,	VIVI_PRECEDENCE_EQUALITY)
DEFAULT_BINARY (Greater,	greater,	">",	SWFDEC_AS_ACTION_GREATER,	VIVI_PRECEDENCE_RELATIONAL)
DEFAULT_BINARY (StringGreater,	string_greater,	"gt",	SWFDEC_AS_ACTION_STRING_GREATER,VIVI_PRECEDENCE_RELATIONAL)
/* other special ones:
DEFAULT_BINARY (And,		and,		"&&",	???,				VIVI_PRECEDENCE_AND)
DEFAULT_BINARY (Or,		or,		"||",	???,				VIVI_PRECEDENCE_AND)
*/

#undef DEFAULT_BINARY

#ifndef DEFAULT_BUILTIN_STATEMENT
#define DEFAULT_BUILTIN_STATEMENT(CapsName, underscore_name, function_name, bytecode)
#endif

DEFAULT_BUILTIN_STATEMENT (NextFrame,		next_frame,	"nextFrame",		SWFDEC_AS_ACTION_NEXT_FRAME)
DEFAULT_BUILTIN_STATEMENT (Play,		play,		"play",			SWFDEC_AS_ACTION_PLAY)
DEFAULT_BUILTIN_STATEMENT (PreviousFrame,	previous_frame,	"prevFrame",		SWFDEC_AS_ACTION_PREVIOUS_FRAME)
DEFAULT_BUILTIN_STATEMENT (Stop,		stop,		"stop",			SWFDEC_AS_ACTION_STOP)
DEFAULT_BUILTIN_STATEMENT (StopDrag,		stop_drag,	"stopDrag",		SWFDEC_AS_ACTION_END_DRAG)
DEFAULT_BUILTIN_STATEMENT (StopSounds,		stop_sounds,	"stopSounds",		SWFDEC_AS_ACTION_STOP_SOUNDS)
DEFAULT_BUILTIN_STATEMENT (ToggleQuality,	toggle_quality,	"toggleQuality",	SWFDEC_AS_ACTION_TOGGLE_QUALITY)

#undef DEFAULT_BUILTIN_STATEMENT

#ifndef DEFAULT_BUILTIN_VALUE_STATEMENT
#define DEFAULT_BUILTIN_VALUE_STATEMENT(CapsName, underscore_name, function_name, bytecode)
#endif

//DEFAULT_BUILTIN_VALUE_STATEMENT (CallFrame,		call_frame,		"callFrame",		SWFDEC_AS_ACTION_CALL)
//DEFAULT_BUILTIN_VALUE_STATEMENT (GotoAndPlay,		goto_and_play,		"gotoAndPlay",		???)
//DEFAULT_BUILTIN_VALUE_STATEMENT (GotoAndStop,		goto_and_stop,		"gotoAndStop",		???)
DEFAULT_BUILTIN_VALUE_STATEMENT (RemoveMovieClip,	remove_movie_clip,	"removeMovieClip",	SWFDEC_AS_ACTION_REMOVE_SPRITE)
DEFAULT_BUILTIN_VALUE_STATEMENT (SetTarget,		set_target,		"setTarget",		SWFDEC_AS_ACTION_SET_TARGET2)
DEFAULT_BUILTIN_VALUE_STATEMENT (Trace,			trace,			"trace",		SWFDEC_AS_ACTION_TRACE)

#undef DEFAULT_BUILTIN_VALUE_STATEMENT

#ifndef DEFAULT_BUILTIN_VALUE_CALL
#define DEFAULT_BUILTIN_VALUE_CALL(CapsName, underscore_name, function_name, bytecode)
#endif

DEFAULT_BUILTIN_VALUE_CALL (Chr,	chr,		"chr",		SWFDEC_AS_ACTION_ASCII_TO_CHAR)
DEFAULT_BUILTIN_VALUE_CALL (Eval,	eval,		"eval",		SWFDEC_AS_ACTION_GET_VARIABLE)
DEFAULT_BUILTIN_VALUE_CALL (Int,	int,		"int",		SWFDEC_AS_ACTION_TO_INTEGER)
DEFAULT_BUILTIN_VALUE_CALL (Length,	length,		"length",	SWFDEC_AS_ACTION_STRING_LENGTH)
DEFAULT_BUILTIN_VALUE_CALL (Ord,	ord,		"ord",		SWFDEC_AS_ACTION_CHAR_TO_ASCII)
DEFAULT_BUILTIN_VALUE_CALL (Random,	random,		"random",	SWFDEC_AS_ACTION_RANDOM)
DEFAULT_BUILTIN_VALUE_CALL (TargetPath,	target_path,	"targetPath",	SWFDEC_AS_ACTION_TARGET_PATH)
DEFAULT_BUILTIN_VALUE_CALL (TypeOf,	type_of,	"typeOf",	SWFDEC_AS_ACTION_TYPE_OF)

#undef DEFAULT_BUILTIN_VALUE_CALL
