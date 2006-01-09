
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <unistd.h>
#include <string.h>

#include "gstappsrc.h"

#include "spp.h"

static void gst_appsrc_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *param_spec);
static void gst_appsrc_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *param_spec);
static void gst_appsrc_uri_handler_init (gpointer g_iface, gpointer iface_data);
static GstFlowReturn gst_appsrc_create (GstPushSrc *push_src, GstBuffer **buffer);

GST_DEBUG_CATEGORY_STATIC (gst_appsrc_debug);
#define GST_CAT_DEFAULT gst_appsrc_debug

GstElementDetails gst_appsrc_details = GST_ELEMENT_DETAILS (
    "Application Source",
    "Source",
    "Push buffers created by an application",
    "David Schleef <ds@schleef.org");

enum {
  ARG_0,
  ARG_LOCATION,
  ARG_SOURCE_URL
};

GstStaticPadTemplate gst_appsrc_src_template = GST_STATIC_PAD_TEMPLATE (
    "src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

#if 0
static const GstEventMask *
gst_appsrc_get_event_mask (GstPad *pad)
{
  static const GstEventMask masks [] = {
    { GST_EVENT_SEEK, GST_SEEK_METHOD_CUR | GST_SEEK_METHOD_SET |
      GST_SEEK_FLAG_FLUSH },
    { 0, 0 }
  };
  return masks;
}

static const GstFormat *
gst_appsrc_get_formats (GstPad *pad)
{
  static const GstFormat formats[] = {
    GST_FORMAT_BYTES,
    0
  };

  return formats;
}
#endif

static void
_do_init (GType appsrc_type)
{
  static const GInterfaceInfo urihandler_info = {
    gst_appsrc_uri_handler_init,
    NULL,
    NULL
  };

  g_type_add_interface_static (appsrc_type, GST_TYPE_URI_HANDLER,
      &urihandler_info);

  GST_DEBUG_CATEGORY_INIT (gst_appsrc_debug, "appsrc", 0, "appsrc element");
}

GST_BOILERPLATE_FULL (GstAppSrc, gst_appsrc, GstPushSrc, GST_TYPE_PUSH_SRC,
    _do_init);


static void
gst_appsrc_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details (element_class, &gst_appsrc_details);
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_appsrc_src_template));
}

static void
gst_appsrc_class_init (GstAppSrcClass *klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstPushSrcClass *gstpushsrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstpushsrc_class = (GstPushSrcClass *) klass;

  gobject_class->set_property = gst_appsrc_set_property;
  gobject_class->get_property = gst_appsrc_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_LOCATION,
      g_param_spec_string ("location", "File Location",
        "Location of the file to read", NULL, G_PARAM_READWRITE));
  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_SOURCE_URL,
      g_param_spec_string ("source_url", "Source URL",
        "Source URL (as reported by mozilla)", NULL, G_PARAM_READABLE));

  //gstbasesrc_class->is_seekable = gst_appsrc_is_seekable;
  //gstbasesrc_class->do_seek = gst_appsrc_do_seek;
  //gstbasesrc_class->query = gst_appsrc_query;
  //gstbasesrc_class->get_times = gst_appsrc_get_times;

  gstpushsrc_class->create = gst_appsrc_create;
}

static void
gst_appsrc_init (GstAppSrc * appsrc, GstAppSrcClass *appsrc_class)
{
  /* FIXME: this gets leaked */
  appsrc->source_url = g_strdup("");

  //gst_base_src_set_live (GST_BASE_SRC (appsrc), TRUE);

  appsrc->buffer_queue = g_queue_new();
}


static void
gst_appsrc_set_property (GObject *object, guint prop_id, const GValue *value,
    GParamSpec *param_spec)
{
  GstAppSrc *appsrc;

  appsrc = GST_APPSRC (object);

  switch (prop_id) {
    case ARG_LOCATION:
      appsrc->fd = strtoul (g_value_get_string (value), NULL, 0);
      break;
    default:
      break;
  }
}

static void
gst_appsrc_get_property (GObject *object, guint prop_id, GValue *value,
    GParamSpec *param_spec)
{
  GstAppSrc *appsrc;
  char *s;

  appsrc = GST_APPSRC (object);

  switch (prop_id) {
    case ARG_LOCATION:
      s = g_strdup_printf ("%d", appsrc->fd);
      g_value_set_string (value, s);
      g_free (s);
      break;
    case ARG_SOURCE_URL:
      g_value_set_string (value, appsrc->source_url);
      break;
    default:
      break;
  }
}

