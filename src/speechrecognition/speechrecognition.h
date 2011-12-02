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

#ifndef SPEECHRECOGNITION_H
#define SPEECHRECOGNITION_H

#include <QObject>
#include <QList>

#include "endpointer.h"

class QIODevice;
class QNetworkAccessManager;
class QNetworkReply;

class GstEngine;
class SpeechRecognitionPipeline;

class SpeechRecognition : public QObject {
  Q_OBJECT

public:
  SpeechRecognition(GstEngine* engine, QObject* parent = 0);

  static const char* kUrl;
  static const char* kContentType;

  static const int kEndpointerEstimationTimeMs;
  static const int kEndpointerFrameSizeSamples;

  struct Hypothesis {
    QString utterance;
    qreal confidence;
  };
  typedef QList<Hypothesis> Hypotheses;

  // This enumeration follows the values described here:
  // http://www.w3.org/2005/Incubator/htmlspeech/2010/10/google-api-draft.html#speech-input-error
  enum Result {
    Result_Success = 0,
    Result_ErrorAborted,
    Result_ErrorAudio,
    Result_ErrorNetwork,
    Result_NoSpeech,
    Result_NoMatch,
    Result_BadGrammar
  };

  bool Start();
  void Cancel();

signals:
  void Finished(Result result, const Hypotheses& hypotheses);

private slots:
  void SocketFinished();

  void PipelineNewRawData(const QByteArray& data);

private:
  void ParseResponse(QIODevice* reply, Result* result, Hypotheses* hypotheses);

private:
  QNetworkAccessManager* network_;

  SpeechRecognitionPipeline* pipeline_;
  QNetworkReply* reply_;

  QByteArray buffered_raw_data_;
  speech_input::Endpointer endpointer_;
  int num_samples_recorded_;
};

#endif // SPEECHRECOGNITION_H
