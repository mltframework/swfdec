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

#ifndef _SWFDEC_SECURITY_ALLOW_H_
#define _SWFDEC_SECURITY_ALLOW_H_

#include <libswfdec/swfdec_security.h>

G_BEGIN_DECLS

typedef struct _SwfdecSecurityAllow SwfdecSecurityAllow;
typedef struct _SwfdecSecurityAllowClass SwfdecSecurityAllowClass;

#define SWFDEC_TYPE_SECURITY_ALLOW                    (swfdec_security_allow_get_type())
#define SWFDEC_IS_SECURITY_ALLOW(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_SECURITY_ALLOW))
#define SWFDEC_IS_SECURITY_ALLOW_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_SECURITY_ALLOW))
#define SWFDEC_SECURITY_ALLOW(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_SECURITY_ALLOW, SwfdecSecurityAllow))
#define SWFDEC_SECURITY_ALLOW_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_SECURITY_ALLOW, SwfdecSecurityAllowClass))
#define SWFDEC_SECURITY_ALLOW_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_SECURITY_ALLOW, SwfdecSecurityAllowClass))

struct _SwfdecSecurityAllow
{
  SwfdecSecurity	security;
};

struct _SwfdecSecurityAllowClass
{
  SwfdecSecurityClass 	security_class;
};

GType			swfdec_security_allow_get_type	(void);

SwfdecSecurity *	swfdec_security_allow_new	(void);


G_END_DECLS
#endif
