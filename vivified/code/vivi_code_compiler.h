/* Vivified
 * Copyright (C) 2008 Benjamin Otte <otte@gnome.org>
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

#ifndef _VIVI_CODE_COMPILER_H_
#define _VIVI_CODE_COMPILER_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_as_interpret.h>
#include <swfdec/swfdec_bots.h>

#include <vivified/code/vivi_code_token.h>
#include <vivified/code/vivi_code_value.h>

G_BEGIN_DECLS


typedef struct _ViviCodeCompilerClass ViviCodeCompilerClass;

#define VIVI_TYPE_CODE_COMPILER                    (vivi_code_compiler_get_type())
#define VIVI_IS_CODE_COMPILER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_COMPILER))
#define VIVI_IS_CODE_COMPILER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_COMPILER))
#define VIVI_CODE_COMPILER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_COMPILER, ViviCodeCompiler))
#define VIVI_CODE_COMPILER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_COMPILER, ViviCodeCompilerClass))
#define VIVI_CODE_COMPILER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_COMPILER, ViviCodeCompilerClass))

typedef struct {
  SwfdecAsAction		id;
  SwfdecBots *			data;
  GSList *			parent;
} ViviCodeCompilerAction;

struct _ViviCodeCompiler
{
  GObject			object;

  GSList *			actions;
  GSList *			current;
  ViviCodeCompilerAction *	action;
};

struct _ViviCodeCompilerClass
{
  GObjectClass		object_class;
};

GType		vivi_code_compiler_get_type		(void);

ViviCodeCompiler *vivi_code_compiler_new		(void);

SwfdecBuffer *	vivi_code_compiler_get_data		(ViviCodeCompiler *	compiler);

void		vivi_code_compiler_compile_token	(ViviCodeCompiler *	compiler,
							 ViviCodeToken *	token);
void		vivi_code_compiler_compile_value	(ViviCodeCompiler *	compiler,
							 ViviCodeValue *	value);

gsize		vivi_code_compiler_tail_size		(ViviCodeCompiler *	compiler);

void		vivi_code_compiler_begin_action		(ViviCodeCompiler *	compiler,
							 SwfdecAsAction		action);
void		vivi_code_compiler_end_action		(ViviCodeCompiler *	compiler);
void		vivi_code_compiler_write_empty_action	(ViviCodeCompiler *	compiler,
							 SwfdecAsAction		action);

void		vivi_code_compiler_write_u8		(ViviCodeCompiler *	compiler, guint value);
void		vivi_code_compiler_write_u16		(ViviCodeCompiler *	compiler, guint value);
void		vivi_code_compiler_write_s16		(ViviCodeCompiler *	compiler, guint value);
void		vivi_code_compiler_write_double		(ViviCodeCompiler *	compiler, double value);
void		vivi_code_compiler_write_string		(ViviCodeCompiler *	compiler, const char *value);


G_END_DECLS
#endif
