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

#ifndef _VIVI_CODE_EMITTER_H_
#define _VIVI_CODE_EMITTER_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_bots.h>
#include <vivified/code/vivi_code_asm.h>
#include <vivified/code/vivi_code_label.h>

G_BEGIN_DECLS


typedef struct _ViviCodeEmitterClass ViviCodeEmitterClass;
typedef gboolean (* ViviCodeEmitLater) (ViviCodeEmitter *emitter, SwfdecBuffer *buffer, gpointer data, GError **error);

#define VIVI_TYPE_CODE_EMITTER                    (vivi_code_emitter_get_type())
#define VIVI_IS_CODE_EMITTER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), VIVI_TYPE_CODE_EMITTER))
#define VIVI_IS_CODE_EMITTER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), VIVI_TYPE_CODE_EMITTER))
#define VIVI_CODE_EMITTER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), VIVI_TYPE_CODE_EMITTER, ViviCodeEmitter))
#define VIVI_CODE_EMITTER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), VIVI_TYPE_CODE_EMITTER, ViviCodeEmitterClass))
#define VIVI_CODE_EMITTER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), VIVI_TYPE_CODE_EMITTER, ViviCodeEmitterClass))

struct _ViviCodeEmitter
{
  GObject		object;

  SwfdecBots *		bots;		/* output stream */
  GHashTable *		labels;		/* ViviCodeLabel => offset + 1 */
  GSList *		later;		/* ViviCodeEmitLater/data tuples */
};

struct _ViviCodeEmitterClass
{
  GObjectClass		object_class;
};

GType			vivi_code_emitter_get_type   	(void);

ViviCodeEmitter *	vivi_code_emitter_new		(void);

gboolean		vivi_code_emitter_emit_asm	(ViviCodeEmitter *	emitter,
							 ViviCodeAsm *		code,
							 GError **		error);

SwfdecBots *		vivi_code_emitter_get_bots	(ViviCodeEmitter *	emitter);
void			vivi_code_emitter_add_label	(ViviCodeEmitter *	emitter,
							 ViviCodeLabel *	label);
gssize			vivi_code_emitter_get_label_offset 
							(ViviCodeEmitter *	emitter,
							 ViviCodeLabel *	label);
void			vivi_code_emitter_add_later	(ViviCodeEmitter *	emitter,
							 ViviCodeEmitLater	func,
							 gpointer		data);
SwfdecBuffer *		vivi_code_emitter_finish	(ViviCodeEmitter *	emitter,
							 GError **		error);


G_END_DECLS
#endif
