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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "vivi_code_emitter.h"

G_DEFINE_TYPE (ViviCodeEmitter, vivi_code_emitter, G_TYPE_OBJECT)

static void
vivi_code_emitter_dispose (GObject *object)
{
  ViviCodeEmitter *emit = VIVI_CODE_EMITTER (object);

  if (emit->bots) {
    SwfdecBuffer *buffer = swfdec_bots_close (emit->bots);
    swfdec_buffer_unref (buffer);
    emit->bots = NULL;
  }
  g_slist_foreach (emit->later, (GFunc) g_free, NULL);
  g_slist_free (emit->later);
  emit->later = NULL;
  g_hash_table_remove_all (emit->labels);
  emit->labels = NULL;

  G_OBJECT_CLASS (vivi_code_emitter_parent_class)->dispose (object);
}

static void
vivi_code_emitter_class_init (ViviCodeEmitterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = vivi_code_emitter_dispose;
}

static void
vivi_code_emitter_init (ViviCodeEmitter *emit)
{
  emit->bots = swfdec_bots_open ();
  emit->labels = g_hash_table_new_full (g_direct_hash, g_direct_equal,
      g_object_unref, g_free);
}

gboolean
vivi_code_emitter_emit_asm (ViviCodeEmitter *emitter, ViviCodeAsm *code, GError **error)
{
  ViviCodeAsmInterface *iface;

  g_return_val_if_fail (VIVI_IS_CODE_EMITTER (emitter), FALSE);
  g_return_val_if_fail (VIVI_IS_CODE_ASM (code), FALSE);

  iface = VIVI_CODE_ASM_GET_INTERFACE (code);
  return iface->emit (code, emitter, error);
}

void
vivi_code_emitter_add_label (ViviCodeEmitter *emitter, ViviCodeLabel *label)
{
  g_return_if_fail (VIVI_IS_CODE_EMITTER (emitter));
  g_return_if_fail (VIVI_IS_CODE_LABEL (label));

  g_assert (g_hash_table_lookup (emitter->labels, label) == NULL);
  g_hash_table_insert (emitter->labels, g_object_ref (label), 
      GSIZE_TO_POINTER (swfdec_bots_get_bytes (emitter->bots) + 1));
}

gssize
vivi_code_emitter_get_label_offset (ViviCodeEmitter *emitter, ViviCodeLabel *label)
{
  gsize size;

  g_return_val_if_fail (VIVI_IS_CODE_EMITTER (emitter), -1);
  g_return_val_if_fail (VIVI_IS_CODE_LABEL (label), -1);

  size = GPOINTER_TO_SIZE (g_hash_table_lookup (emitter->labels, label));
  return (gssize) size - 1;
}

typedef struct {
  ViviCodeEmitLater	func;
  gpointer		data;
} Later;

void
vivi_code_emitter_add_later (ViviCodeEmitter *emitter, ViviCodeEmitLater func, gpointer data)
{
  Later *later;

  g_return_if_fail (VIVI_IS_CODE_EMITTER (emitter));
  g_return_if_fail (VIVI_IS_CODE_EMITTER (emitter));

  later = g_new (Later, 1);
  later->func = func;
  later->data = data;
  emitter->later = g_slist_prepend (emitter->later, later);
}

SwfdecBuffer *
vivi_code_emitter_finish (ViviCodeEmitter *emitter, GError **error)
{
  SwfdecBuffer *buffer;
  GSList *walk;

  g_return_val_if_fail (VIVI_IS_CODE_EMITTER (emitter), NULL);

  buffer = swfdec_bots_close (emitter->bots);
  emitter->bots = NULL;
  for (walk = emitter->later; walk; walk = walk->next) {
    Later *later = walk->data;
    if (!later->func (emitter, buffer, later->data, error)) {
      swfdec_buffer_unref (buffer);
      buffer = NULL;
    }
  }
  g_slist_foreach (emitter->later, (GFunc) g_free, NULL);
  g_slist_free (emitter->later);
  emitter->later = NULL;
  g_hash_table_remove_all (emitter->labels);
  emitter->bots = swfdec_bots_open ();

  return buffer;
}

SwfdecBots *
vivi_code_emitter_get_bots (ViviCodeEmitter *emitter)
{
  g_return_val_if_fail (VIVI_IS_CODE_EMITTER (emitter), NULL);

  return emitter->bots;
}

