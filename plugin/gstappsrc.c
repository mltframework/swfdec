
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <unistd.h>

#include "gstappsrc.h"

#include "spp.h"

static void gst_appsrc_base_init (gpointer g_class);
static void gst_appsrc_class_init (GstAppSrcClass *klass);
static void gst_appsrc_init (GstAppSrc * appsrc);
static gboolean gst_appsrc_event_handler (GstPad *pad, GstEvent *event);
static void gst_appsrc_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *param_spec);
static void gst_appsrc_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *param_spec);
static void gst_appsrc_loop (GstElement *element);
//static GstElementStateReturn gst_appsrc_change_state (GstElement * element);
static void gst_appsrc_uri_handler_init (gpointer g_iface, gpointer iface_data);

GST_DEBUG_CATEGORY_STATIC (gst_appsrc_debug);
#define GST_CAT_DEFAULT gst_appsrc_debug

GstElementDetails gst_appsrc_details = GST_ELEMENT_DETAILS (
    "Application Source",
    "Source",
    "Push buffers created by an application",
    "David Schleef <ds@schleef.org");

enum {
  ARG_0,
  ARG_LOCATION
};

GstStaticPadTemplate gst_appsrc_src_template = GST_STATIC_PAD_TEMPLATE (
    "src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

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

GST_BOILERPLATE_FULL (GstAppSrc, gst_appsrc, GstElement, GST_TYPE_ELEMENT,
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

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_appsrc_set_property;
  gobject_class->get_property = gst_appsrc_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), ARG_LOCATION,
      g_param_spec_string ("location", "File Location",
        "Location of the file to read", NULL, G_PARAM_READWRITE));

  //gstelement_class->change_state = gst_appsrc_change_state;
}

static void
gst_appsrc_init (GstAppSrc * appsrc)
{
  appsrc->srcpad = gst_pad_new_from_template (gst_static_pad_template_get (
          &gst_appsrc_src_template), "src");

  gst_pad_set_event_function (appsrc->srcpad, gst_appsrc_event_handler);
  gst_pad_set_event_mask_function (appsrc->srcpad, gst_appsrc_get_event_mask);
  //gst_pad_set_query_function (appsrc->srcpad, gst_appsrc_query);
  //gst_pad_set_query_mask_function (appsrc->srcpad, gst_appsrc_get_query_mask);
  gst_pad_set_formats_function (appsrc->srcpad, gst_appsrc_get_formats);
  gst_element_set_loop_function (GST_ELEMENT (appsrc), gst_appsrc_loop);

  gst_element_add_pad (GST_ELEMENT (appsrc), appsrc->srcpad);

  appsrc->buffers = g_array_new(FALSE, FALSE, sizeof (GstAppsrcBufferinfo));
}

static gboolean
gst_appsrc_event_handler (GstPad *pad, GstEvent *event)
{
  GstAppSrc *appsrc;

  appsrc = GST_APPSRC (gst_pad_get_parent(pad));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      appsrc->seeking = TRUE;
      switch (GST_EVENT_SEEK_METHOD (event)) {
        case GST_SEEK_METHOD_SET:
          GST_DEBUG("seek method set");
          appsrc->seek_offset = GST_EVENT_SEEK_OFFSET (event);
          break;
        case GST_SEEK_METHOD_CUR:
          GST_DEBUG("seek method cur");
          appsrc->seek_offset = appsrc->current_offset +
            GST_EVENT_SEEK_OFFSET (event);
          break;
        default:
          GST_DEBUG("unknown seek method %d", GST_EVENT_SEEK_METHOD(event));
          break;
      }
      GST_DEBUG ("got seek event offset=%d", appsrc->seek_offset);
      appsrc->need_discont = TRUE;
      appsrc->need_flush = GST_EVENT_SEEK_FLAGS (event) & GST_SEEK_FLAG_FLUSH;
      break;
    case GST_EVENT_SEEK_SEGMENT:
      break;
    case GST_EVENT_FLUSH:
      break;
    default:
      break;
  }
  gst_event_unref (event);

  return TRUE;
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
    default:
      break;
  }
}

