/* Swfedit
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <gtk/gtk.h>
#include "swfedit_tag.h"

G_DEFINE_TYPE (SwfeditTag, swfedit_tag, SWFEDIT_TYPE_TOKEN)

static void
swfedit_tag_dispose (GObject *object)
{
  //SwfeditTag *tag = SWFEDIT_TAG (object);

  G_OBJECT_CLASS (swfedit_tag_parent_class)->dispose (object);
}

static void
swfedit_tag_class_init (SwfeditTagClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = swfedit_tag_dispose;
}

static void
swfedit_tag_init (SwfeditTag *tag)
{
}

SwfeditTag *
swfedit_tag_new (guint tag, SwfdecBuffer *buffer)
{
  SwfeditTag *item;

  item = g_object_new (SWFEDIT_TYPE_TAG, NULL);
  item->tag = tag;
  swfedit_token_add (SWFEDIT_TOKEN (item), "contents", SWFEDIT_TOKEN_BINARY, buffer);
  return item;
}

