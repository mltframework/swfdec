/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GdkPixbuf library - SWF loader declarations
 *
 * Copyright (C) 2003 Dom Lachowicz
 *
 * Authors: Dom Lachowicz <cinamod@hotmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef GDK_PIXBUF_SWF_H
#define GDK_PIXBUF_SWF_H

#include <gdk-pixbuf/gdk-pixbuf-animation.h>

typedef struct _GdkPixbufSwfAnim GdkPixbufSwfAnim;
typedef struct _GdkPixbufSwfAnimClass GdkPixbufSwfAnimClass;
typedef struct _GdkPixbufFrame GdkPixbufFrame;

#define GDK_TYPE_PIXBUF_SWF_ANIM              (gdk_pixbuf_swf_anim_get_type ())
#define GDK_PIXBUF_SWF_ANIM(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_PIXBUF_SWF_ANIM, GdkPixbufSwfAnim))
#define GDK_IS_PIXBUF_SWF_ANIM(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_PIXBUF_SWF_ANIM))

#define GDK_PIXBUF_SWF_ANIM_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_PIXBUF_SWF_ANIM, GdkPixbufSwfAnimClass))
#define GDK_IS_PIXBUF_SWF_ANIM_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_PIXBUF_SWF_ANIM))
#define GDK_PIXBUF_SWF_ANIM_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_PIXBUF_SWF_ANIM, GdkPixbufSwfAnimClass))

/* Private part of the GdkPixbufSwfAnim structure */
struct _GdkPixbufSwfAnim {
        GdkPixbufAnimation parent_instance;

        /* Number of frames */
        int n_frames;

        /* Total length of animation */
        int rate;
        int total_time;
        
	/* List of GdkPixbufFrame structures */
        GList *frames;

	/* bounding box size */
	int width, height;
};

struct _GdkPixbufSwfAnimClass {
        GdkPixbufAnimationClass parent_class;      
};

GType gdk_pixbuf_swf_anim_get_type (void) G_GNUC_CONST;

typedef struct _GdkPixbufSwfAnimIter GdkPixbufSwfAnimIter;
typedef struct _GdkPixbufSwfAnimIterClass GdkPixbufSwfAnimIterClass;

#define GDK_TYPE_PIXBUF_SWF_ANIM_ITER              (gdk_pixbuf_swf_anim_iter_get_type ())
#define GDK_PIXBUF_SWF_ANIM_ITER(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), GDK_TYPE_PIXBUF_SWF_ANIM_ITER, GdkPixbufSwfAnimIter))
#define GDK_IS_PIXBUF_SWF_ANIM_ITER(object)        (G_TYPE_CHECK_INSTANCE_TYPE ((object), GDK_TYPE_PIXBUF_SWF_ANIM_ITER))

#define GDK_PIXBUF_SWF_ANIM_ITER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), GDK_TYPE_PIXBUF_SWF_ANIM_ITER, GdkPixbufSwfAnimIterClass))
#define GDK_IS_PIXBUF_SWF_ANIM_ITER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GDK_TYPE_PIXBUF_SWF_ANIM_ITER))
#define GDK_PIXBUF_SWF_ANIM_ITER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), GDK_TYPE_PIXBUF_SWF_ANIM_ITER, GdkPixbufSwfAnimIterClass))

struct _GdkPixbufSwfAnimIter {
        GdkPixbufAnimationIter parent_instance;
        
        GdkPixbufSwfAnim   *swf_anim;

        GTimeVal            start_time;
        GTimeVal            current_time;

        /* Time in milliseconds into this run of the animation */
        gint                position;
        
        GList              *current_frame;
};

struct _GdkPixbufSwfAnimIterClass {
        GdkPixbufAnimationIterClass parent_class;
};

GType gdk_pixbuf_swf_anim_iter_get_type (void) G_GNUC_CONST;

struct _GdkPixbufFrame {
	/* The pixbuf with this frame's image data */
	GdkPixbuf *pixbuf;

	/* Frame duration in ms */
	int delay_time;

        /* Sum of preceding delay times */
        int elapsed;        
};

#endif
