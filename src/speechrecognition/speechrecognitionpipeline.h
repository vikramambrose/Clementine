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

#ifndef SPEECHRECOGNITIONPIPELINE_H
#define SPEECHRECOGNITIONPIPELINE_H

#include <QIODevice>
#include <QMutex>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

class GstEngine;

// GStreamer pipeline that listens on the microphone and emits both raw audio
// buffers for environment analysis and UI, and flac-encoded buffers for
// sending to the server.
// The raw buffers are emitted via signals, the flac-encoded buffers are emitted
// through the QIODevice API.
class SpeechRecognitionPipeline : public QIODevice {
  Q_OBJECT

public:
  SpeechRecognitionPipeline(GstEngine* engine, QObject* parent = 0);
  ~SpeechRecognitionPipeline();

  static const int kChannels = 1;
  static const int kBitsPerSample = 16;
  static const int kBitrate = 16000;

  // Starts listening immediately.  Returns false if the pipeline could not
  // be created, true otherwise.
  bool Start();

  // Stop the pipeline and "close" the IO device.
  void Stop();

  // QIODevice
  bool isSequential() const { return true; }
  qint64 bytesAvailable() const;

signals:
  void NewRawData(const QByteArray& data);

protected:
  // QIODevice
  qint64 readData(char* data, qint64 maxlen);
  qint64 writeData(const char*, qint64) { return -1; }

private:
  // GStreamer callbacks
  static GstFlowReturn NewBufferCallback(GstAppSink* sink, gpointer data);
  static bool ProbeCallback(GstPad* pad, GstBuffer* buffer, gpointer data);

private:
  GstEngine* engine_;

  QMutex pending_data_mutex_;
  QByteArray pending_data_;

  GstElement* pipeline_;
  GstAppSink* appsink_;
};

#endif // SPEECHRECOGNITIONPIPELINE_H