static void
gst_appsrc_loop (GstElement *element)
{
  GstAppSrc *appsrc = GST_APPSRC(element);
  //GstBuffer *buffer;
  int ret;
  int code;
  int length;
  guint8 *data;
  GstAppsrcBufferinfo bufferinfo = { 0 };
  fd_set readfds;
  struct timeval timeout;

  GST_DEBUG("loop");

  FD_ZERO (&readfds);
  FD_SET(appsrc->fd, &readfds);
  timeout.tv_sec = 0;
  timeout.tv_usec = 0;
  ret = select (appsrc->fd + 1, &readfds, NULL, NULL, &timeout);
  GST_DEBUG("select returned %d", ret);
  if (ret > 0 && FD_ISSET (appsrc->fd, &readfds)) {
    ret = read (appsrc->fd, &code, 4);
    ret = read (appsrc->fd, &length, 4);

    data = g_malloc (length);
    ret = read (appsrc->fd, data, length);

    GST_DEBUG ("got packet code=%d size=%d", code, length);

    switch (code) {
      case SPP_EXIT:
        //exit(0);
        break;
      case SPP_DATA:
        bufferinfo.buffer = gst_buffer_new ();
        bufferinfo.offset = appsrc->head_offset;
        bufferinfo.size = length;
        GST_BUFFER_DATA (bufferinfo.buffer) = data;
        GST_BUFFER_SIZE (bufferinfo.buffer) = length;
        GST_BUFFER_OFFSET (bufferinfo.buffer) = bufferinfo.offset;
        g_array_append_val (appsrc->buffers, bufferinfo);
        appsrc->head_offset += length;
        appsrc->head_index++;
        break;
      case SPP_EOF:
        appsrc->eof = TRUE;
        break;
      case SPP_SIZE:
      break;
      default:
        break;
    }
  } else {
    if (appsrc->seeking) {
      int i;

      GST_DEBUG ("handling seek");

      appsrc->seeking = FALSE;
      for(i=0;i<appsrc->head_index;i++){
        GstAppsrcBufferinfo *bip;

        bip = &g_array_index (appsrc->buffers, GstAppsrcBufferinfo, i);
        if (appsrc->seek_offset < bip->offset + bip->size) break;
      }

      appsrc->current_index = i;
      appsrc->current_offset = appsrc->seek_offset;
      GST_DEBUG ("seek to index=%d", i);
      if (appsrc->need_flush) {
        appsrc->need_flush = FALSE;
        GST_DEBUG ("sending flush");
        gst_pad_push (appsrc->srcpad, GST_DATA (gst_event_new_flush()));
      }
      if (appsrc->need_discont) {
        GstEvent *event;

        event = gst_event_new_discontinuous (FALSE, GST_FORMAT_BYTES,
            (guint64) appsrc->current_offset, GST_FORMAT_UNDEFINED);

        GST_DEBUG ("sending discont");
        appsrc->need_discont = FALSE;
        gst_pad_push (appsrc->srcpad, GST_DATA (event));
      }
    }

    if (appsrc->current_index < appsrc->head_index) {
      GstAppsrcBufferinfo *bip;

      bip = &g_array_index (appsrc->buffers, GstAppsrcBufferinfo,
          appsrc->current_index);
      if (bip->offset == appsrc->current_offset) {
        GST_DEBUG ("pushing normal buffer offset=%d", bip->offset);
        gst_buffer_ref (bip->buffer);
        gst_pad_push (appsrc->srcpad, GST_DATA (bip->buffer));
        appsrc->current_index++;
        appsrc->current_offset += bip->size;
      } else {
        GstBuffer *buffer;
        int offset;
        int size;

        offset = appsrc->current_offset - bip->offset;
        size = bip->size - offset;
        GST_DEBUG ("pushing sub buffer offset=%d size=%d", offset, size);

        buffer = gst_buffer_create_sub (bip->buffer, offset, size);
        GST_BUFFER_OFFSET (buffer) = appsrc->current_offset;
        gst_pad_push (appsrc->srcpad, GST_DATA(buffer));
        appsrc->current_index++;
        appsrc->current_offset += size;
      }
    } else {
      if (appsrc->eof) {
        GST_DEBUG ("pushing EOS");
        gst_pad_push (appsrc->srcpad,
            GST_DATA (gst_event_new (GST_EVENT_EOS)));
        gst_element_set_eos (GST_ELEMENT(appsrc));
      } else {
        //usleep (1000);
      }
    }
  }
}

#if 0
static GstElementStateReturn
gst_appsrc_change_state (GstElement * element)
{
  GstAppSrc *appsrc;

  appsrc = GST_APPSRC (element);

  switch (GST_STATE_TRANSITION (element)) {
    case GST_STATE_NULL_TO_READY:
      break;
    case GST_STATE_READY_TO_PAUSED:
      break;
    case GST_STATE_PAUSED_TO_PLAYING:
      break;
    case GST_STATE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_PAUSED_TO_READY:
      break;
    case GST_STATE_READY_TO_NULL:
      break;
  }

  if (GST_ELEMENT_CLASS (parent_class)->change_state)
    return GST_ELEMENT_CLASS (parent_class)->change_state (element);

  return GST_STATE_SUCCESS;
}
#endif

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

