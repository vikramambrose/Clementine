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
#include <clementine/Player>
#include <clementine/PlayerDelegate>
#include <clementine/Plugin>

#include <boost/python.hpp>
#include <Python.h>

#include <QFile>

using namespace boost::python;


class PythonPlugin : public clementine::Plugin {
public:
  PythonPlugin(object py_object, clementine::Clementine* clem,
               const clementine::AvailablePlugin& plugin_info)
    : Plugin(clem, plugin_info),
      py_object_(py_object)
  {
  }

private:
  object py_object_;
};


BOOST_PYTHON_MODULE(clementine) {
  class_<clementine::Clementine, boost::noncopyable>("Clementine", no_init)
      .add_property("player",
                    make_function(&clementine::Clementine::player,
                                  return_value_policy<reference_existing_object>()));

  {
    scope s = class_<clementine::Player, boost::noncopyable>("Player", no_init)
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
}


PythonLanguage::PythonLanguage(clementine::Clementine* clem)
  : clementine::Language(clem) {
}

bool PythonLanguage::Init() {
  Py_Initialize();
  initclementine();

  python_utils::RegisterTypeConverters();

  if (!python_utils::AddModule(":python/models.py", "clementine.models"))
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

  // Try to instantiate it.
  object py_object;
  try {
    object py_app(handle<>(
        reference_existing_object::apply<clementine::Clementine*>::type()(clementine_)));
    py_object = plugin_class(py_app);
  } catch (error_already_set&) {
    PyErr_Print();
    PyErr_Clear();
    return NULL;
  }

  // Create and return the C++ plugin.
  return new PythonPlugin(py_object, clementine_, plugin);
}

void PythonLanguage::UnloadPlugin(clementine::Plugin* plugin) {
}
