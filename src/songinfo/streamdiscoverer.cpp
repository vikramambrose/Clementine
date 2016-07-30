#include "streamdiscoverer.h"

#include <QEventLoop>

#include <gst/pbutils/pbutils.h>
#include "core/logging.h"
#include "core/signalchecker.h"

const unsigned int StreamDiscoverer::kDiscoveryTimeoutS = 10;

StreamDiscoverer::StreamDiscoverer()
    : QObject(nullptr),
      url_(),
      format_description_(),
      bitrate_(0),
      channels_(0),
      depth_(0),
      sample_rate_(0) {
  // Setting up a discoverer:
  discoverer_ = gst_discoverer_new(kDiscoveryTimeoutS * GST_SECOND, NULL);
  if (discoverer_ == NULL) {
    qLog(Error) << "Error at creating discoverer" << endl;
    data_valid_ = false;
    return;
  }

  // Connecting its signals:
  CHECKED_GCONNECT(discoverer_, "discovered", &on_discovered_cb, this);
  CHECKED_GCONNECT(discoverer_, "finished", &on_finished_cb, this);

  // Starting the discoverer process:
  gst_discoverer_start(discoverer_);
}

StreamDiscoverer::~StreamDiscoverer() {
  // Stop the discoverer process and free it:
  qLog(Debug) << "~Streamdiscoverer()" << endl;
  gst_discoverer_stop(discoverer_);
  g_object_unref(discoverer_);
}

void StreamDiscoverer::discover(QString url) {
  // Adding the request to discover the url given as a parameter:
  qLog(Debug) << "discover(" << url << ")";
  std::string url_std = url.toStdString();
  const char* url_c = url_std.c_str();
  if (!gst_discoverer_discover_uri_async(discoverer_, url_c)) {
    qLog(Error) << "Failed to start discovering URL " << url << endl;
    g_object_unref(discoverer_);
    data_valid_ = false;
    return;
  }

  // Creating a loop and setting it to run. That way we can wait for signals.
  QEventLoop loop;
  loop.connect(this, SIGNAL(DiscoverererFinished()), SLOT(quit()));
  loop.exec();
}

void StreamDiscoverer::on_discovered_cb(GstDiscoverer* discoverer,
                                        GstDiscovererInfo* info, GError* err,
                                        gpointer self) {
  StreamDiscoverer* instance = reinterpret_cast<StreamDiscoverer*>(self);

  qLog(Debug) << "on_discovered_cb(" << instance;

  // Redirected?
  QString discovered_url(gst_discoverer_info_get_uri(info));
  qLog(Debug) << "URL: " << discovered_url << endl;

  GstDiscovererResult result = gst_discoverer_info_get_result(info);
  if (result != GST_DISCOVERER_OK) {
    emit instance->Error(tr("Error discovering %1: %2").arg(discovered_url).arg(
        gstDiscovererErrorMessage(result)));
    return;
  }

  // Get audio streams (we will only care about the first one).
  GList* audio_streams = gst_discoverer_info_get_audio_streams(info);
  qLog(Debug) << "Glist with audio_streams: " << g_list_length(audio_streams)
              << endl;

  if (g_list_length(audio_streams) > 0) {
    // We found a valid audio stream, extracting and saving its info:
    GstDiscovererStreamInfo* stream_audio_info =
        (GstDiscovererStreamInfo*)g_list_first(audio_streams)->data;

    instance->url_ = discovered_url;
    instance->bitrate_ = gst_discoverer_audio_info_get_bitrate(
        GST_DISCOVERER_AUDIO_INFO(stream_audio_info));
    instance->channels_ = gst_discoverer_audio_info_get_channels(
        GST_DISCOVERER_AUDIO_INFO(stream_audio_info));
    instance->depth_ = gst_discoverer_audio_info_get_depth(
        GST_DISCOVERER_AUDIO_INFO(stream_audio_info));
    instance->sample_rate_ = gst_discoverer_audio_info_get_sample_rate(
        GST_DISCOVERER_AUDIO_INFO(stream_audio_info));

    // Human-readable codec name:
    GstCaps* stream_caps =
        gst_discoverer_stream_info_get_caps(stream_audio_info);
    gchar* decoder_description =
        gst_pb_utils_get_codec_description(stream_caps);
    instance->format_description_ = (decoder_description == NULL)
                                        ? QString(tr("Unknown"))
                                        : QString(decoder_description);

    gst_caps_unref(stream_caps);
    g_free(decoder_description);

    instance->data_valid_ = true;
    emit instance->DataReady();

  } else {
    instance->data_valid_ = false;
    emit instance->Error(
        tr("Could not get audio information").arg(discovered_url));
  }

  gst_discoverer_stream_info_list_free(audio_streams);
}

void StreamDiscoverer::on_finished_cb(GstDiscoverer* discoverer,
                                      gpointer self) {
  // The discoverer doesn't have any more urls in its queue. Let the loop know
  // it can exit.
  StreamDiscoverer* instance = reinterpret_cast<StreamDiscoverer*>(self);
  emit instance->DiscoverererFinished();
}

// This will have to be replaced with a type StreamType that gets passed by copy
// with the DataReady signal.
bool StreamDiscoverer::dataValid() const { return data_valid_; }
const QString& StreamDiscoverer::url() const { return url_; }
const QString& StreamDiscoverer::format() const { return format_description_; }
unsigned int StreamDiscoverer::bitrate() const { return bitrate_; }
unsigned int StreamDiscoverer::channels() const { return channels_; }
unsigned int StreamDiscoverer::depth() const { return depth_; }
unsigned int StreamDiscoverer::sampleRate() const { return sample_rate_; }
//

QString StreamDiscoverer::gstDiscovererErrorMessage(
    GstDiscovererResult result) {
  switch (result) {
    case (GST_DISCOVERER_URI_INVALID):
      return tr("Invalid URL");
    case (GST_DISCOVERER_TIMEOUT):
      return tr("Connection timed out");
    case (GST_DISCOVERER_BUSY):
      return tr("The discoverer is busy");
    case (GST_DISCOVERER_MISSING_PLUGINS):
      return tr("Missing plugins");
    case (GST_DISCOVERER_ERROR):
    default:
      return tr("Could not get details");
  }
}
