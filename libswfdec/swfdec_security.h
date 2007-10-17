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

#ifndef _SWFDEC_SECURITY_H_
#define _SWFDEC_SECURITY_H_

#include <glib-object.h>
#include <libswfdec/swfdec_url.h>

G_BEGIN_DECLS

typedef struct _SwfdecSecurity SwfdecSecurity;
typedef struct _SwfdecSecurityClass SwfdecSecurityClass;

#define SWFDEC_TYPE_SECURITY                    (swfdec_security_get_type())
#define SWFDEC_IS_SECURITY(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SECURITY))
#define SWFDEC_IS_SECURITY_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SECURITY))
#define SWFDEC_SECURITY(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SECURITY, SwfdecSecurity))
#define SWFDEC_SECURITY_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SECURITY, SwfdecSecurityClass))
#define SWFDEC_SECURITY_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_SECURITY, SwfdecSecurityClass))

struct _SwfdecSecurity
{
  GObject		object;
};

struct _SwfdecSecurityClass
{
  GObjectClass		object_class;

  SwfdecSecurity *	(* allow)			(SwfdecSecurity *	guard,
							 SwfdecSecurity *	from);
  gboolean		(* allow_url)			(SwfdecSecurity *	guard,
							 const SwfdecURL *	url);
};

GType			swfdec_security_get_type	(void);

SwfdecSecurity *	swfdec_security_new_default	(SwfdecSecurity *	sec);

SwfdecSecurity *	swfdec_security_allow		(SwfdecSecurity *	guard,
							 SwfdecSecurity *	key);
gboolean		swfdec_security_allow_url	(SwfdecSecurity *	guard,
							 const SwfdecURL *	url);


G_END_DECLS
#endif
