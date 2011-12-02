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

#include "speechrecognition.h"
#include "speechrecognitionpipeline.h"
#include "core/logging.h"
#include "core/network.h"
#include "core/timeconstants.h"

#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSslSocket>
#include <QUrl>

#include <qjson/parser.h>


const char* SpeechRecognition::kContentType = "audio/x-flac; rate=16000";
const char* SpeechRecognition::kUrl =
    "http://www.google.com/speech-api/v1/recognize?xjerr=1&client=clementine&lang=en-US";


const int SpeechRecognition::kEndpointerEstimationTimeMs = 300;
const int SpeechRecognition::kEndpointerFrameSizeSamples = 320;

// The following constants are related to the volume level indicator shown in
// the UI for recorded audio.
// Multiplier used when new volume is greater than previous level.
const float SpeechRecognition::kUpSmoothingFactor = 1.0f;
// Multiplier used when new volume is lesser than previous level.
const float SpeechRecognition::kDownSmoothingFactor = 0.7f;
// RMS dB value of a maximum (unclipped) sine wave for int16 samples.
const float SpeechRecognition::kAudioMeterMaxDb = 90.31f;
// This value corresponds to RMS dB for int16 with 6 most-significant-bits = 0.
// Values lower than this will display as empty level-meter.
const float SpeechRecognition::kAudioMeterMinDb = 30.0f;
const float SpeechRecognition::kAudioMeterDbRange = kAudioMeterMaxDb - kAudioMeterMinDb;


SpeechRecognition::SpeechRecognition(GstEngine* engine, QObject* parent)
  : QObject(parent),
    network_(new NetworkAccessManager(this)),
    pipeline_(new SpeechRecognitionPipeline(engine, this)),
    reply_(NULL),
    endpointer_(SpeechRecognitionPipeline::kBitrate),
    num_samples_recorded_(0),
    audio_level_(0)
{
  endpointer_.set_speech_input_complete_silence_length(kUsecPerSec / 2);
  endpointer_.set_long_speech_input_complete_silence_length(kUsecPerSec);
  endpointer_.set_long_speech_length(3 * kUsecPerSec);

  connect(pipeline_, SIGNAL(NewRawData(QByteArray)), SLOT(PipelineNewRawData(QByteArray)));
}

bool SpeechRecognition::Start() {
  // Start the GStreamer pipeline.
  if (!pipeline_->Start())
    return false;

  // The endpointer decides when the user has finished talking.
  endpointer_.StartSession();
  endpointer_.SetEnvironmentEstimationMode();

  // Start the network request to the server.
  const QUrl url(kUrl);
  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, kContentType);
  req.setAttribute(QNetworkRequest::DoNotBufferUploadDataAttribute, false);
  req.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                   QNetworkRequest::AlwaysNetwork);

  // The network request takes the flac-encoded data from the pipeline.
  reply_ = network_->post(req, pipeline_);
  connect(reply_, SIGNAL(finished()), SLOT(SocketFinished()));

  return true;
}

void SpeechRecognition::SocketFinished() {
  Result result = Result_ErrorNetwork;
  Hypotheses hypotheses;

  if (reply_->error() != QNetworkReply::NoError) {
    qLog(Warning) << reply_->errorString();
  } else {
    ParseResponse(reply_, &result, &hypotheses);
  }

  emit Finished(result, hypotheses);

  reply_->deleteLater();
  reply_ = NULL;

  delete pipeline_;
  pipeline_ = NULL;
}

void SpeechRecognition::ParseResponse(QIODevice* reply, Result* result,
                                      Hypotheses* hypotheses) {
  QJson::Parser parser;
  QVariantMap data = parser.parse(reply).toMap();

  const int status = data.value("status", Result_ErrorNetwork).toInt();
  *result = static_cast<Result>(status);

  if (status != Result_Success)
    return;

  QVariantList list = data.value("hypotheses", QVariantList()).toList();
  foreach (const QVariant& variant, list) {
    QVariantMap map = variant.toMap();

    if (!map.contains("utterance") || !map.contains("confidence"))
      continue;

    Hypothesis hypothesis;
    hypothesis.utterance = map.value("utterance", QString()).toString();
    hypothesis.confidence = map.value("confidence", 0.0).toReal();
    *hypotheses << hypothesis;

    qLog(Debug) << hypothesis.confidence << hypothesis.utterance;
  }
}

void SpeechRecognition::PipelineNewRawData(const QByteArray& data) {
  if (!reply_ || !pipeline_)
    return;

  // We've got some raw data, we have to give it to the endpointer so it can
  // figure out when the user starts talking.  The endpointer processes data
  // in whole "frames", so we have to split up and buffer our data here before
  // passing it to the endpointer.

  const int kSamples = kEndpointerFrameSizeSamples;
  const int kBytes   = kSamples * sizeof(qint16);

  buffered_raw_data_ += data;
  while (buffered_raw_data_.size() >= kBytes) {
    num_samples_recorded_ += kSamples;

    // Process this frame
    float rms;
    endpointer_.ProcessAudio(reinterpret_cast<const qint16*>(buffered_raw_data_.data()),
                             kEndpointerFrameSizeSamples, &rms);

    // Remove this frame's data from the buffer
    buffered_raw_data_.remove(0, kEndpointerFrameSizeSamples);

    if (endpointer_.IsEstimatingEnvironment()) {
      if (num_samples_recorded_ >=
          kEndpointerEstimationTimeMs * SpeechRecognitionPipeline::kBitrate / 1000) {
        // We've sampled enough of the envionment now - switch to user input
        // mode.
        endpointer_.SetUserInputMode();
      }

      // We're still sampling the environment so don't do any further
      // processing.
      break;
    }

    // Calculate the input volume to show in the UI, smoothing towards the new
    // level.
    float level = (rms - kAudioMeterMinDb) /
        (kAudioMeterDbRange / kAudioMeterRangeMaxUnclipped);
    level = qBound(0.0f, level, 1.0f);

    if (level > audio_level_) {
      audio_level_ += (level - audio_level_) * kUpSmoothingFactor;
    } else {
      audio_level_ += (level - audio_level_) * kDownSmoothingFactor;
    }

    emit AudioLevelChanged(audio_level_);


    if (endpointer_.speech_input_complete()) {
      qLog(Debug) << "Speech complete";

      // Speech finished - stop listening on the microphone and finish sending
      // data to the server.
      pipeline_->Stop();
      break;
    }
  }
}
