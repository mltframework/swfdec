/* Swfdec
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

#ifndef _SWFDEC_RESOURCE_REQUEST_H_
#define _SWFDEC_RESOURCE_REQUEST_H_

#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_security.h>

G_BEGIN_DECLS

typedef struct _SwfdecResourceRequest SwfdecResourceRequest;
typedef void (* SwfdecResourceFunc) (SwfdecPlayer *player, SwfdecLoader *loader, gpointer data);
typedef void (* SwfdecResourceAbortFunc) (SwfdecPlayer *player, gpointer data);
typedef void (* SwfdecResourceUnloadFunc) (SwfdecPlayer *player, const char *target, gpointer data);

typedef enum {
  SWFDEC_RESOURCE_REQUEST_LOAD,
  SWFDEC_RESOURCE_REQUEST_FSCOMMAND,
  SWFDEC_RESOURCE_REQUEST_UNLOAD
} SwfdecResourceRequestType;

/* FIXME: make this a union? */
struct _SwfdecResourceRequest
{
  SwfdecResourceRequestType	type;	/* type of request */

  GDestroyNotify	destroy;	/* function to call on player dispose */
  gpointer		data;		/* function to pass to the above functions */

  /* LOAD */
  SwfdecSecurity *	security;     	/* security context when loading or NULL for fscommand */
  char *		url;		/* URL we're gonna load */
  SwfdecLoaderRequest	request;	/* how are we goona load this URL? */
  SwfdecBuffer *	buffer;		/* data to pass to load request or NULL */
  SwfdecResourceFunc	func;		/* function to call when we got a loader (or an error) */

  /* FSCOMMAND */
  char *		command;	/* fscommand to execute */
  char *		value;		/* value to pass to fscommand */

  /* UNLOAD */
  char *	      	target;		/* the target to unload */
  SwfdecResourceUnloadFunc	unload;	/* function to call when we got a loader (or an error) */
};

/* public api for swfdec */

void		swfdec_player_request_resource		(SwfdecPlayer *		player,
							 SwfdecSecurity *	security,
							 const char *		url,
							 SwfdecLoaderRequest	req,
							 SwfdecBuffer *		buffer,
							 SwfdecResourceFunc	func,
							 gpointer		data,
							 GDestroyNotify		destroy);
void		swfdec_player_request_resource_now	(SwfdecPlayer *		player,
							 SwfdecSecurity *	security,
							 const char *		url,
							 SwfdecLoaderRequest	req,
							 SwfdecBuffer *		buffer,
							 SwfdecResourceFunc	callback,
							 SwfdecResourceAbortFunc	abort,
							 gpointer		user_data);
void		swfdec_player_request_unload		(SwfdecPlayer *		player,
							 const char *		target,
							 SwfdecResourceUnloadFunc func,
							 gpointer		data,
							 GDestroyNotify		destroy);
gboolean	swfdec_player_request_fscommand		(SwfdecPlayer *		player,
							 const char *		command,
							 const char *		value);

/* private api for swfdec_player.c */
void		swfdec_player_resource_request_init	(SwfdecPlayer *		player);
void		swfdec_player_resource_request_perform	(SwfdecPlayer *		player);
void		swfdec_player_resource_request_finish	(SwfdecPlayer *		player);

G_END_DECLS
#endif
