/* Swfdec
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

#ifndef _SWFDEC_ACCESS_H_
#define _SWFDEC_ACCESS_H_

#include <swfdec/swfdec.h>
#include <swfdec/swfdec_player_internal.h>
#include <swfdec/swfdec_sandbox.h>

G_BEGIN_DECLS

typedef enum {
  SWFDEC_ACCESS_LOCAL,
  SWFDEC_ACCESS_SAME_HOST,
  SWFDEC_ACCESS_NET
} SwfdecAccessType;

typedef enum {
  SWFDEC_ACCESS_NO,
  SWFDEC_ACCESS_POLICY,
  SWFDEC_ACCESS_YES
} SwfdecAccessPermission;

typedef SwfdecAccessPermission SwfdecAccessMatrix[5][3];

void	      	swfdec_player_allow_by_matrix	(SwfdecPlayer *		player,
						 const char *		url_string,
						 const SwfdecAccessMatrix matrix,
						 SwfdecPolicyFunc	func,
						 gpointer		data);


G_END_DECLS
#endif
