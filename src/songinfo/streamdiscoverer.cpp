#include "streamdiscoverer.h"

#include <gst/pbutils/pbutils.h>
#include "core/logging.h"

const unsigned int StreamDiscoverer::kDiscoveryTimeoutS = 5;

StreamDiscoverer::StreamDiscoverer(const QString url)
    : QObject(nullptr),
      url_(),
      format_description_(),
      bitrate_(0),
      channels_(0),
      depth_(0),
      sample_rate_(0) {
  GstDiscoverer* discoverer =
      gst_discoverer_new(kDiscoveryTimeoutS * GST_SECOND, NULL);
  if (discoverer == NULL) {
    qLog(Debug) << "Error at creating discoverer" << endl;
    data_valid_ = false;
    return;
  }

  std::string url_std = url.toStdString();
  const char* url_c = url_std.c_str();
  GstDiscovererInfo* discoverer_info =
      gst_discoverer_discover_uri(discoverer, url_c, NULL);
  if (discoverer_info == NULL) {
    qLog(Debug) << "Could not discover url" << endl;
    data_valid_ = false;
    return;
  }

  // Redirected?
  const gchar* discovered_url = gst_discoverer_info_get_uri(discoverer_info);
  qLog(Debug) << "URL: " << discovered_url << endl;

  // Get audio streams (we will only care about the first one).
  GList* audio_streams = gst_discoverer_info_get_audio_streams(discoverer_info);
  qLog(Debug) << "Glist with audio_streams: " << g_list_length(audio_streams)
              << endl;
  if (g_list_length(audio_streams) == 0) {
    qLog(Debug) << "The stream does not contain audio" << endl;
    data_valid_ = false;
    return;
  }

  // We found a valid audio stream, extracting and saving its info:
  GstDiscovererStreamInfo* stream_audio_info =
      (GstDiscovererStreamInfo*)g_list_first(audio_streams)->data;
  // qLog(Debug) << "g_list_first()" << stream_audio_info << endl;
  url_ = QString(discovered_url);

  bitrate_ = gst_discoverer_audio_info_get_bitrate(
      GST_DISCOVERER_AUDIO_INFO(stream_audio_info));
  channels_ = gst_discoverer_audio_info_get_channels(
      GST_DISCOVERER_AUDIO_INFO(stream_audio_info));
  depth_ = gst_discoverer_audio_info_get_depth(
      GST_DISCOVERER_AUDIO_INFO(stream_audio_info));
  sample_rate_ = gst_discoverer_audio_info_get_sample_rate(
      GST_DISCOVERER_AUDIO_INFO(stream_audio_info));

  GstCaps* stream_caps = gst_discoverer_stream_info_get_caps(stream_audio_info);
  gchar* decoder_description = gst_pb_utils_get_codec_description(stream_caps);
  format_description_ = (decoder_description == NULL)
                            ? QString(tr("Unknown"))
                            : QString(decoder_description);

  data_valid_ = true;

  // Cleaning up:
  g_object_unref(discoverer);
  gst_discoverer_stream_info_unref(discoverer_info);
  gst_discoverer_stream_info_list_free(audio_streams);
  gst_caps_unref(stream_caps);
  g_free(decoder_description);
}

StreamDiscoverer::~StreamDiscoverer() {}

bool StreamDiscoverer::discoveryValid() const { return data_valid_; }
const QString& StreamDiscoverer::url() const { return url_; }
const QString& StreamDiscoverer::format() const { return format_description_; }
unsigned int StreamDiscoverer::bitrate() const { return bitrate_; }
unsigned int StreamDiscoverer::channels() const { return channels_; }
unsigned int StreamDiscoverer::depth() const { return depth_; }
unsigned int StreamDiscoverer::sampleRate() const { return sample_rate_; }
