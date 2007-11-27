/* Swfdec
 * Copyright (C) 2007 Benjamin Otte <otte@gnome.org>
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

#include <libswfdec/swfdec.h>
#include <libswfdec/swfdec_as_object.h>
#include <libswfdec/swfdec_flash_security.h>

G_BEGIN_DECLS

typedef struct _SwfdecPolicyLoader SwfdecPolicyLoader;
typedef struct _SwfdecPolicyLoaderClass SwfdecPolicyLoaderClass;
typedef void (* SwfdecPolicyLoaderFunc) (SwfdecPolicyLoader *policy_loader, gboolean allow);

#define SWFDEC_TYPE_POLICY_LOADER                    (swfdec_policy_loader_get_type())
#define SWFDEC_IS_POLICY_LOADER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_POLICY_LOADER))
#define SWFDEC_IS_POLICY_LOADER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_POLICY_LOADER))
#define SWFDEC_POLICY_LOADER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_POLICY_LOADER, SwfdecPolicyLoader))
#define SWFDEC_POLICY_LOADER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_POLICY_LOADER, SwfdecPolicyLoaderClass))
#define SWFDEC_POLICY_LOADER_GET_CLASS(obj)          (G_TYPE_INSTANCE_GET_CLASS ((obj), SWFDEC_TYPE_POLICY_LOADER, SwfdecPolicyLoaderClass))

struct _SwfdecPolicyLoader {
  GObject		object;

  SwfdecLoader *	loader;

  SwfdecFlashSecurity *	sec;
  char *		host;
  SwfdecPolicyLoaderFunc func;
};

struct _SwfdecPolicyLoaderClass {
  GObjectClass		object_class;
};

GType		swfdec_policy_loader_get_type	(void);

SwfdecPolicyLoader *swfdec_policy_loader_new	(SwfdecFlashSecurity *	sec,
						 const char *		host,
						 SwfdecPolicyLoaderFunc	func);
void		swfdec_policy_loader_free	(SwfdecPolicyLoader *	policy_loader);

G_END_DECLS
#endif
