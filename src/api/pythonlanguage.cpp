/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>
   
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

#include "availableplugin.h"
#include "pythonlanguage.h"
#include "python_utils.h"
#include "python_wrappers.h"
#include "core/logging.h"

#include <clementine/Clementine>
#include <clementine/Database>
#include <clementine/DatabaseDelegate>
#include <clementine/Player>
#include <clementine/PlayerDelegate>
#include <clementine/Plugin>

#include <boost/python.hpp>
#include <Python.h>

#include <QFile>

using namespace boost::python;


class PythonPlugin : public clementine::Plugin {
public:
  PythonPlugin(object py_object, clementine::ClementinePtr clem,
               const clementine::AvailablePlugin& plugin_info)
    : Plugin(clem, plugin_info),
      py_object_(py_object)
  {
  }

  object py_object_;
};


namespace {
  void LoggingHandler(int level, const QString& logger_name,
                      int lineno, const QString& message) {
    logging::Level        level_name = logging::Level_Debug;
    if      (level >= 40) level_name = logging::Level_Error;
    else if (level >= 30) level_name = logging::Level_Warning;
    else if (level >= 20) level_name = logging::Level_Info;

    QDebug dbg = logging::CreateLogger(level_name, logger_name, lineno);

    // There's no QDebug::operator<< that takes a unicode string without
    // printing quotes around it.  operator<<(const char*) passes the bytes
    // through QString::fromAscii first, destroying any UTF-8 characters.
    // Luckily the QTextStream is the first thing inside the QDebug struct...
    (**reinterpret_cast<QTextStream**>(&dbg)) << message;
  }
}


BOOST_PYTHON_MODULE(clementine) {
  class_<clementine::Clementine, clementine::ClementinePtr, boost::noncopyable>("Clementine", no_init)
      .add_property("player", &clementine::Clementine::player)
      .add_property("database", &clementine::Clementine::database);

  {
    scope s = class_<clementine::Player, clementine::PlayerPtr, boost::noncopyable>("Player", no_init)
        .add_property("volume_percent",
                      &clementine::Player::GetVolumePercent,
                      &clementine::Player::SetVolumePercent)
        .add_property("position_seconds",
                      &clementine::Player::GetPositionSeconds,
                      &clementine::Player::SeekToSeconds)
        .add_property("position_nanoseconds",
                      &clementine::Player::GetPositionNanoseconds,
                      &clementine::Player::SeekToNanoseconds)
        .add_property("state",
                      &clementine::Player::GetState,
                      &clementine::Player::SetState)
        .def("play", &clementine::Player::Play)
        .def("pause", &clementine::Player::Pause)
        .def("play_pause", &clementine::Player::PlayPause)
        .def("stop", &clementine::Player::Stop)
        .def("next", &clementine::Player::Next)
        .def("previous", &clementine::Player::Previous)
        .def("toggle_mute", &clementine::Player::ToggleMute)
        .def("show_osd", &clementine::Player::ShowOSD)
        .def("register_delegate", &clementine::Player::RegisterDelegate)
        .def("unregister_delegate", &clementine::Player::UnregisterDelegate);

    enum_<clementine::Player::State>("State")
        .value("STOPPED", clementine::Player::State_Stopped)
        .value("PLAYING", clementine::Player::State_Playing)
        .value("PAUSED", clementine::Player::State_Paused);
  }

  class_<PlayerDelegateWrapper, boost::noncopyable>("PlayerDelegate")
      .def("state_changed",
           &clementine::PlayerDelegate::StateChanged,
           &PlayerDelegateWrapper::DefaultStateChanged)
      .def("volume_changed",
           &clementine::PlayerDelegate::VolumeChanged,
           &PlayerDelegateWrapper::DefaultVolumeChanged)
      .def("position_changed",
           &clementine::PlayerDelegate::PositionChanged,
           &PlayerDelegateWrapper::DefaultPositionChanged)
      .def("playlist_finished",
           &clementine::PlayerDelegate::PlaylistFinished,
           &PlayerDelegateWrapper::DefaultPlaylistFinished)
      .def("song_changed",
           &clementine::PlayerDelegate::SongChanged,
           &PlayerDelegateWrapper::DefaultSongChanged);

  class_<clementine::Database, clementine::DatabasePtr, boost::noncopyable>("Database", no_init)
      .add_property("database_url", &clementine::Database::DatabaseUrl)
      .def("register_delegate", &clementine::Database::RegisterDelegate)
      .def("unregister_delegate", &clementine::Database::UnregisterDelegate);

  class_<DatabaseDelegateWrapper, boost::noncopyable>("DatabaseDelegate")
      .def("directory_added",
           &clementine::DatabaseDelegate::DirectoryAdded,
           &DatabaseDelegateWrapper::DefaultDirectoryAdded)
      .def("directory_removed",
           &clementine::DatabaseDelegate::DirectoryRemoved,
           &DatabaseDelegateWrapper::DefaultDirectoryRemoved)
      .def("songs_changed",
           &clementine::DatabaseDelegate::SongsChanged,
           &DatabaseDelegateWrapper::DefaultSongsChanged)
      .def("songs_removed",
           &clementine::DatabaseDelegate::SongsRemoved,
           &DatabaseDelegateWrapper::DefaultSongsRemoved)
      .def("total_song_count_updated",
           &clementine::DatabaseDelegate::TotalSongCountUpdated,
           &DatabaseDelegateWrapper::DefaultTotalSongCountUpdated);

  def("logging_handler", LoggingHandler);
}


