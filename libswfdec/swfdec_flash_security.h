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

#ifndef _SWFDEC_FLASH_SECURITY_H_
#define _SWFDEC_FLASH_SECURITY_H_

#include <libswfdec/swfdec_security.h>

G_BEGIN_DECLS

typedef struct _SwfdecFlashSecurity SwfdecFlashSecurity;
typedef struct _SwfdecFlashSecurityClass SwfdecFlashSecurityClass;

typedef enum {
  SWFDEC_SANDBOX_NONE,
  SWFDEC_SANDBOX_REMOTE,
  SWFDEC_SANDBOX_LOCAL_FILE,
  SWFDEC_SANDBOX_LOCAL_NETWORK,
  SWFDEC_SANDBOX_LOCAL_TRUSTED
} SwfdecSandboxType;

#define SWFDEC_TYPE_FLASH_SECURITY                    (swfdec_flash_security_get_type())
#define SWFDEC_IS_FLASH_SECURITY(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_FLASH_SECURITY))
#define SWFDEC_IS_FLASH_SECURITY_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_FLASH_SECURITY))
#define SWFDEC_FLASH_SECURITY(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_FLASH_SECURITY, SwfdecFlashSecurity))
#define SWFDEC_FLASH_SECURITY_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_FLASH_SECURITY, SwfdecFlashSecurityClass))
#define SWFDEC_FLASH_SECURITY_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_FLASH_SECURITY, SwfdecFlashSecurityClass))

struct _SwfdecFlashSecurity
{
  SwfdecSecurity	security;

  SwfdecSandboxType	sandbox;	/* sandbox we are operating in */
  SwfdecURL *		url;		/* url this security was loaded from */
};

struct _SwfdecFlashSecurityClass
{
  SwfdecSecurityClass 	security_class;
};

GType			swfdec_flash_security_get_type	(void);

void			swfdec_flash_security_set_url	(SwfdecFlashSecurity *	sec,
							 const SwfdecURL *	url);


G_END_DECLS
#endif