static int
gst_appsrc_read_packet (GstAppSrc *appsrc, int timeout_usec)
{
  int ret;
  guint8 *data;
  struct timeval timeout;
  fd_set readfds;
  GstBuffer *buffer;
  int code;
  int length;

  FD_ZERO (&readfds);
  FD_SET(appsrc->fd, &readfds);
  timeout.tv_sec = 0;
  timeout.tv_usec = timeout_usec;
  ret = select (appsrc->fd + 1, &readfds, NULL, NULL, &timeout);
  GST_DEBUG("select returned %d", ret);
  
  if (ret > 0 && FD_ISSET (appsrc->fd, &readfds)) {
    ret = read (appsrc->fd, &code, 4);
    ret = read (appsrc->fd, &length, 4);
    
    GST_DEBUG ("got packet code=%d size=%d", code, length);

    data = g_malloc (length);
    ret = read (appsrc->fd, data, length);

    switch (code) {
      case SPP_EXIT:
        //exit(0);
        break;
      case SPP_DATA:
        buffer = gst_buffer_new ();
        GST_BUFFER_DATA (buffer) = data;
        data = NULL;
        GST_BUFFER_SIZE (buffer) = length;
        //GST_BUFFER_OFFSET (buffer) = offset;
        g_queue_push_tail (appsrc->buffer_queue, buffer);
        appsrc->head_index++;
        break;
      case SPP_EOF:
        appsrc->eof = TRUE;
        break;
      case SPP_SIZE:
        break;
      case SPP_METADATA:
        {
          char *tag;
          char *value;

          tag = g_strdup ((char *)data);
          value = g_strdup ((char *)data + strlen (tag) + 1);

          GST_DEBUG ("%s=%s", tag, value);

          if (strcmp(tag, "src") == 0) {
            if (appsrc->source_url) g_free(appsrc->source_url);
            appsrc->source_url = g_strdup(value);
            g_object_notify(G_OBJECT(appsrc),"source_url");
          }

          g_free (tag);
          g_free (value);
        }
        break;
      default:
        break;
    }
    if (data) {
      g_free (data);
    }
    return 1;
  }

  return 0;
}

static GstFlowReturn
gst_appsrc_create (GstPushSrc *push_src, GstBuffer **buffer)
{
  GstAppSrc *appsrc = GST_APPSRC(push_src);
  //GstBuffer *buffer;

  GST_DEBUG("create");

  gst_appsrc_read_packet (appsrc, 0);
  while (1) {
    if (!g_queue_is_empty(appsrc->buffer_queue)) {
      *buffer = g_queue_pop_head (appsrc->buffer_queue);
      return GST_FLOW_OK;
    }
    if (appsrc->eof) {
      return GST_FLOW_UNEXPECTED;
    }
    gst_appsrc_read_packet (appsrc, 10000);
  }
}

/* uri interface */

static guint
gst_appsrc_uri_get_type (void)
{
return GST_URI_SRC;
}

static gchar **
gst_appsrc_uri_get_protocols (void)
{
  static gchar *protocols[] = { "appsrc", NULL };

  return protocols;
}

static const gchar *
gst_appsrc_uri_get_uri (GstURIHandler *handler)
{
  GstAppSrc *src = GST_APPSRC (handler);
  return src->uri;
}

static gboolean
gst_appsrc_uri_set_uri (GstURIHandler *handler, const gchar *uri)
{
  GstAppSrc *src = GST_APPSRC (handler);

  if (src->uri) g_free(src->uri);
  src->uri = g_strdup(uri);

  return TRUE;
}

static void
gst_appsrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *)g_iface;

  iface->get_type = gst_appsrc_uri_get_type;
  iface->get_protocols = gst_appsrc_uri_get_protocols;
  iface->get_uri = gst_appsrc_uri_get_uri;
  iface->set_uri = gst_appsrc_uri_set_uri;
}

/* plugin stuff */

static gboolean
plugin_init (GstPlugin *plugin)
{
  return gst_element_register (plugin, "appsrc", 0, GST_TYPE_APPSRC);
}

GST_PLUGIN_DEFINE_STATIC (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "appsrc",
    "desc",
    plugin_init,
    "0.8",
    "LGPL",
    "gst-mozilla-plugin",
    "ds");