PythonLanguage::PythonLanguage(Application* app)
  : clementine::Language(app) {
}

bool PythonLanguage::Init() {
  Py_Initialize();
  initclementine();

  python_utils::RegisterTypeConverters();

  if (!python_utils::AddModule(":python/models.py", "clementine.models"))
    return false;

  if (!python_utils::AddModule(":python/logging_integration.py", "clementine.logging_integration"))
    return false;

  if (!python_utils::AddModule(":python/sqlalchemy_integration.py", "clementine.sqlalchemy_integration"))
    return false;

  return true;
}

clementine::Plugin* PythonLanguage::LoadPlugin(const clementine::AvailablePlugin& plugin) {
  // Open the source file
  const QString filename = plugin.path_ + "/__main__.py";
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    qLog(Warning) << "Couldn't open Python file" << filename;
    return NULL;
  }

  // Read its contents
  const QByteArray source = file.readAll();

  object module(python_utils::AddModule(source, filename.toUtf8(),
        QString("clementineplugin_" + plugin.id_).toUtf8()));
  if (!module) {
    return NULL;
  }

  // Get the plugin's Plugin class.
  object plugin_class;
  try {
    plugin_class = module.attr("Plugin");
  } catch (error_already_set&) {
    if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
      qLog(Warning) << plugin.id_
                    << ": You must define a class called 'Plugin' in __main__.py";
    } else {
      PyErr_Print();
    }
    PyErr_Clear();
    return NULL;
  }

  // Create a new Clementine instance for this plugin.
  clementine::ClementinePtr clementine(new clementine::Clementine(app()));

  // Try to instantiate it.
  object py_object;
  try {
    py_object = plugin_class(clementine);
  } catch (error_already_set&) {
    PyErr_Print();
    PyErr_Clear();
    return NULL;
  }

  // Create and return the C++ plugin.
  return new PythonPlugin(py_object, clementine, plugin);
}

void PythonLanguage::UnloadPlugin(clementine::Plugin* plugin) {
  // Grab a reference to the actual Python Plugin instance so we can get its
  // reference count after we delete the C++ plugin.
  object py_object = static_cast<PythonPlugin*>(plugin)->py_object_;
  const QString plugin_id = plugin->plugin_info().id_;

  delete plugin;

  // The plugin's refcount should now be exactly 1.  If it isn't, it's holding
  // a circular reference.
  if (py_object.ptr()->ob_refcnt != 1) {
    // Try running the garbage collector.
    PyGC_Collect();

    if (py_object.ptr()->ob_refcnt != 1) {
      // If the reference count is still greater than 1 there's nothing we can
      // do - it's a bug in the plugin.
      qLog(Warning) << "Plugin" << plugin_id << "has a circular reference"
                    << "(refcount" << py_object.ptr()->ob_refcnt << ") and"
                    << "could not be unloaded properly.";
    }
  }
}
