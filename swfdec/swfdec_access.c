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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <swfdec/swfdec_access.h>
#include <swfdec/swfdec_debug.h>

void
swfdec_player_allow_by_matrix (SwfdecPlayer *player, SwfdecSandbox *sandbox,
    const char *url_string, const SwfdecAccessMatrix matrix, 
    SwfdecPolicyFunc func, gpointer data)
{
  SwfdecAccessPermission perm;
  SwfdecAccessType type;
  SwfdecURL *url;

  g_return_if_fail (SWFDEC_IS_PLAYER (player));
  g_return_if_fail (SWFDEC_IS_SANDBOX (sandbox));
  g_return_if_fail (url_string != NULL);
  g_return_if_fail (func);

  url = swfdec_player_create_url (player, url_string);
  if (url == NULL) {
    func (player, FALSE, data);
    return;
  }

  if (swfdec_url_is_local (url)) {
    type = SWFDEC_ACCESS_LOCAL;
  } else if (swfdec_url_host_equal(url, sandbox->url)) {
    type = SWFDEC_ACCESS_SAME_HOST;
  } else {
    type = SWFDEC_ACCESS_NET;
  }

  perm = matrix[sandbox->type][type];

  if (perm == SWFDEC_ACCESS_YES) {
    func (player, TRUE, data);
  } else if (perm == SWFDEC_ACCESS_NO) {
    func (player, FALSE, data);
  } else {
    SwfdecURL *load_url = swfdec_url_new_components (
	swfdec_url_get_protocol (url), swfdec_url_get_host (url), 
	swfdec_url_get_port (url), "crossdomain.xml", NULL);
    swfdec_player_allow_or_load (player, sandbox->url, url, load_url, func, data);
    swfdec_url_free (load_url);
  }

  swfdec_url_free (url);
}

