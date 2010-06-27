/* This file is part of Clementine.

   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gstenginepipeline.h"
#include "gstequalizer.h"
#include "gstengine.h"
#include "bufferconsumer.h"

#include <QDebug>

const int GstEnginePipeline::kGstStateTimeoutNanosecs = 10000000;
const int GstEnginePipeline::kFaderFudgeMsec = 2000;

GstEnginePipeline::GstEnginePipeline(GstEngine* engine)
  : QObject(NULL),
    engine_(engine),
    valid_(false),
    sink_(GstEngine::kAutoSink),
    rg_enabled_(false),
    rg_mode_(0),
    rg_preamp_(0.0),
    rg_compression_(true),
    volume_percent_(100),
    volume_modifier_(1.0),
    fader_(NULL),
    pipeline_(NULL),
    uridecodebin_(NULL),
    audiobin_(NULL),
    audioconvert_(NULL),
    rgvolume_(NULL),
    rglimiter_(NULL),
    audioconvert2_(NULL),
    equalizer_(NULL),
    volume_(NULL),
    audioscale_(NULL),
    audiosink_(NULL)
{
}

void GstEnginePipeline::set_output_device(const QString &sink, const QString &device) {
  sink_ = sink;
  device_ = device;
}

void GstEnginePipeline::set_replaygain(bool enabled, int mode, float preamp,
                                       bool compression) {
  rg_enabled_ = enabled;
  rg_mode_ = mode;
  rg_preamp_ = preamp;
  rg_compression_ = compression;
}

bool GstEnginePipeline::StopUriDecodeBin(gpointer bin) {
  gst_element_set_state(GST_ELEMENT(bin), GST_STATE_NULL);
  return false; // So it doesn't get called again
}

bool GstEnginePipeline::ReplaceDecodeBin(const QUrl& url) {
  return true;
}

bool GstEnginePipeline::Init(const QUrl &url) {
  pipeline_ = gst_pipeline_new("pipeline");
  url_ = url;

  // Here we create all the parts of the gstreamer pipeline - from the source
  // to the sink.  The parts of the pipeline are split up into bins:
  //   uri decode bin -> audio bin
  // The uri decode bin is a gstreamer builtin that automatically picks the
  // right type of source and decoder for the URI.
  // The audio bin gets created here and contains:
  //   audioconvert -> rgvolume -> rglimiter -> equalizer -> volume ->
  //   audioscale -> audioconvert -> audiosink

  // Decode bin
  GstElement* playbin = engine_->CreateElement("playbin2", pipeline_);
  g_object_set(G_OBJECT(playbin), "uri", url.toEncoded().constData(), NULL);

  gst_bus_set_sync_handler(gst_pipeline_get_bus(GST_PIPELINE(pipeline_)), BusCallbackSync, this);
  bus_cb_id_ = gst_bus_add_watch(gst_pipeline_get_bus(GST_PIPELINE(pipeline_)), BusCallback, this);

  return true;
}

GstEnginePipeline::~GstEnginePipeline() {
  if (pipeline_) {
    gst_bus_set_sync_handler(gst_pipeline_get_bus(GST_PIPELINE(pipeline_)), NULL, NULL);
    g_source_remove(bus_cb_id_);
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline_));
  }
}



gboolean GstEnginePipeline::BusCallback(GstBus*, GstMessage* msg, gpointer self) {
  GstEnginePipeline* instance = reinterpret_cast<GstEnginePipeline*>(self);

  switch ( GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_ERROR:
      instance->ErrorMessageReceived(msg);
      break;

    case GST_MESSAGE_TAG:
      instance->TagMessageReceived(msg);
      break;

    default:
      break;
  }

  return FALSE;
}

GstBusSyncReply GstEnginePipeline::BusCallbackSync(GstBus*, GstMessage* msg, gpointer self) {
  GstEnginePipeline* instance = reinterpret_cast<GstEnginePipeline*>(self);
  switch (GST_MESSAGE_TYPE(msg)) {
    case GST_MESSAGE_EOS:
      emit instance->EndOfStreamReached(false);
      break;

    case GST_MESSAGE_TAG:
      instance->TagMessageReceived(msg);
      break;

    case GST_MESSAGE_ERROR:
      instance->ErrorMessageReceived(msg);
      break;

    case GST_MESSAGE_ELEMENT:
      instance->ElementMessageReceived(msg);
      break;

    default:
      break;
  }
  return GST_BUS_PASS;
}

void GstEnginePipeline::ElementMessageReceived(GstMessage* msg) {
  const GstStructure* structure = gst_message_get_structure(msg);

  if (gst_structure_has_name(structure, "redirect")) {
    const char* uri = gst_structure_get_string(structure, "new-location");

    // Set the redirect URL.  In mmssrc redirect messages come during the
    // initial state change to PLAYING, so callers can pick up this URL after
    // the state change has failed.
    redirect_url_ = QUrl::fromEncoded(uri);
  }
}

void GstEnginePipeline::ErrorMessageReceived(GstMessage* msg) {
  GError* error;
  gchar* debugs;

  gst_message_parse_error(msg, &error, &debugs);
  QString message = QString::fromLocal8Bit(error->message);
  QString debugstr = QString::fromLocal8Bit(debugs);

  g_error_free(error);
  free(debugs);

  if (!redirect_url_.isEmpty() && debugstr.contains(
      "A redirect message was posted on the bus and should have been handled by the application.")) {
    // mmssrc posts a message on the bus *and* makes an error message when it
    // wants to do a redirect.  We handle the message, but now we have to
    // ignore the error too.
    return;
  }

  qDebug() << debugstr;
  emit Error(message);
}

void GstEnginePipeline::TagMessageReceived(GstMessage* msg) {
  GstTagList* taglist = NULL;
  gst_message_parse_tag(msg, &taglist);

  Engine::SimpleMetaBundle bundle;
  bundle.title = ParseTag(taglist, GST_TAG_TITLE);
  bundle.artist = ParseTag(taglist, GST_TAG_ARTIST);
  bundle.comment = ParseTag(taglist, GST_TAG_COMMENT);
  bundle.album = ParseTag(taglist, GST_TAG_ALBUM);

  gst_tag_list_free(taglist);

  if (!bundle.title.isEmpty() || !bundle.artist.isEmpty() ||
      !bundle.comment.isEmpty() || !bundle.album.isEmpty())
    emit MetadataFound(bundle);
}

QString GstEnginePipeline::ParseTag(GstTagList* list, const char* tag) const {
  gchar* data = NULL;
  bool success = gst_tag_list_get_string(list, tag, &data);

  QString ret;
  if (success && data) {
    ret = QString::fromUtf8(data);
    g_free(data);
  }
  return ret.trimmed();
}


void GstEnginePipeline::NewPadCallback(GstElement*, GstPad* pad, gpointer self) {
  GstEnginePipeline* instance = reinterpret_cast<GstEnginePipeline*>(self);
  GstPad* const audiopad = gst_element_get_pad(instance->audiobin_, "sink");

  if (GST_PAD_IS_LINKED(audiopad)) {
    qDebug() << "audiopad is already linked. Unlinking old pad.";
    gst_pad_unlink(audiopad, GST_PAD_PEER(audiopad));
  }

  gst_pad_link(pad, audiopad);

  gst_object_unref(audiopad);
}


bool GstEnginePipeline::HandoffCallback(GstPad*, GstBuffer* buf, gpointer self) {
  GstEnginePipeline* instance = reinterpret_cast<GstEnginePipeline*>(self);

  QList<BufferConsumer*> consumers;
  {
    QMutexLocker l(&instance->buffer_consumers_mutex_);
    consumers = instance->buffer_consumers_;
  }

  foreach (BufferConsumer* consumer, consumers) {
    gst_buffer_ref(buf);
    consumer->ConsumeBuffer(buf, instance);
  }

  return true;
}

void GstEnginePipeline::SourceDrainedCallback(GstURIDecodeBin* bin, gpointer self) {
}

qint64 GstEnginePipeline::position() const {
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 value = 0;
  gst_element_query_position(pipeline_, &fmt, &value);

  return value;
}


qint64 GstEnginePipeline::length() const {
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 value = 0;
  gst_element_query_duration(pipeline_,  &fmt, &value);

  return value;
}


GstState GstEnginePipeline::state() const {
  GstState s, sp;
  if (gst_element_get_state(pipeline_, &s, &sp, kGstStateTimeoutNanosecs) ==
      GST_STATE_CHANGE_FAILURE)
    return GST_STATE_NULL;

  return s;
}

bool GstEnginePipeline::SetState(GstState state) {
  return gst_element_set_state(pipeline_, state) != GST_STATE_CHANGE_FAILURE;
}

bool GstEnginePipeline::Seek(qint64 nanosec) {
  return gst_element_seek_simple(pipeline_, GST_FORMAT_TIME,
                                 GST_SEEK_FLAG_FLUSH, nanosec);
}

void GstEnginePipeline::SetEqualizerEnabled(bool enabled) {
}


void GstEnginePipeline::SetEqualizerParams(int preamp, const QList<int>& band_gains) {
}

void GstEnginePipeline::SetVolume(int percent) {
  volume_percent_ = percent;
  UpdateVolume();
}

void GstEnginePipeline::SetVolumeModifier(qreal mod) {
  volume_modifier_ = mod;
  UpdateVolume();
}

void GstEnginePipeline::UpdateVolume() {
}

void GstEnginePipeline::StartFader(int duration_msec,
                                   QTimeLine::Direction direction,
                                   QTimeLine::CurveShape shape) {
  // If there's already another fader running then start from the same time
  // that one was already at.
  int start_time = direction == QTimeLine::Forward ? 0 : duration_msec;
  if (fader_ && fader_->state() == QTimeLine::Running)
    start_time = fader_->currentTime();

  fader_.reset(new QTimeLine(duration_msec, this));
  connect(fader_.get(), SIGNAL(valueChanged(qreal)), SLOT(SetVolumeModifier(qreal)));
  connect(fader_.get(), SIGNAL(finished()), SLOT(FaderTimelineFinished()));
  fader_->setDirection(direction);
  fader_->setCurveShape(shape);
  fader_->setCurrentTime(start_time);
  fader_->resume();

  fader_fudge_timer_.stop();

  SetVolumeModifier(fader_->currentValue());
}

void GstEnginePipeline::FaderTimelineFinished() {
  fader_.reset();

  // Wait a little while longer before emitting the finished signal (and
  // probably distroying the pipeline) to account for delays in the audio
  // server/driver.
  fader_fudge_timer_.start(kFaderFudgeMsec, this);
}

void GstEnginePipeline::timerEvent(QTimerEvent* e) {
  if (e->timerId() == fader_fudge_timer_.timerId()) {
    fader_fudge_timer_.stop();
    emit FaderFinished();
    return;
  }

  QObject::timerEvent(e);
}

void GstEnginePipeline::AddBufferConsumer(BufferConsumer *consumer) {
  QMutexLocker l(&buffer_consumers_mutex_);
  buffer_consumers_ << consumer;
}

void GstEnginePipeline::RemoveBufferConsumer(BufferConsumer *consumer) {
  QMutexLocker l(&buffer_consumers_mutex_);
  buffer_consumers_.removeAll(consumer);
}

void GstEnginePipeline::RemoveAllBufferConsumers() {
  QMutexLocker l(&buffer_consumers_mutex_);
  buffer_consumers_.clear();
}
