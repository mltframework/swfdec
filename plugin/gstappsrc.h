
#ifndef __GST_APPSRC_H__
#define __GST_APPSRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

G_BEGIN_DECLS

#define GST_TYPE_APPSRC \
    (gst_appsrc_get_type())
#define GST_APPSRC(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_APPSRC,GstAppSrc))
#define GST_APPSRC_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_APPSRC,GstAppSrcClass))
#define GST_IS_APPSRC(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_APPSRC))
#define GST_IS_APPSRC_CLASS(obj) \
    (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_APPSRC))

typedef struct _GstAppSrc GstAppSrc;
typedef struct _GstAppSrcClass GstAppSrcClass;

struct _GstAppSrc {
  GstPushSrc base_src;

  GQueue *buffer_queue;
  unsigned int head_offset;
  unsigned int head_index;
  unsigned int current_offset;
  unsigned int current_index;

  gchar *uri;
  
  int fd;

  gboolean seeking;
  gboolean need_discont;
  gboolean need_flush;
  unsigned int seek_offset;
  gboolean eof;

  gchar *source_url;
};

struct _GstAppSrcClass {
  GstPushSrcClass parent_class;

};

GType gst_appsrc_get_type (void);



G_END_DECLS

#endif

