/* Swfdec
 * Copyright (C) 2007-2008 Benjamin Otte <otte@gnome.org>
 *               2007 Pekka Lampila <pekka.lampila@iki.fi>
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

#ifndef _SWFDEC_FLASH_SECURITY_POLICY_H_
#define _SWFDEC_FLASH_SECURITY_POLICY_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_player.h>

G_BEGIN_DECLS

typedef struct _SwfdecPolicyFile SwfdecPolicyFile;
typedef struct _SwfdecPolicyFileClass SwfdecPolicyFileClass;

#define SWFDEC_TYPE_POLICY_FILE                    (swfdec_policy_file_get_type())
#define SWFDEC_IS_POLICY_FILE(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_POLICY_FILE))
#define SWFDEC_IS_POLICY_FILE_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_POLICY_FILE))
#define SWFDEC_POLICY_FILE(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_POLICY_FILE, SwfdecPolicyFile))
#define SWFDEC_POLICY_FILE_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_POLICY_FILE, SwfdecPolicyFileClass))
#define SWFDEC_POLICY_FILE_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_POLICY_FILE, SwfdecPolicyFileClass))

struct _SwfdecPolicyFile {
  GObject		object;

  SwfdecPlayer *	player;		/* player we're loaded from */
  SwfdecURL *		load_url;	/* url we're loaded from */
  SwfdecURL *		url;		/* parent url we check with */
  SwfdecStream *	stream;		/* stream we are loading or NULL if done loading */
  GSList *		allowed_hosts;	/* list of GPatternSpec of the allowed hosts */

  GSList *		requests;	/* requests waiting for this file to finish loading */
};

struct _SwfdecPolicyFileClass {
  GObjectClass		object_class;
};

GType		swfdec_policy_file_get_type	(void);

SwfdecPolicyFile *swfdec_policy_file_new	(SwfdecPlayer *		player,
						 const SwfdecURL *	url);
gboolean	swfdec_policy_file_is_loading	(SwfdecPolicyFile *	file);
gboolean	swfdec_policy_file_allow	(SwfdecPolicyFile *	file,
						 const SwfdecURL *	url);


G_END_DECLS
#endif
