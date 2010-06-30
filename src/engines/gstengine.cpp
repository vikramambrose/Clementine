/***************************************************************************
 *   Copyright (C) 2003-2005 by Mark Kretschmann <markey@web.de>           *
 *   Copyright (C) 2005 by Jakub Stachowski <qbast@go2.pl>                 *
 *   Copyright (C) 2006 Paul Cifarelli <paul@cifarelli.net>                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#define DEBUG_PREFIX "Gst-Engine"

#include "gstengine.h"
#include "gstequalizer.h"
#include "gstenginepipeline.h"

#include <math.h>
#include <unistd.h>
#include <vector>
#include <iostream>

#include <boost/bind.hpp>

#include <QTimer>
#include <QRegExp>
#include <QFile>
#include <QSettings>
#include <QtDebug>
#include <QCoreApplication>
#include <QTimeLine>
#include <QDir>

#include <gst/gst.h>


using std::vector;
using boost::shared_ptr;

const char* GstEngine::kSettingsGroup = "GstEngine";
const char* GstEngine::kAutoSink = "autoaudiosink";


GstEngine::GstEngine()
  : Engine::Base(),
    equalizer_enabled_(false),
    rg_enabled_(false),
    rg_mode_(0),
    rg_preamp_(0.0),
    rg_compression_(true),
    timer_id_(-1)
{
  ReloadSettings();
}

GstEngine::~GstEngine() {
  current_pipeline_.reset();

  // Save configuration
  gst_deinit();
}

void GstEngine::SetEnv(const char *key, const QString &value) {
#ifdef Q_OS_WIN32
  putenv(QString("%1=%2").arg(key, value).toLocal8Bit().constData());
#else
  setenv(key, value.toLocal8Bit().constData(), 1);
#endif
}

bool GstEngine::Init() {
  QString scanner_path;
  QString plugin_path;
  QString registry_filename;

  // On windows and mac we bundle the gstreamer plugins with clementine
#if defined(Q_OS_DARWIN)
  scanner_path = QCoreApplication::applicationDirPath() + "/../PlugIns/gst-plugin-scanner";
  plugin_path = QCoreApplication::applicationDirPath() + "/../PlugIns/gstreamer";
#elif defined(Q_OS_WIN32)
  plugin_path = QCoreApplication::applicationDirPath() + "/gstreamer-plugins";
#endif

#if defined(Q_OS_WIN32) || defined(Q_OS_DARWIN)
  registry_filename = QString("%1/.config/%2/gst-registry-%3.bin").arg(
      QDir::homePath(), QCoreApplication::organizationName(),
      QCoreApplication::applicationVersion());
#endif

  if (!scanner_path.isEmpty())
    SetEnv("GST_PLUGIN_SCANNER", scanner_path);

  if (!plugin_path.isEmpty()) {
    SetEnv("GST_PLUGIN_PATH", plugin_path);
    // Never load plugins from anywhere else.
    SetEnv("GST_PLUGIN_SYSTEM_PATH", plugin_path);
  }

  if (!registry_filename.isEmpty()) {
    SetEnv("GST_REGISTRY", registry_filename);
  }

  // GStreamer initialization
  GError *err;
  if ( !gst_init_check( NULL, NULL, &err ) ) {
    qWarning("GStreamer could not be initialized");
    return false;
  }

  return true;
}

void GstEngine::ReloadSettings() {
  Engine::Base::ReloadSettings();

  QSettings s;
  s.beginGroup(kSettingsGroup);

  sink_ = s.value("sink", kAutoSink).toString();
  device_ = s.value("device").toString();

#ifdef Q_OS_WIN32
  if (sink_ == kAutoSink) {
    // HACK: Force the direct sound sink on Windows unless the user has
    // explicitly chosen otherwise.
    sink_ = "directsoundsink";
  }
#endif

  rg_enabled_ = s.value("rgenabled", false).toBool();
  rg_mode_ = s.value("rgmode", 0).toInt();
  rg_preamp_ = s.value("rgpreamp", 0.0).toDouble();
  rg_compression_ = s.value("rgcompression", true).toBool();
}


uint GstEngine::position() const {
  if (!current_pipeline_)
    return 0;

  return uint(current_pipeline_->position() / GST_MSECOND);
}

uint GstEngine::length() const {
  if (!current_pipeline_)
    return 0;

  return uint(current_pipeline_->length() / GST_MSECOND);
}


Engine::State GstEngine::state() const {
  if (!current_pipeline_)
    return url_.isEmpty() ? Engine::Empty : Engine::Idle;

  switch (current_pipeline_->state()) {
    case GST_STATE_NULL:    return Engine::Empty;
    case GST_STATE_READY:   return Engine::Idle;
    case GST_STATE_PLAYING: return Engine::Playing;
    case GST_STATE_PAUSED:  return Engine::Paused;
    default:                return Engine::Empty;
  }
}

const Engine::Scope& GstEngine::scope() {
  return scope_;
}

bool GstEngine::Load(const QUrl& url, Engine::TrackChangeType change) {
  Engine::Base::Load(url, change);

  // Clementine just crashes when asked to load a file that doesn't exist on
  // Windows, so check for that here.  This is definitely the wrong place for
  // this "fix"...
  if (url.scheme() == "file" && !QFile::exists(url.toLocalFile()))
    return false;

  QUrl gst_url = url;

  // It's a file:// url with a hostname set.  QUrl::fromLocalFile does this
  // when given a \\host\share\file path on Windows.  Munge it back into a
  // path that gstreamer will recognise.
  if (url.scheme() == "file" && !url.host().isEmpty()) {
    gst_url.setPath("//" + gst_url.host() + gst_url.path());
    gst_url.setHost(QString());
  }

  const bool crossfade = current_pipeline_ &&
                         ((crossfade_enabled_ && change == Engine::Manual) ||
                          (autocrossfade_enabled_ && change == Engine::Auto));

  if (!crossfade && current_pipeline_ && current_pipeline_->url() == gst_url &&
      change == Engine::Auto) {
    // We're not crossfading, and the pipeline is already playing the URI we
    // want, so just do nothing.
    return true;
  }

  shared_ptr<GstEnginePipeline> pipeline;
  pipeline = CreatePipeline(gst_url);
  if (!pipeline)
    return false;

  if (crossfade)
    StartFadeout();

  current_pipeline_ = pipeline;

  SetVolume(volume_);
  SetEqualizerEnabled(equalizer_enabled_);
  SetEqualizerParameters(equalizer_preamp_, equalizer_gains_);

  // Maybe fade in this track
  if (crossfade)
    current_pipeline_->StartFader(fadeout_duration_, QTimeLine::Forward);

  return true;
}

void GstEngine::StartFadeout() {
  fadeout_pipeline_ = current_pipeline_;
  disconnect(fadeout_pipeline_.get(), 0, 0, 0);
  fadeout_pipeline_->RemoveAllBufferConsumers();

  fadeout_pipeline_->StartFader(fadeout_duration_, QTimeLine::Backward);
  connect(fadeout_pipeline_.get(), SIGNAL(FaderFinished()), SLOT(FadeoutFinished()));
}


bool GstEngine::Play( uint offset ) {
  // Try to play input pipeline; if fails, destroy input bin
  forever {
    if (current_pipeline_->SetState(GST_STATE_PLAYING))
      break; // Success

    // Failure, but we got a redirection URL - try loading that instead
    QUrl redirect_url = current_pipeline_->redirect_url();
    if (!redirect_url.isEmpty() && redirect_url != current_pipeline_->url()) {
      qDebug() << "Redirecting to" << redirect_url;
      current_pipeline_ = CreatePipeline(redirect_url);
      continue;
    }

    // Failure - give up
    qWarning() << "Could not set thread to PLAYING.";
    current_pipeline_.reset();
    return false;
  }

  // If "Resume playback on start" is enabled, we must seek to the last position
  if (offset) Seek(offset);

  if (timer_id_ != -1)
    killTimer(timer_id_);
  timer_id_ = startTimer(kTimerInterval);

  emit StateChanged(Engine::Playing);
  return true;
}


void GstEngine::Stop() {
  killTimer(timer_id_);
  timer_id_ = -1;

  url_ = QUrl(); // To ensure we return Empty from state()

  if (fadeout_enabled_ && current_pipeline_)
    StartFadeout();

  current_pipeline_.reset();
  emit StateChanged(Engine::Empty);
}

void GstEngine::FadeoutFinished() {
  fadeout_pipeline_.reset();
}

void GstEngine::Pause() {
  if (!current_pipeline_)
    return;

  if ( current_pipeline_->state() == GST_STATE_PLAYING ) {
    current_pipeline_->SetState(GST_STATE_PAUSED);
    emit StateChanged(Engine::Paused);

    killTimer(timer_id_);
    timer_id_ = -1;
  }
}

void GstEngine::Unpause() {
  if (!current_pipeline_)
    return;

  if ( current_pipeline_->state() == GST_STATE_PAUSED ) {
    current_pipeline_->SetState(GST_STATE_PLAYING);
    emit StateChanged(Engine::Playing);

    timer_id_ = startTimer(kTimerInterval);
  }
}

void GstEngine::Seek(uint ms) {
  if (!current_pipeline_)
    return;

  if (!current_pipeline_->Seek(ms * GST_MSECOND))
    qDebug() << "Seek failed";
}

void GstEngine::SetEqualizerEnabled(bool enabled) {
  equalizer_enabled_= enabled;

  if (current_pipeline_)
    current_pipeline_->SetEqualizerEnabled(enabled);
}


void GstEngine::SetEqualizerParameters(int preamp, const QList<int>& band_gains) {
  equalizer_preamp_ = preamp;
  equalizer_gains_ = band_gains;

  if (current_pipeline_)
    current_pipeline_->SetEqualizerParams(preamp, band_gains);
}

void GstEngine::SetVolumeSW( uint percent ) {
  if (current_pipeline_)
    current_pipeline_->SetVolume(percent);
}

void GstEngine::HandlePipelineError(const QString& message) {
  qWarning() << "Gstreamer error:" << message;

  current_pipeline_.reset();
  emit Error(message);
  emit StateChanged(Engine::Empty);
}


void GstEngine::EndOfStreamReached(bool has_next_track) {
  if (!has_next_track)
    current_pipeline_.reset();
  emit TrackEnded();
}

void GstEngine::NewMetaData(const Engine::SimpleMetaBundle& bundle) {
  emit MetaData(bundle);
}

GstElement* GstEngine::CreateElement(
    const QString& factoryName, GstElement* bin, const QString& name ) {
  GstElement* element =
      gst_element_factory_make(
          factoryName.toAscii().constData(),
          name.isNull() ? factoryName.toAscii().constData() : name.toAscii().constData() );

  if ( element ) {
    if ( bin ) gst_bin_add( GST_BIN( bin ), element );
  } else {
    emit Error(QString("GStreamer could not create the element: %1.  "
                       "Please make sure that you have installed all necessary GStreamer plugins (e.g. OGG and MP3)").arg( factoryName ) );
    gst_object_unref( GST_OBJECT( bin ) );
  }

  return element;
}


GstEngine::PluginDetailsList
    GstEngine::GetPluginList(const QString& classname) const {
  PluginDetailsList ret;

  GstRegistry* registry = gst_registry_get_default();
  GList* const features =
      gst_registry_get_feature_list(registry, GST_TYPE_ELEMENT_FACTORY);

  GList* p = features;
  while (p) {
    GstElementFactory* factory = GST_ELEMENT_FACTORY(p->data);
    if (QString(factory->details.klass).contains(classname)) {
      PluginDetails details;
      details.name = QString::fromUtf8(GST_PLUGIN_FEATURE_NAME(p->data));
      details.long_name = QString::fromUtf8(factory->details.longname);
      details.description = QString::fromUtf8(factory->details.description);
      details.author = QString::fromUtf8(factory->details.author);
      ret << details;
    }
    p = g_list_next(p);
  }

  gst_plugin_feature_list_free(features);
  return ret;
}

shared_ptr<GstEnginePipeline> GstEngine::CreatePipeline(const QUrl& url) {
  shared_ptr<GstEnginePipeline> ret(new GstEnginePipeline(this));
  ret->set_output_device(sink_, device_);
  ret->set_replaygain(rg_enabled_, rg_mode_, rg_preamp_, rg_compression_);

  foreach (BufferConsumer* consumer, buffer_consumers_)
    ret->AddBufferConsumer(consumer);

  connect(ret.get(), SIGNAL(EndOfStreamReached(bool)), SLOT(EndOfStreamReached(bool)));
  connect(ret.get(), SIGNAL(Error(QString)), SLOT(HandlePipelineError(QString)));
  connect(ret.get(), SIGNAL(MetadataFound(Engine::SimpleMetaBundle)),
          SLOT(NewMetaData(Engine::SimpleMetaBundle)));
  connect(ret.get(), SIGNAL(destroyed()), SLOT(ClearScopeBuffers()));

  if (!ret->Init(url))
    ret.reset();

  return ret;
}

bool GstEngine::DoesThisSinkSupportChangingTheOutputDeviceToAUserEditableString(const QString &name) {
  return (name == "alsasink" || name == "osssink" || name == "pulsesink");
}

void GstEngine::AddBufferConsumer(BufferConsumer *consumer) {
  buffer_consumers_ << consumer;
  if (current_pipeline_)
    current_pipeline_->AddBufferConsumer(consumer);
}

void GstEngine::RemoveBufferConsumer(BufferConsumer *consumer) {
  buffer_consumers_.removeAll(consumer);
  if (current_pipeline_)
    current_pipeline_->RemoveBufferConsumer(consumer);
}

int GstEngine::AddBackgroundStream(const QUrl& url) {
  shared_ptr<GstEnginePipeline> pipeline = CreatePipeline(url);
  if (!pipeline) {
    return -1;
  }
  pipeline->SetVolume(30);
  // We don't want to get metadata messages or end notifications.
  disconnect(pipeline.get(), SIGNAL(MetadataFound(Engine::SimpleMetaBundle)), this, 0);
  disconnect(pipeline.get(), SIGNAL(EndOfStreamReached(bool)), this, 0);
  connect(pipeline.get(), SIGNAL(EndOfStreamReached(bool)), SLOT(BackgroundStreamFinished()));
  if (!pipeline->SetState(GST_STATE_PLAYING)) {
    qWarning() << "Could not set thread to PLAYING.";
    pipeline.reset();
    return -1;
  }
  pipeline->SetNextUrl(url);
  int stream_id = next_background_stream_id_++;
  background_streams_[stream_id] = pipeline;
  return stream_id;
}

void GstEngine::StopBackgroundStream(int id) {
  background_streams_.remove(id);  // Removes last shared_ptr reference.
}

void GstEngine::BackgroundStreamFinished() {
  GstEnginePipeline* pipeline = qobject_cast<GstEnginePipeline*>(sender());
  pipeline->SetNextUrl(pipeline->url());
}
