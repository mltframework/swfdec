/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GdkPixbuf library - animated gif support
 *
 * Copyright (C) Dom Lachowicz
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
 *
 * Based on code originally by:
 *          Jonathan Blandford <jrb@redhat.com>
 *          Havoc Pennington <hp@redhat.com>
 */

#include <errno.h>
#include "io-swf-anim.h"

static void gdk_pixbuf_swf_anim_class_init (GdkPixbufSwfAnimClass *klass);
static void gdk_pixbuf_swf_anim_finalize   (GObject        *object);

static gboolean                gdk_pixbuf_swf_anim_is_static_image  (GdkPixbufAnimation *animation);
static GdkPixbuf*              gdk_pixbuf_swf_anim_get_static_image (GdkPixbufAnimation *animation);

static void                    gdk_pixbuf_swf_anim_get_size (GdkPixbufAnimation *anim,
                                                             int                *width,
                                                             int                *height);
static GdkPixbufAnimationIter* gdk_pixbuf_swf_anim_get_iter (GdkPixbufAnimation *anim,
                                                             const GTimeVal     *start_time);


static gpointer parent_class;

GType
gdk_pixbuf_swf_anim_get_type (void)
{
        static GType object_type = 0;

        if (!object_type) {
                static const GTypeInfo object_info = {
                        sizeof (GdkPixbufSwfAnimClass),
                        (GBaseInitFunc) NULL,
                        (GBaseFinalizeFunc) NULL,
                        (GClassInitFunc) gdk_pixbuf_swf_anim_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof (GdkPixbufSwfAnim),
                        0,              /* n_preallocs */
                        (GInstanceInitFunc) NULL,
                };
                
                object_type = g_type_register_static (GDK_TYPE_PIXBUF_ANIMATION,
                                                      "GdkPixbufSwfAnim",
                                                      &object_info, 0);
        }
        
        return object_type;
}

static void
gdk_pixbuf_swf_anim_class_init (GdkPixbufSwfAnimClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GdkPixbufAnimationClass *anim_class = GDK_PIXBUF_ANIMATION_CLASS (klass);
        
        parent_class = g_type_class_peek_parent (klass);
        
        object_class->finalize = gdk_pixbuf_swf_anim_finalize;

        anim_class->is_static_image = gdk_pixbuf_swf_anim_is_static_image;
        anim_class->get_static_image = gdk_pixbuf_swf_anim_get_static_image;
        anim_class->get_size = gdk_pixbuf_swf_anim_get_size;
        anim_class->get_iter = gdk_pixbuf_swf_anim_get_iter;
}

