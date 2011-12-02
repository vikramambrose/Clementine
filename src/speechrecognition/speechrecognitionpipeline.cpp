/* This file is part of Clementine.
   Copyright 2011, David Sansome <me@davidsansome.com>

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

#include "speechrecognitionpipeline.h"
#include "core/logging.h"
#include "engines/gstengine.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QMutexLocker>


SpeechRecognitionPipeline::SpeechRecognitionPipeline(GstEngine* engine, QObject* parent)
  : QIODevice(parent),
    engine_(engine),
    pipeline_(NULL),
    appsink_(NULL)
{
}

SpeechRecognitionPipeline::~SpeechRecognitionPipeline() {
  Stop();
}

bool SpeechRecognitionPipeline::Start() {
  if (pipeline_)
    return false;

  pipeline_ = gst_pipeline_new("pipeline");
  if (!pipeline_)
    return false;

  // Create elements
  GstElement* src        = engine_->CreateElement("autoaudiosrc", pipeline_);
  GstElement* convert    = engine_->CreateElement("audioconvert", pipeline_);
  GstElement* encode     = engine_->CreateElement("flacenc",      pipeline_);
  appsink_ = (GstAppSink*) engine_->CreateElement("appsink",      pipeline_);

  if (!src || !convert || !encode || !appsink_) {
    gst_object_unref(pipeline_);
    pipeline_ = NULL;
    return false;
  }

  // Set appsink callbacks
  GstAppSinkCallbacks callbacks;
  memset(&callbacks, 0, sizeof(callbacks));
  callbacks.new_buffer = &NewBufferCallback;
  gst_app_sink_set_callbacks(appsink_, &callbacks, this, NULL);

  // Create the caps filter to control the quality of the raw audio
  GstCaps* caps = gst_caps_new_simple("audio/x-raw-int",
      "channels", G_TYPE_INT, kChannels,
      "rate", G_TYPE_INT, kBitrate,
      "width", G_TYPE_INT, kBitsPerSample,
      "depth", G_TYPE_INT, kBitsPerSample,
      NULL);

  // Link the elements
  gst_element_link(src, convert);
  gst_element_link_filtered(convert, encode, caps);
  gst_element_link(encode, GST_ELEMENT(appsink_));
  gst_caps_unref(caps);

  // Add a probe to get the raw buffers before they're encoded
  gst_pad_add_buffer_probe(gst_element_get_pad(convert, "src"),
                           G_CALLBACK(ProbeCallback), this);

  // Ready to go
  GstStateChangeReturn ret = gst_element_set_state(pipeline_, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    return false;
  }

  open(QIODevice::ReadOnly);

  return true;
}

void SpeechRecognitionPipeline::Stop() {
  if (pipeline_) {
    gst_element_set_state(pipeline_, GST_STATE_NULL);
    gst_object_unref(GST_OBJECT(pipeline_));
    pipeline_ = NULL;
  }

  emit readChannelFinished();
  close();
}

GstFlowReturn SpeechRecognitionPipeline::NewBufferCallback(GstAppSink*, gpointer user_data) {
  SpeechRecognitionPipeline* me = reinterpret_cast<SpeechRecognitionPipeline*>(user_data);

  // Get the buffer from the appsink
  GstBuffer* buffer = gst_app_sink_pull_buffer(me->appsink_);
  if (!buffer) {
    return GST_FLOW_ERROR;
  }

  // Get the data from the buffer
  QByteArray data(reinterpret_cast<const char*>(GST_BUFFER_DATA(buffer)),
                  GST_BUFFER_SIZE(buffer));
  gst_buffer_unref(buffer);

  // Emit the data
  {
    QMutexLocker l(&me->pending_data_mutex_);
    me->pending_data_.append(data);
  }
  emit me->readyRead();

  return GST_FLOW_OK;
}

bool SpeechRecognitionPipeline::ProbeCallback(GstPad*, GstBuffer* buffer, gpointer user_data) {
  SpeechRecognitionPipeline* me = reinterpret_cast<SpeechRecognitionPipeline*>(user_data);

  // Get the data from the buffer
  QByteArray data(reinterpret_cast<const char*>(GST_BUFFER_DATA(buffer)),
                  GST_BUFFER_SIZE(buffer));

  // Emit the data
  emit me->NewRawData(data);

  return true;
}

qint64 SpeechRecognitionPipeline::bytesAvailable() const {
  QMutexLocker l(const_cast<QMutex*>(&pending_data_mutex_));
  return QIODevice::bytesAvailable() + pending_data_.size();
}

qint64 SpeechRecognitionPipeline::readData(char* data, qint64 maxlen) {
  const qint64 bytes = qMin(maxlen, qint64(pending_data_.size()));
  if (bytes != 0) {
    memcpy(data, pending_data_.data(), bytes);
    pending_data_.remove(0, bytes);
  }

  return bytes;
}
