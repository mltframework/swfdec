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

#ifndef _SWFDEC_VIDEO_H_
#define _SWFDEC_VIDEO_H_

#include <swfdec/swfdec_graphic.h>
#include <swfdec/swfdec_codec_video.h>

G_BEGIN_DECLS

typedef struct _SwfdecVideo SwfdecVideo;
typedef struct _SwfdecVideoClass SwfdecVideoClass;

#define SWFDEC_TYPE_VIDEO                    (swfdec_video_get_type())
#define SWFDEC_IS_VIDEO(obj)                 (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SWFDEC_TYPE_VIDEO))
#define SWFDEC_IS_VIDEO_CLASS(klass)         (G_TYPE_CHECK_CLASS_TYPE ((klass), SWFDEC_TYPE_VIDEO))
#define SWFDEC_VIDEO(obj)                    (G_TYPE_CHECK_INSTANCE_CAST ((obj), SWFDEC_TYPE_VIDEO, SwfdecVideo))
#define SWFDEC_VIDEO_CLASS(klass)            (G_TYPE_CHECK_CLASS_CAST ((klass), SWFDEC_TYPE_VIDEO, SwfdecVideoClass))

struct _SwfdecVideo {
  SwfdecGraphic			graphic;

  guint				width;		/* width in pixels */
  guint				height;		/* height in pixels */
  guint				n_frames;	/* length of movie */
  GArray *			images;		/* actual images of the movie */
  
  guint				format;		/* format in use */
};

struct _SwfdecVideoClass {
  SwfdecGraphicClass	graphic_class;
};

GType	swfdec_video_get_type		(void);

int	tag_func_define_video		(SwfdecSwfDecoder *	s,
					 guint			tag);
int	tag_func_video_frame    	(SwfdecSwfDecoder *	s,
					 guint			tag);


G_END_DECLS
#endif
