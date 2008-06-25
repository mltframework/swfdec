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

#ifndef _SWFDEC_VIDEO_VIDEO_PROVIDER_H_
#define _SWFDEC_VIDEO_VIDEO_PROVIDER_H_

#include <swfdec/swfdec_graphic.h>
#include <swfdec/swfdec_video.h>
#include <swfdec/swfdec_video_decoder.h>
#include <swfdec/swfdec_video_provider.h>

G_BEGIN_DECLS

typedef struct _SwfdecVideoVideoProvider SwfdecVideoVideoProvider;
typedef struct _SwfdecVideoVideoProviderClass SwfdecVideoVideoProviderClass;

#define SWFDEC_TYPE_VIDEO_VIDEO_PROVIDER                    (swfdec_video_video_provider_get_type())
#define SWFDEC_IS_VIDEO_VIDEO_PROVIDER(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_VIDEO_VIDEO_PROVIDER))
#define SWFDEC_IS_VIDEO_VIDEO_PROVIDER_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_VIDEO_VIDEO_PROVIDER))
#define SWFDEC_VIDEO_VIDEO_PROVIDER(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_VIDEO_VIDEO_PROVIDER, SwfdecVideoVideoProvider))
#define SWFDEC_VIDEO_VIDEO_PROVIDER_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_VIDEO_VIDEO_PROVIDER, SwfdecVideoVideoProviderClass))

struct _SwfdecVideoVideoProvider {
  GObject			object;

  SwfdecVideo *			video;		/* video we play back */
  guint				current_frame;	/* id of the frame we should have decoded */
  guint				decoder_frame;	/* id of the last frame the decoder has decoded */
  SwfdecVideoDecoder *		decoder;	/* the current decoder */
};

struct _SwfdecVideoVideoProviderClass {
  GObjectClass			object_class;
};

GType			swfdec_video_video_provider_get_type	(void);

SwfdecVideoProvider *	swfdec_video_video_provider_new		(SwfdecVideo *	video);


G_END_DECLS
#endif