static void
gdk_pixbuf_swf_anim_finalize (GObject *object)
{
        GdkPixbufSwfAnim *swf_anim = GDK_PIXBUF_SWF_ANIM (object);

        GList *l;
        GdkPixbufFrame *frame;
        
        for (l = swf_anim->frames; l; l = l->next) {
                frame = l->data;
                g_object_unref (frame->pixbuf);
                g_free (frame);
        }
        
        g_list_free (swf_anim->frames);
        
        G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gdk_pixbuf_swf_anim_is_static_image  (GdkPixbufAnimation *animation)
{
        GdkPixbufSwfAnim *swf_anim;

        swf_anim = GDK_PIXBUF_SWF_ANIM (animation);

        return (swf_anim->frames != NULL &&
                swf_anim->frames->next == NULL);
}

static GdkPixbuf*
gdk_pixbuf_swf_anim_get_static_image (GdkPixbufAnimation *animation)
{
        GdkPixbufSwfAnim *swf_anim;

        swf_anim = GDK_PIXBUF_SWF_ANIM (animation);

        if (swf_anim->frames == NULL)
                return NULL;
        else
                return GDK_PIXBUF (((GdkPixbufFrame*)swf_anim->frames->data)->pixbuf);        
}

static void
gdk_pixbuf_swf_anim_get_size (GdkPixbufAnimation *anim,
                              int                *width,
                              int                *height)
{
        GdkPixbufSwfAnim *swf_anim;

        swf_anim = GDK_PIXBUF_SWF_ANIM (anim);

        if (width)
                *width = swf_anim->width;

        if (height)
                *height = swf_anim->height;
}


static void
iter_clear (GdkPixbufSwfAnimIter *iter)
{
        iter->current_frame = NULL;
}

static void
iter_restart (GdkPixbufSwfAnimIter *iter)
{
        iter_clear (iter);
  
        iter->current_frame = iter->swf_anim->frames;
}

static GdkPixbufAnimationIter*
gdk_pixbuf_swf_anim_get_iter (GdkPixbufAnimation *anim,
                              const GTimeVal     *start_time)
{
        GdkPixbufSwfAnimIter *iter;

        iter = g_object_new (GDK_TYPE_PIXBUF_SWF_ANIM_ITER, NULL);

        iter->swf_anim = GDK_PIXBUF_SWF_ANIM (anim);

        g_object_ref (iter->swf_anim);
        
        iter_restart (iter);

        iter->start_time = *start_time;
        iter->current_time = *start_time;
        
        return GDK_PIXBUF_ANIMATION_ITER (iter);
}

static void gdk_pixbuf_swf_anim_iter_class_init (GdkPixbufSwfAnimIterClass *klass);
static void gdk_pixbuf_swf_anim_iter_finalize   (GObject                   *object);

static int        gdk_pixbuf_swf_anim_iter_get_delay_time             (GdkPixbufAnimationIter *iter);
static GdkPixbuf* gdk_pixbuf_swf_anim_iter_get_pixbuf                 (GdkPixbufAnimationIter *iter);
static gboolean   gdk_pixbuf_swf_anim_iter_on_currently_loading_frame (GdkPixbufAnimationIter *iter);
static gboolean   gdk_pixbuf_swf_anim_iter_advance                    (GdkPixbufAnimationIter *iter,
                                                                       const GTimeVal         *current_time);

static gpointer iter_parent_class;

GType
gdk_pixbuf_swf_anim_iter_get_type (void)
{
        static GType object_type = 0;

        if (!object_type) {
                static const GTypeInfo object_info = {
                        sizeof (GdkPixbufSwfAnimIterClass),
                        (GBaseInitFunc) NULL,
                        (GBaseFinalizeFunc) NULL,
                        (GClassInitFunc) gdk_pixbuf_swf_anim_iter_class_init,
                        NULL,           /* class_finalize */
                        NULL,           /* class_data */
                        sizeof (GdkPixbufSwfAnimIter),
                        0,              /* n_preallocs */
                        (GInstanceInitFunc) NULL,
                };
                
                object_type = g_type_register_static (GDK_TYPE_PIXBUF_ANIMATION_ITER,
                                                      "GdkPixbufSwfAnimIter",
                                                      &object_info, 0);
        }
        
        return object_type;
}

static void
gdk_pixbuf_swf_anim_iter_class_init (GdkPixbufSwfAnimIterClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GdkPixbufAnimationIterClass *anim_iter_class =
                GDK_PIXBUF_ANIMATION_ITER_CLASS (klass);
        
        iter_parent_class = g_type_class_peek_parent (klass);
        
        object_class->finalize = gdk_pixbuf_swf_anim_iter_finalize;

        anim_iter_class->get_delay_time = gdk_pixbuf_swf_anim_iter_get_delay_time;
        anim_iter_class->get_pixbuf = gdk_pixbuf_swf_anim_iter_get_pixbuf;
        anim_iter_class->on_currently_loading_frame = gdk_pixbuf_swf_anim_iter_on_currently_loading_frame;
        anim_iter_class->advance = gdk_pixbuf_swf_anim_iter_advance;
}

static void
gdk_pixbuf_swf_anim_iter_finalize (GObject *object)
{
        GdkPixbufSwfAnimIter *iter = GDK_PIXBUF_SWF_ANIM_ITER (object);

        iter_clear (iter);

        g_object_unref (iter->swf_anim);
        
        G_OBJECT_CLASS (iter_parent_class)->finalize (object);
}

static gboolean
gdk_pixbuf_swf_anim_iter_advance (GdkPixbufAnimationIter *anim_iter,
                                  const GTimeVal         *current_time)
{
        GdkPixbufSwfAnimIter *iter;
        gint elapsed;
        gint loop;
        GList *tmp;
        GList *old;
        
        iter = GDK_PIXBUF_SWF_ANIM_ITER (anim_iter);
        
        iter->current_time = *current_time;

        /* We use milliseconds for all times */
        elapsed =
          (((iter->current_time.tv_sec - iter->start_time.tv_sec) * G_USEC_PER_SEC +
            iter->current_time.tv_usec - iter->start_time.tv_usec)) / 1000;

        if (elapsed < 0) {
                /* Try to compensate; probably the system clock
                 * was set backwards
                 */
                iter->start_time = iter->current_time;
                elapsed = 0;
        }

        g_assert (iter->swf_anim->total_time > 0);
        
        /* See how many times we've already played the full animation,
         * and subtract time for that.
         */

        loop = elapsed / iter->swf_anim->total_time;
        elapsed = elapsed % iter->swf_anim->total_time;

        iter->position = elapsed;

        /* Now move to the proper frame */
        if (loop < 1) 
                tmp = iter->swf_anim->frames;
        else 
                tmp = NULL;
        while (tmp != NULL) {
                GdkPixbufFrame *frame = tmp->data;
                
                if (iter->position >= frame->elapsed &&
                    iter->position < (frame->elapsed + frame->delay_time))
                        break;

                tmp = tmp->next;
        }

        old = iter->current_frame;
        
        iter->current_frame = tmp;

        return iter->current_frame != old;
}

static int
gdk_pixbuf_swf_anim_iter_get_delay_time (GdkPixbufAnimationIter *anim_iter)
{
        GdkPixbufFrame *frame;
        GdkPixbufSwfAnimIter *iter;
  
        iter = GDK_PIXBUF_SWF_ANIM_ITER (anim_iter);

        if (iter->current_frame) {
                frame = iter->current_frame->data;
                return frame->delay_time - (iter->position - frame->elapsed);
        } else {
                return -1; /* show last frame forever */
        }
}

static GdkPixbuf*
gdk_pixbuf_swf_anim_iter_get_pixbuf (GdkPixbufAnimationIter *anim_iter)
{
        GdkPixbufSwfAnimIter *iter;
        GdkPixbufFrame *frame;
	GList *list;
        
        iter = GDK_PIXBUF_SWF_ANIM_ITER (anim_iter);

	if (iter->current_frame)
	  list = iter->current_frame;
	else 
	  list = g_list_last (iter->swf_anim->frames);
	if (list == NULL)
	  return NULL;

	frame = list->data;
        if (frame == NULL)
                return NULL;

        return frame->pixbuf;
}

static gboolean
gdk_pixbuf_swf_anim_iter_on_currently_loading_frame (GdkPixbufAnimationIter *anim_iter)
{
        GdkPixbufSwfAnimIter *iter;
  
        iter = GDK_PIXBUF_SWF_ANIM_ITER (anim_iter);

        return iter->current_frame == NULL || iter->current_frame->next == NULL;  
}
